#include "xtr111_loop.h"
#include "bsp_pins.h"
#include "stm32u3xx_hal.h"

/* XTR111 transfer:   I_OUT = 10 * V_SET / R_SET                        */
/* Şemada R_SET = R409 = 1.2 kΩ (0.1% precision)                        */
/*    I[mA] = V_SET[V] × 10 / 1200 × 1000 = V_SET × 8.333               */
/*    V_SET[V] = 0.12 × I[mA]                                            */
/*    4  mA → V_SET = 0.48 V → DAC ≈ 596                                 */
/*    20 mA → V_SET = 2.40 V → DAC ≈ 2979                                */
#define V_SET_PER_MA       (0.12f)
#define DAC_PER_VOLT       ((float)DAC_FULLSCALE / 3.3f)   /* 1241.0... */

/* Loop çıkış kalibrasyonu — donanım 2-nokta uç trim (bring-up, 2026-06-23):
   ÖLÇÜLDÜ: kod 699 → tam 4.00 mA, kod 3011 → tam 20.00 mA.
   kod = 144.5*mA + 121   (eğim=(3011-699)/16=144.5, offset=699-4*144.5=121)
   (XTR111+DAC zincirinde ~-0.84mA offset var; eski offset'siz model şaşıyordu) */
#define LOOP_CAL_CODE_PER_MA   (144.5f)
#define LOOP_CAL_CODE_OFFSET   (121.0f)

/* Komut penceresi: NAMUR alarm seviyeleri dahil (3.6 mA alarm-low'un
   gerçekten üretilebilmesi için — eski 3.8 clamp'i alarm'ı yutuyordu).     */
#define I_MIN_MA           LOOP_ALARM_LOW_MA
#define I_MAX_MA           LOOP_ALARM_HIGH_MA

/* Sapma monitörü + fault retry parametreleri                               */
#define DEV_MAX_MA         (0.3f)
#define DEV_TIME_MS        (2000u)
#define FAULT_RETRY_MS     (5000u)
/* XTR111 güç-açılış FLT glitch maskesi: enable sonrası bu süre boyunca FLT
   kenarı yok sayılır (XTR111 oturma süresi). Settling SONRASI seviye kontrolü
   gerçek/sürekli arızayı yine yakalar — güvenlik korunur.                   */
#define FLT_SETTLE_MS      (100u)

static float s_cmd_ma   = 4.0f;
static float s_meas_ma  = 0.0f;
static volatile bool s_fault = false;
static volatile uint32_t s_fault_t = 0;
static bool     s_deviation = false;
static uint32_t s_dev_t     = 0;      /* eşik geçiş/dönüş zaman damgası    */
static bool     s_dev_over  = false;  /* şu an eşik üstünde mi             */
static volatile uint32_t s_flt_ignore_until = 0;  /* enable sonrası FLT mask sonu */

/* BRING-UP karakterizasyon hook'u: >=0 iken loop'un DAC yazımını ezer, sabit
   DAC kodu sürer (V_SET süpürme). Debugger'dan yazılır; üretimde -1 (kapalı). */
volatile int32_t g_loop_dbg_dac = -1;
/* >=0 iken: bu mA'yı ma_to_dac_code üzerinden sür (kalibrasyon doğrulaması). */
volatile float g_loop_dbg_ma = -1.0f;

static uint32_t ma_to_dac_code(float ma)
{
    if (ma < I_MIN_MA) ma = I_MIN_MA;
    if (ma > I_MAX_MA) ma = I_MAX_MA;
    /* Donanım-kalibreli ters dönüşüm: kod = 146.5*mA + 93.9 (3-nokta fit).
       Eski V_SET_PER_MA/DAC_PER_VOLT modeli offset'i içermiyordu.            */
    int32_t code = (int32_t)(ma * LOOP_CAL_CODE_PER_MA + LOOP_CAL_CODE_OFFSET + 0.5f);
    if (code < 0) code = 0;
    if (code > (int32_t)DAC_FULLSCALE) code = (int32_t)DAC_FULLSCALE;
    return (uint32_t)code;
}

void loop_init(void)
{
    /* DAC start + ilk değer 4 mA (loop disabled olsa bile DAC hazır)       */
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R,
                     ma_to_dac_code(4.0f));
    s_cmd_ma  = 4.0f;
    s_meas_ma = 0.0f;
    s_fault   = false;
    HAL_GPIO_WritePin(LOOP_EN_PORT, LOOP_EN_PIN, GPIO_PIN_RESET);  /* safe */
}

void loop_enable(bool on)
{
    if (on && !s_fault) {
        /* FLT maskesini PB2'yi sürmeden ÖNCE kur: XTR111 güç-açılışında FLT'yi
           anlık LOW yapar; bu glitch'i settling penceresinde yok say.          */
        s_flt_ignore_until = HAL_GetTick() + FLT_SETTLE_MS;
        HAL_GPIO_WritePin(LOOP_EN_PORT, LOOP_EN_PIN, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(LOOP_EN_PORT, LOOP_EN_PIN, GPIO_PIN_RESET);
    }
}

bool loop_is_enabled(void)
{
    return HAL_GPIO_ReadPin(LOOP_EN_PORT, LOOP_EN_PIN) == GPIO_PIN_SET;
}

void loop_set_current_ma(float ma)
{
    s_cmd_ma = ma;
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R,
                     ma_to_dac_code(ma));
}

float loop_get_commanded_ma(void)            { return s_cmd_ma; }

void loop_update_feedback_from_adc(uint16_t adc_raw)
{
    /* V_fb [V] = adc * 3.3 / 4095; I [mA] = V_fb / (100 * 1Ω) * 1000 = V_fb*10 */
    float v_fb = ((float)adc_raw * 3.3f) / (float)ADC_FULLSCALE;
    s_meas_ma  = v_fb * 10.0f;
}

float loop_get_measured_ma(void)             { return s_meas_ma; }
float loop_get_error_ma(void)                { return s_cmd_ma - s_meas_ma; }

void loop_set_safe_state(void)
{
    s_fault   = true;
    s_fault_t = HAL_GetTick();
    HAL_GPIO_WritePin(LOOP_EN_PORT, LOOP_EN_PIN, GPIO_PIN_RESET);
    /* DAC'i de en düşük güvenli değere getir (alarm-low 3.6 mA komutu;    */
    /* XTR111 zaten enable=0'da yüksek empedans olduğu için sembolik)      */
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R,
                     ma_to_dac_code(LOOP_ALARM_LOW_MA));
    s_cmd_ma = LOOP_ALARM_LOW_MA;
}

bool loop_is_in_fault(void)  { return s_fault; }
void loop_clear_fault(void)  { s_fault = false; }

/* BRING-UP: g_loop_dbg_dac >=0 ise DAC'ı o koda zorla (loop yazımını ezer).
   Her 100 ms, tüm loop DAC yazımlarından SONRA çağrılmalı.                    */
void loop_dbg_apply(void)
{
    if (g_loop_dbg_ma >= 0.0f) {
        /* mA'yı kalibreli ma_to_dac_code'dan geçir → çıkış bu mA'yı vermeli */
        HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, ma_to_dac_code(g_loop_dbg_ma));
    } else if (g_loop_dbg_dac >= 0) {
        if (g_loop_dbg_dac > (int32_t)DAC_FULLSCALE) g_loop_dbg_dac = (int32_t)DAC_FULLSCALE;
        HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, (uint32_t)g_loop_dbg_dac);
    }
}

/* FLT (PA6) düşen-kenar — main.c EXTI callback'inden çağrılır.
   Enable sonrası settling penceresindeyse XTR111 güç-açılış glitch'idir → yok
   say. Pencere dışındaki kenar gerçek arızadır → safe state.                   */
void loop_on_flt_edge(void)
{
    if ((int32_t)(HAL_GetTick() - s_flt_ignore_until) < 0) {
        return;   /* settling: power-up glitch, yok say */
    }
    loop_set_safe_state();
}

bool loop_has_deviation(void) { return s_deviation; }

void loop_service(uint32_t now_ms)
{
    /* ---- Fault auto-retry (FLT latch'inden 5 s sonra) ---- */
    if (s_fault) {
        s_deviation = false;          /* fault sapmayı kapsar              */
        s_dev_over  = false;
        if ((now_ms - s_fault_t) >= FAULT_RETRY_MS) {
            /* EXTI yalnız düşen kenarı görür: FLT hâlâ asserted (LOW) ise
               retry'ı ertele, yoksa kalıcı arıza sessiz kalırdı.           */
            if (HAL_GPIO_ReadPin(LOOP_FLT_PORT, LOOP_FLT_PIN) == GPIO_PIN_RESET) {
                s_fault_t = now_ms;   /* bekleme penceresini yenile         */
            } else {
                s_fault = false;
                loop_enable(true);    /* komut, app'in sonraki tikinde gelir */
            }
        }
        return;
    }

    /* ---- Sürekli FLT seviye kontrolü (settling penceresinden SONRA) ----
       Güç-açılış glitch'i maskelendi; ama settling sonrası FLT hâlâ LOW ise
       bu gerçek bir arızadır (yeni düşen kenar üretmemiş olabilir) → safe. */
    if (loop_is_enabled() &&
        (int32_t)(now_ms - s_flt_ignore_until) >= 0 &&
        HAL_GPIO_ReadPin(LOOP_FLT_PORT, LOOP_FLT_PIN) == GPIO_PIN_RESET) {
        loop_set_safe_state();
        return;
    }

    /* ---- Sapma monitörü (yalnız enabled + alarm bölgesi dışında) ---- */
    if (!loop_is_enabled() || s_cmd_ma < LOOP_SAT_LOW_MA) {
        s_dev_over = false;
        return;
    }
    float err = s_cmd_ma - s_meas_ma;
    if (err < 0.0f) err = -err;
    bool over = (err > DEV_MAX_MA);

    if (over != s_dev_over) {         /* eşik durumu değişti → zamanı tut  */
        s_dev_over = over;
        s_dev_t    = now_ms;
    } else if ((now_ms - s_dev_t) >= DEV_TIME_MS) {
        s_deviation = over;           /* 2 s kesintisiz → bayrağı izle     */
    }
}
