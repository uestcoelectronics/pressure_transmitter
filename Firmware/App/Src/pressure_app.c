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
#include "stm32u3xx_hal.h"
#ifdef USE_IWDG
#include "iwdg.h"        /* CubeMX'in IWDG handle'ı */
#endif
#include <stdio.h>
#include <string.h>
#include <math.h>

/* -------------------------------------------------------------------------- */
/* ADC dönüştürme — 3 kanal, regular sequence (PC0 → PC4 → PC5)                */
/* -------------------------------------------------------------------------- */
static uint16_t s_adc_buf[ADC_RANK_COUNT];
static volatile bool s_adc_done = false;

static void start_adc_scan(void)
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)s_adc_buf, ADC_RANK_COUNT);
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
    /* Sıcaklık kompansasyonu */
    p -= c->k_t * (t_celsius - c->t_ref);
    return p;
}

static float pressure_to_ma(float p, const cal_params_t *c)
{
    float span_p = c->p_max - c->p_min;
    if (span_p == 0.0f) return 4.0f;
    float frac = (p - c->p_min) / span_p;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    return 4.0f + frac * 16.0f;
}

/* -------------------------------------------------------------------------- */
/* Görüntüleme yardımcıları                                                    */
/* -------------------------------------------------------------------------- */
static void render_normal(float p, float t, float ma)
{
    char line[24];
    snprintf(line, sizeof line, "P=%7.3f bar     ", (double)p); lcd_write_line(0, line);
    snprintf(line, sizeof line, "I=%5.2f mA       ", (double)ma); lcd_write_line(1, line);
    snprintf(line, sizeof line, "T=%5.1f C        ", (double)t); lcd_write_line(2, line);
    lcd_write_line(3, loop_is_in_fault()   ? "*FAULT*"    :
                       fdc2214_has_error() ? "SENSOR ERR" :
                       tmp108_overtemp()   ? "AMB HOT >60C" :
                       loop_is_enabled()   ? "OK"         : "LOOP DISABLED");
}

static void render_menu(int idx, const char *label)
{
    char line[24];
    snprintf(line, sizeof line, "MENU %d/10", idx + 1);
    lcd_write_line(0, line);
    lcd_write_line(1, label);
    lcd_write_line(2, "UP/DN: navigate");
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
    lcd_write_line(3, "SET long: capture");
}

/* -------------------------------------------------------------------------- */
/* Init                                                                        */
/* -------------------------------------------------------------------------- */
void pressure_app_init(void)
{
    /* Tüm output GPIO'lar safe default'larda (boot LOW). */
    cal_init();
    buttons_init();
    sm_init();

    /* Watchdog kick pin'i LOW; ilk kick'i hemen yapacağız. */
    HAL_GPIO_WritePin(WDI_PORT, WDI_PIN, GPIO_PIN_RESET);

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
        btn_event_t e = buttons_get_event();
        if (e != BTN_EVT_NONE) sm_handle_event(e);
    }

    /* ---- Sensör + ADC döngüsü (her 100 ms) ---- */
    if ((now - t_sense) >= 100) {
        t_sense = now;

        /* ADC sonucu hazırsa işleyelim, yenisini başlatalım */
        if (s_adc_done) {
            s_adc_done = false;
            temp_diode_update_from_adc(s_adc_buf[ADC_RANK_TDIODE]);
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
            if (loop_is_enabled()) loop_set_current_ma(3.6f);
        }
        /* data_ready değilse ve hata yoksa: sadece bu tik atlandı             */
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
                render_normal(p_filt,
                              temp_diode_get_celsius(),
                              loop_get_measured_ma());
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

    /* ---- WDT kick (her 100 ms — TPS3851 timeout > 200 ms tipik) ---- */
    if ((now - t_wdi) >= 100) {
        t_wdi = now;
        HAL_GPIO_TogglePin(WDI_PORT, WDI_PIN);
    #ifdef USE_IWDG
        HAL_IWDG_Refresh(&hiwdg);   /* iwdg.h'ten extern hiwdg          */
    #endif
    }

    /* ---- Status LED (1 Hz heartbeat, fault'ta 5 Hz) ---- */
    uint32_t blink = loop_is_in_fault() ? 100 : 500;
    if ((now - t_led) >= blink) {
        t_led = now;
        led_state = !led_state;
        HAL_GPIO_WritePin(LED_PORT, LED_PIN,
                          led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

/* EXTI Falling callback main.c (USER CODE BEGIN 4) içinde tanımlandı. */
