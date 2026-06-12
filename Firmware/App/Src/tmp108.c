#include "tmp108.h"
#include "bsp_pins.h"
#include "stm32u3xx_hal.h"

/* -------------------------------------------------------------------------- */
/* Register pointer'ları (TMP108 datasheet §7.5, SBOS663A)                     */
/* -------------------------------------------------------------------------- */
#define REG_TEMP        0x00u
#define REG_CONFIG      0x01u
#define REG_TLOW        0x02u
#define REG_THIGH       0x03u

/* Config (16-bit, MSB first — datasheet Table 8):
 *   Byte1: ID  CR1 CR0 FH  FL  TM  M1  M0
 *   Byte2: POL 0   HYS1 HYS0 0  0  0  0
 * 0x2230 = CR=01 (1 Hz), TM=0 (comparator), M=10 (continuous),
 *          POL=0 (ALERT aktif-LOW), HYS=11 (4 °C — maks).
 * FH/FL okunur-temizlenir durum bitleri; yazımda 0.                          */
#define CONFIG_VALUE    0x2230u

/* 12-bit, 0.0625 °C/LSB, 16-bit registerda sol-hizalı ([15:4]).              */
#define C_TO_RAW(c)     ((int16_t)((c) / 0.0625f) << 4)
#define RAW_TO_C(r)     (((int16_t)(r) >> 4) * 0.0625f)

#define THIGH_RAW       ((uint16_t)C_TO_RAW(TMP108_ALERT_HIGH_C))  /* 0x3C00 */
#define TLOW_RAW        ((uint16_t)C_TO_RAW(-50.0f))   /* alt limit pasif     */

#define I2C_TIMEOUT_MS  25u

static uint8_t       s_addr7    = 0x48u;
static bool          s_ok       = false;
static float         s_amb_c    = 25.0f;
static volatile bool s_overtemp = false;

/* -------------------------------------------------------------------------- */
/* Düşük seviye I2C                                                            */
/* -------------------------------------------------------------------------- */
static bool t_write16(uint8_t reg, uint16_t val)
{
    uint8_t b[2] = { (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    return HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(s_addr7 << 1), reg,
                             I2C_MEMADD_SIZE_8BIT, b, 2, I2C_TIMEOUT_MS) == HAL_OK;
}

static bool t_read16(uint8_t reg, uint16_t *val)
{
    uint8_t b[2] = {0};
    if (HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(s_addr7 << 1), reg,
                         I2C_MEMADD_SIZE_8BIT, b, 2, I2C_TIMEOUT_MS) != HAL_OK) {
        return false;
    }
    *val = ((uint16_t)b[0] << 8) | b[1];
    return true;
}

/* -------------------------------------------------------------------------- */
/* Init                                                                        */
/* -------------------------------------------------------------------------- */
bool tmp108_init(void)
{
    /* A0 pini şemadan teyit edilene kadar 4 olası adresi tara.               */
    static const uint8_t addrs[4] = { 0x48u, 0x49u, 0x4Au, 0x4Bu };
    s_ok = false;

    for (unsigned i = 0; i < 4; i++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addrs[i] << 1),
                                  2, I2C_TIMEOUT_MS) == HAL_OK) {
            s_addr7 = addrs[i];
            s_ok = true;
            break;
        }
    }
    if (!s_ok) return false;

    /* Limitler + konfigürasyon                                               */
    if (!t_write16(REG_THIGH,  THIGH_RAW))    { s_ok = false; return false; }
    if (!t_write16(REG_TLOW,   TLOW_RAW))     { s_ok = false; return false; }
    if (!t_write16(REG_CONFIG, CONFIG_VALUE)) { s_ok = false; return false; }

    /* Read-back doğrulaması: FH/FL durum bitlerini maskeleyerek karşılaştır. */
    uint16_t rb = 0;
    if (!t_read16(REG_THIGH, &rb) || rb != THIGH_RAW) { s_ok = false; return false; }
    if (!t_read16(REG_CONFIG, &rb) ||
        (rb & ~0x1800u) != (CONFIG_VALUE & ~0x1800u)) {   /* ~FH(b12)/FL(b11) */
        s_ok = false;
        return false;
    }

    /* İlk okuma                                                              */
    uint16_t raw = 0;
    if (t_read16(REG_TEMP, &raw)) s_amb_c = RAW_TO_C(raw);
    return true;
}

/* -------------------------------------------------------------------------- */
/* Çalışma zamanı                                                              */
/* -------------------------------------------------------------------------- */
void tmp108_poll(void)
{
    if (s_ok) {
        uint16_t raw = 0;
        if (t_read16(REG_TEMP, &raw)) s_amb_c = RAW_TO_C(raw);
        else                          s_ok    = false;   /* I2C bozuldu       */
    }

    /* Comparator mode: pin histeresisle kendiliğinden kalkar. Durumu pin
       seviyesinden güncelle (EXTI yalnız hızlı set için).                    */
    s_overtemp = tmp108_alert_active();
}

float tmp108_get_ambient_c(void) { return s_amb_c; }

void tmp108_on_alert_edge(void)  { s_overtemp = true; }

bool tmp108_overtemp(void)       { return s_overtemp; }

bool tmp108_alert_active(void)
{
    return HAL_GPIO_ReadPin(FLT_TEMP_PORT, FLT_TEMP_PIN) == GPIO_PIN_RESET;
}

bool tmp108_is_ok(void)          { return s_ok; }

uint8_t tmp108_get_addr(void)    { return s_addr7; }
