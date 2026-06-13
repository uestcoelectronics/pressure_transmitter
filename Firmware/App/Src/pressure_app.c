#include "pressure_app.h"
#include "bsp_pins.h"
#include "buttons.h"
#include "cal_storage.h"
#include "fdc2214.h"
#include "lcd400.h"
#include "state_machine.h"
#include "temp_diode.h"
#include "tmp108.h"
#include "xtr111_loop.h"
#include "diag.h"
#include "ble_uart.h"
#include "stm32u3xx_hal.h"
#ifdef USE_IWDG
#include "iwdg.h"        /* CubeMX'in IWDG handle'ı */
#endif
#include <stdio.h>
#include <string.h>
#include <math.h>

/* -------------------------------------------------------------------------- */
/* SICAKLIK MİMARİSİ — rol ayrımı (kullanıcı teyitli, 2026-06-12):             */
/*                                                                            */
/*   temp_diode (2× 1N4148, PC0/PC1, sensör modülü içinde)                    */
/*     → YALNIZ basınç kompanzasyonu. Çift kanal arbitrasyonlu; tutarsızlıkta */
/*       son tutarlı değerle devam eder (degraded-but-operational): ölçüm ve  */
/*       4-20 mA çıkışı DURMAZ, ekranda "TDIODE ERR" gösterilir.              */
/*                                                                            */
/*   tmp108 (B701, I2C, kart üstü)                                            */
/*     → YALNIZ ortam sıcaklığı izleme + 60 °C alert (FLT_TEMP#/PB5 EXTI).    */
/*       Kompanzasyona girmez; diyotlara failover YAPILMAZ (farklı fiziksel   */
/*       konum → yanlış kompanzasyon üretir). Alarmda ölçüm devam eder.       */
/*                                                                            */
/*   Alarm-low (3.6 mA) yalnız basınç sensörü (FDC2214) hatasında sürülür.    */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* ADC dönüştürme — 3 kanal, regular sequence (PC0 → PC4 → PC5)                */
/* -------------------------------------------------------------------------- */
static uint16_t s_adc_buf[ADC_RANK_COUNT];
static volatile bool s_adc_done = false;

static void start_adc_scan(void)
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)s_adc_buf, ADC_RANK_COUNT);
}

/* -------------------------------------------------------------------------- */
/* Watchdog beslemesi — harici TPS3851 (pencereli, WDI=PC3) + dahili IWDG      */
/*                                                                            */
/* TPS3851: WDI DÜŞEN-kenar tetiklemeli, PENCERELİ — kick çok erken VEYA çok  */
/* geç olursa reset. tWD CWD pinine bağlı (şema teyidi MANUAL-2 madde 6).     */
/* WDT_KICK_PERIOD_MS bu pencerenin İÇİNDE olmalı: tWD_MIN < P < tWD_MAX.     */
/*                                                                            */
/* wdt_feed_raw(): net düşen kenar + IWDG refresh; KOŞULSUZ (flash gibi       */
/* kontrollü uzun işlemlerden de çağrılır — bkz. cal_storage.c).             */
/* Ana döngüde besleme, güvenlik görevinin (sensör+loop tiki) canlılığına     */
/* koşullanır (FMEDA A.16 program-akış izleme): görev WDT_HEALTH_TIMEOUT_MS   */
/* içinde çalışmadıysa besleme durur → harici WDT+IWDG reset → güvenli durum. */
/* -------------------------------------------------------------------------- */
#define WDT_KICK_PERIOD_MS     100u
#define WDT_HEALTH_TIMEOUT_MS  400u   /* güvenlik görevi bu süre çalışmazsa besleme durur */

static volatile uint32_t s_loop_token = 0;   /* sensör+loop görevinin son çalışma anı */

void wdt_feed_raw(void)
{
    /* Net düşen kenar: önce HIGH garanti, sonra kısa LOW darbe, sonra HIGH.
       TPS3851 WDI kenar tetiklemeli; µs mertebesinde darbe yeterli.          */
    HAL_GPIO_WritePin(WDI_PORT, WDI_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(WDI_PORT, WDI_PIN, GPIO_PIN_RESET);   /* ↓ düşen kenar  */
    for (volatile uint32_t i = 0; i < 100u; ++i) { __NOP(); }
    HAL_GPIO_WritePin(WDI_PORT, WDI_PIN, GPIO_PIN_SET);
#ifdef USE_IWDG
    HAL_IWDG_Refresh(&hiwdg);
#endif
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *h)
{
    if (h == &hadc1) s_adc_done = true;
}

/* -------------------------------------------------------------------------- */
/* Damping (IIR low-pass): y += alpha * (x - y)                                */
/*   alpha = dt / (tau + dt)                                                  */
/* -------------------------------------------------------------------------- */
static float iir_step(float y, float x, float tau_s, float dt_s)
{
    if (tau_s <= 0.0f) return x;
    float alpha = dt_s / (tau_s + dt_s);
    return y + alpha * (x - y);
}

/* -------------------------------------------------------------------------- */
/* Capacitance → engineering unit dönüşümü (lineer, 2-nokta + T kompansasyon)  */
/* -------------------------------------------------------------------------- */
static float dC_to_pressure(int32_t dC, const cal_params_t *c, float t_celsius)
{
    int32_t span = c->cap_at_span - c->cap_at_zero;
    if (span == 0) return c->p_min;
    float frac = (float)(dC - c->cap_at_zero) / (float)span;
    float p    = c->p_min + frac * (c->p_max - c->p_min);
    /* Sıcaklık kompansasyonu v2: zero (offset) + span (gain, okumayla
       orantılı). k_t_span=0 → v1 davranışı.                                */
    float dT = t_celsius - c->t_ref;
    p -= (c->k_t_zero + c->k_t_span * frac) * dT;
    return p;
}

static float pressure_to_ma(float p, const cal_params_t *c)
{
    float span_p = c->p_max - c->p_min;
    if (span_p == 0.0f) return 4.0f;
    float frac = (p - c->p_min) / span_p;
    /* NAMUR NE43: aralık dışı ölçüm 4/20'de kesilmez, satürasyon
       seviyesine taşar (alıcı aralık aşımını ayırt edebilsin).             */
    if (frac < 0.0f) return LOOP_SAT_LOW_MA;
    if (frac > 1.0f) return LOOP_SAT_HIGH_MA;
    return 4.0f + frac * 16.0f;
}

/* -------------------------------------------------------------------------- */
/* Görüntüleme yardımcıları                                                    */
/* -------------------------------------------------------------------------- */
/* Son gösterim değerleri (sensör tikinde güncellenir) */
static float   s_disp_p  = 0.0f;
static int32_t s_disp_dc = 0;

/* Durum satırı önceliği (tüm NORMAL sayfalarında ortak) */
static const char *status_line(void)
{
    return loop_is_in_fault()           ? "*FAULT*"      :
           fdc2214_has_error()          ? "SENSOR ERR"   :
           loop_has_deviation()         ? "LOOP DEV"     :
           !temp_diode_is_consistent()  ? "TDIODE ERR"   :
           tmp108_overtemp()            ? "AMB HOT >60C" :
           diag_any()                   ? "DIAG CHK"     :
           loop_is_enabled()            ? "OK"           : "LOOP DISABLED";
}

/* Loop fault → adanmış alarm ekranı (CARD-4.1: 5 s sonra auto-retry) */
static void render_fault(void)
{
    lcd_write_line(0, "*** LOOP FAULT ***");
    lcd_write_line(1, "FLT input (PA6)");
    lcd_write_line(2, "Output: 3.6 mA");
    lcd_write_line(3, "auto-retry 5s");
}

static void render_normal(uint8_t page)
{
    char line[24];
    switch (page) {
    default:
    case 0:   /* MAIN: P / I / T / durum */
        snprintf(line, sizeof line, "P=%7.3f bar     ", (double)s_disp_p);  lcd_write_line(0, line);
        snprintf(line, sizeof line, "I=%5.2f mA       ", (double)loop_get_measured_ma()); lcd_write_line(1, line);
        snprintf(line, sizeof line, "T=%5.1f C        ", (double)temp_diode_get_celsius()); lcd_write_line(2, line);
        lcd_write_line(3, status_line());
        break;
    case 1:   /* SENSOR: ham dC + iki diyot + ortam */
        lcd_write_line(0, "-- SENSOR --");
        snprintf(line, sizeof line, "dC=%ld", (long)s_disp_dc); lcd_write_line(1, line);
        snprintf(line, sizeof line, "Td1=%4.1f Td2=%4.1f",
                 (double)temp_diode_get_ch_celsius(0),
                 (double)temp_diode_get_ch_celsius(1)); lcd_write_line(2, line);
        snprintf(line, sizeof line, "Tamb=%4.1f C", (double)tmp108_get_ambient_c()); lcd_write_line(3, line);
        break;
    case 2:   /* LOOP: komut / ölçüm / sapma */
        lcd_write_line(0, "-- LOOP --");
        snprintf(line, sizeof line, "cmd =%5.2f mA", (double)loop_get_commanded_ma()); lcd_write_line(1, line);
        snprintf(line, sizeof line, "meas=%5.2f mA", (double)loop_get_measured_ma()); lcd_write_line(2, line);
        snprintf(line, sizeof line, "err =%+5.2f mA", (double)loop_get_error_ma()); lcd_write_line(3, line);
        break;
    }
}

static void render_menu(int idx, const char *label)
{
    char line[24];
    snprintf(line, sizeof line, "MENU %d/12", idx + 1);  /* MI__COUNT ile senkron */
    lcd_write_line(0, line);
    lcd_write_line(1, label);
    /* Transient mesaj (yakalama/kayıt sonucu, çıkış onayı) varsa göster     */
    const char *msg = sm_get_info_msg();
    lcd_write_line(2, msg[0] ? msg : "UP/DN: navigate");
    lcd_write_line(3, "SET: select");
}

static void render_edit(const char *label, float v)
{
    char line[24];
    lcd_write_line(0, label);
    snprintf(line, sizeof line, "Value: %.3f", (double)v); lcd_write_line(1, line);
    lcd_write_line(2, "UP/DN: +/-");
    lcd_write_line(3, "SET: save  long: esc");
}

static void render_cal_live(const char *label, int32_t dc, float p)
{
    char line[24];
    lcd_write_line(0, label);
    snprintf(line, sizeof line, "dC=%ld", (long)dc); lcd_write_line(1, line);
    snprintf(line, sizeof line, "P=%6.2f bar", (double)p); lcd_write_line(2, line);
    /* Yakalama yalnız stabilken kabul edilir; red mesajı öncelikli          */
    const char *msg = sm_get_info_msg();
    lcd_write_line(3, msg[0]               ? msg :
                      sm_cal_live_stable() ? "STABLE: SET long" : "WAIT: unstable..");
}

/* -------------------------------------------------------------------------- */
/* Init                                                                        */
/* -------------------------------------------------------------------------- */
void pressure_app_init(void)
{
    /* Tüm output GPIO'lar safe default'larda (boot LOW). */
    cal_init();
    /* vf25/tc'nin tek kaynağı cal_params — runtime'a yükle (v2 persistans) */
    temp_diode_set_calibration(cal_get()->vf25_mv, cal_get()->tc_mv_c);
    buttons_init();
    sm_init();

    /* WDI idle = HIGH (düşen-kenar dog); ilk besleme hemen yapılır.
       Boot'ta token taze sayılsın ki ilk pencere kaçmasın.                  */
    HAL_GPIO_WritePin(WDI_PORT, WDI_PIN, GPIO_PIN_SET);
    s_loop_token = HAL_GetTick();
    wdt_feed_raw();

    /* LCD önce — kullanıcıya hayat işareti */
    lcd_init();
    lcd_write_line(0, "  PressureTx 4-20");
    lcd_write_line(1, "  booting...");
    lcd_flush();

    /* Sensor init — başarısızsa bir kez güç çevrimiyle dene                  */
    bool fdc_ok = fdc2214_init();
    if (!fdc_ok) fdc_ok = fdc2214_init();
    if (fdc_ok) fdc2214_start();

    /* TMP108 ortam monitörü — başarısızlık ölümcül değil (ölçüm sürer)       */
    (void)tmp108_init();

    diag_init();

    /* BLE taşıma katmanı (DL-CC2340-B) — güç sırası + USART3 IT RX.
       AT protokolü ve advertise CARD-5.2'de.                               */
    ble_uart_init();

    /* Loop output init (safe state'te kalır) */
    loop_init();

    /* ADC DMA'yı başlat */
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    start_adc_scan();

    /* Kısa bir feedback satırı + loop'u açtık */
    if (fdc_ok) lcd_write_line(2, "  sensor OK");
    else        lcd_write_line(2, "  SENSOR FAIL");
    lcd_write_line(3, " ");
    lcd_flush();
    HAL_Delay(500);

    if (fdc_ok) loop_enable(true);
}

/* -------------------------------------------------------------------------- */
/* Süper döngü                                                                  */
/* -------------------------------------------------------------------------- */
void pressure_app_loop(void)
{
    static uint32_t t_sense   = 0;
    static uint32_t t_disp    = 0;
    static uint32_t t_wdi     = 0;
    static uint32_t t_btn     = 0;
    static uint32_t t_amb     = 0;
    static uint32_t t_led     = 0;
    static bool     led_state = false;
    static float    p_filt    = 0.0f;
    static float    p_inst    = 0.0f;

    uint32_t now = HAL_GetTick();

    /* ---- Buton tarama (her 5 ms) ---- */
    if ((now - t_btn) >= 5) {
        t_btn = now;
        buttons_poll(now);
        sm_tick(now);                       /* zaman tabanı + 60 s timeout */
        btn_event_t e = buttons_get_event();
        if (e != BTN_EVT_NONE) sm_handle_event(e);
    }

    /* ---- Sensör + ADC döngüsü (her 100 ms) ---- */
    if ((now - t_sense) >= 100) {
        t_sense = now;
        s_loop_token = now;   /* güvenlik görevi çalıştı (A.16 canlılık kanıtı) */

        /* ADC sonucu hazırsa işleyelim, yenisini başlatalım */
        if (s_adc_done) {
            s_adc_done = false;
            temp_diode_update(s_adc_buf[ADC_RANK_TDIODE],
                              s_adc_buf[ADC_RANK_TDIODE2]);
            loop_update_feedback_from_adc(s_adc_buf[ADC_RANK_ILOOP_FB]);
            /* s_adc_buf[ADC_RANK_VCC_FB] — supply monitor için ayrılmış */
        }
        start_adc_scan();

        /* FDC2214 hata pini (ERRB) yoklaması — asserted ise STATUS okunur    */
        bool errb_now  = fdc2214_poll_errb();
        bool i2c_fail  = false;
        bool got_data  = false;
        int32_t dC = 0;

        /* INT_B data-ready değilse bu tikte okuma atlanır (son değer korunur;
           conversion ~100 ms olduğundan genelde hazırdır)                    */
        if (!errb_now && fdc2214_data_ready()) {
            if (fdc2214_read_delta(&dC)) got_data = true;
            else                         i2c_fail = true;
        }

        if (got_data) {
            /* ERRB düzelmiş ve okuma başarılı → latched hatayı temizle       */
            if (fdc2214_has_error()) fdc2214_clear_error();

            const cal_params_t *c = cal_get();
            float t_c = temp_diode_get_celsius();
            p_inst = dC_to_pressure(dC, c, t_c);
            /* Damping */
            p_filt = iir_step(p_filt, p_inst, c->damping_s, 0.1f);
            s_disp_p  = p_filt;             /* sayfa render'ı için cache    */
            s_disp_dc = dC;

            /* state machine'e canlı veri */
            if (sm_get_state() == SM_CAL_LIVE) {
                sm_update_live(dC, p_inst);
            }

            /* NORMAL state'te loop'u sür */
            if (sm_get_state() == SM_NORMAL && loop_is_enabled() && !loop_is_in_fault()) {
                loop_set_current_ma(pressure_to_ma(p_filt, c));
            }
        } else if (errb_now || i2c_fail || fdc2214_has_error()) {
            /* Sensör hatası (ERRB veya I2C): alarm-low */
            if (loop_is_enabled()) loop_set_current_ma(LOOP_ALARM_LOW_MA);
        }
        /* data_ready değilse ve hata yoksa: sadece bu tik atlandı             */

        /* Loop servis: sapma monitörü + fault auto-retry                      */
        loop_service(now);

        /* ---- Tanılar (A.13 ADC rail-stuck, A.7 GPIO read-back) ---- */
        diag_service(s_adc_buf[ADC_RANK_VCC_FB], s_adc_buf[ADC_RANK_ILOOP_FB]);
        /* LOOP_EN read-back kritik arızası → güvenli durum */
        if (diag_critical() && !loop_is_in_fault()) {
            loop_set_safe_state();
        }

        /* ---- I2C bus recovery: iki cihaz da uzun süre sağlıksızsa ---- */
        static uint8_t  i2c_bad_cnt = 0;
        static uint32_t i2c_recov_t = 0;
        if (fdc2214_has_error() && !tmp108_is_ok()) {
            if (i2c_bad_cnt < 255u) i2c_bad_cnt++;
        } else {
            i2c_bad_cnt = 0;
        }
        /* 5 ardışık tik (~500 ms) + en az 5 s cooldown */
        if (i2c_bad_cnt >= 5u && (now - i2c_recov_t) >= 5000u) {
            i2c_recov_t = now;
            i2c_bad_cnt = 0;
            diag_i2c_bus_recover();
            (void)fdc2214_init();   /* bus serbest → cihazları yeniden kur   */
            if (fdc2214_has_error() == false) fdc2214_start();
            (void)tmp108_init();
        }
    }

    /* ---- Ortam sıcaklığı (TMP108, her 1000 ms) ---- */
    if ((now - t_amb) >= 1000) {
        t_amb = now;
        tmp108_poll();
    }

    /* ---- Ekran tazeleme (her 250 ms) ---- */
    if ((now - t_disp) >= 250) {
        t_disp = now;
        switch (sm_get_state()) {
            case SM_NORMAL:
                if (loop_is_in_fault()) render_fault();
                else                    render_normal(sm_get_normal_page());
                break;
            case SM_MENU:
                render_menu(sm_get_menu_idx(), sm_get_edit_label());
                break;
            case SM_EDIT_FLOAT:
                render_edit(sm_get_edit_label(), sm_get_edit_value());
                break;
            case SM_CAL_LIVE: {
                /* live değerleri tekrar oku */
                int32_t dC = 0;
                fdc2214_read_delta(&dC);
                render_cal_live(sm_get_edit_label(), dC, p_inst);
                break;
            }
            default: break;
        }
        lcd_flush();
    }

    /* ---- WDT besleme (her 100 ms; yalnız güvenlik görevi canlıysa) ---- */
    if ((now - t_wdi) >= WDT_KICK_PERIOD_MS) {
        t_wdi = now;
        if ((now - s_loop_token) <= WDT_HEALTH_TIMEOUT_MS) {
            wdt_feed_raw();
        }
        /* token bayatsa: besleme yok → harici WDT + IWDG reset (güvenli durum) */
    }

    /* ---- Status LED (1 Hz heartbeat; fault/sapmada 5 Hz) ---- */
    uint32_t blink = (loop_is_in_fault() || loop_has_deviation()) ? 100 : 500;
    if ((now - t_led) >= blink) {
        t_led = now;
        led_state = !led_state;
        HAL_GPIO_WritePin(LED_PORT, LED_PIN,
                          led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

/* EXTI Falling callback main.c (USER CODE BEGIN 4) içinde tanımlandı. */
