#include "diag.h"
#include "bsp_pins.h"
#include "i2c.h"            /* MX_I2C1_Init */
#include "stm32u3xx_hal.h"

/* -------------------------------------------------------------------------- */
/* Eşikler                                                                     */
/* -------------------------------------------------------------------------- */
/* ADC rail-stuck: ham değer bu sınırların dışındaysa kanal kopuk/mux arızası.
   Divider-bağımsız (mutlak gerilim değil). 12-bit (0..4095).                 */
#define ADC_STUCK_LOW       8u
#define ADC_STUCK_HIGH      (4095u - 8u)

/* GPIO read-back uyuşmazlığı kaç ardışık örnekte bayrak olsun (transient ele) */
#define RBACK_PERSIST       2u

/* I2C1 pinleri (CubeMX: PB8=SCL, PB9=SDA, AF4 open-drain) */
#define I2C_SCL_PIN         GPIO_PIN_8
#define I2C_SDA_PIN         GPIO_PIN_9
#define I2C_GPIO_PORT       GPIOB
#define I2C_AF              GPIO_AF4_I2C1

static uint16_t s_flags     = 0;
static uint8_t  s_rback_cnt = 0;     /* LOOP_EN (kritik) uyuşmazlık sayacı   */
static bool     s_loopen_bad= false;

/* ~birkaç µs gecikme (clock pals genişliği için) */
static void short_delay(void)
{
    for (volatile uint32_t i = 0; i < 200u; ++i) { __NOP(); }
}

/* Output GPIO read-back: komut (ODR) ile gerçek pin (IDR) eşleşiyor mu? */
static bool gpio_rback_ok(GPIO_TypeDef *port, uint16_t pin)
{
    uint32_t commanded = (port->ODR & pin) ? 1u : 0u;
    uint32_t actual    = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) ? 1u : 0u;
    return commanded == actual;
}

void diag_init(void)
{
    s_flags      = 0;
    s_rback_cnt  = 0;
    s_loopen_bad = false;
}

void diag_service(uint16_t adc_vcc_raw, uint16_t adc_iloop_raw)
{
    uint16_t f = 0;

    /* --- A.13: ADC kanal rail-stuck --- */
    if (adc_vcc_raw <= ADC_STUCK_LOW || adc_vcc_raw >= ADC_STUCK_HIGH) {
        f |= DIAG_F_ADC_VCC;
    }
    if (adc_iloop_raw <= ADC_STUCK_LOW || adc_iloop_raw >= ADC_STUCK_HIGH) {
        f |= DIAG_F_ADC_ILOOP;
    }

    /* --- A.7: output GPIO read-back --- */
    bool rb_ok = gpio_rback_ok(LCD_PWR_PORT,    LCD_PWR_PIN)    &&
                 gpio_rback_ok(BLE_PWR_ON_PORT, BLE_PWR_ON_PIN) &&
                 gpio_rback_ok(FDC_CLK_EN_PORT, FDC_CLK_EN_PIN);
    bool loopen_ok = gpio_rback_ok(LOOP_EN_PORT, LOOP_EN_PIN);

    /* LOOP_EN güvenlik-kritik: kalıcılıkla teyit et */
    if (!loopen_ok) {
        if (s_rback_cnt < RBACK_PERSIST) s_rback_cnt++;
    } else {
        s_rback_cnt = 0;
    }
    s_loopen_bad = (s_rback_cnt >= RBACK_PERSIST);

    if (!rb_ok || s_loopen_bad) f |= DIAG_F_GPIO_RBACK;

    /* I2C bus bayrağı kalıcı (kurtarma denendiğinde set; diag_init'e dek tutar) */
    s_flags = (uint16_t)(f | (s_flags & DIAG_F_I2C_BUS));
}

uint16_t diag_flags(void)  { return s_flags; }
bool     diag_any(void)    { return s_flags != 0; }
bool     diag_critical(void) { return s_loopen_bad; }

/* -------------------------------------------------------------------------- */
/* I2C bus recovery — 9 clock + STOP + reinit                                  */
/* -------------------------------------------------------------------------- */
void diag_i2c_bus_recover(void)
{
    GPIO_InitTypeDef gi = {0};

    HAL_I2C_DeInit(&hi2c1);

    /* SCL + SDA'yı open-drain GPIO output yap, ikisini de yükselt */
    gi.Mode  = GPIO_MODE_OUTPUT_OD;
    gi.Pull  = GPIO_NOPULL;
    gi.Speed = GPIO_SPEED_FREQ_LOW;
    gi.Pin   = I2C_SCL_PIN | I2C_SDA_PIN;
    HAL_GPIO_Init(I2C_GPIO_PORT, &gi);
    HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    short_delay();

    /* SDA takılı low ise 9 clock ile slave'i serbest bırak */
    for (int i = 0; i < 9; ++i) {
        if (HAL_GPIO_ReadPin(I2C_GPIO_PORT, I2C_SDA_PIN) == GPIO_PIN_SET) break;
        HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
        short_delay();
        HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
        short_delay();
    }

    /* STOP koşulu: SCL high iken SDA low→high */
    HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
    short_delay();
    HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    short_delay();
    HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    short_delay();

    /* Pinleri AF4 (I2C1) open-drain'e geri al + HAL I2C reinit */
    gi.Mode      = GPIO_MODE_AF_OD;
    gi.Pull      = GPIO_NOPULL;
    gi.Speed     = GPIO_SPEED_FREQ_LOW;
    gi.Alternate = I2C_AF;
    gi.Pin       = I2C_SCL_PIN | I2C_SDA_PIN;
    HAL_GPIO_Init(I2C_GPIO_PORT, &gi);

    MX_I2C1_Init();

    s_flags |= DIAG_F_I2C_BUS;
}
