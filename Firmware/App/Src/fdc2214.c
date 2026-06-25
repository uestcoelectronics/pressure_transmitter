#include "fdc2214.h"
#include "bsp_pins.h"
#include "stm32u3xx_hal.h"

/* -------------------------------------------------------------------------- */
/* Register adresleri (FDC2214 datasheet bölüm 7.6)                            */
/* -------------------------------------------------------------------------- */
#define REG_DATA_MSB_CH0        0x00
#define REG_DATA_LSB_CH0        0x01
#define REG_DATA_MSB_CH1        0x02
#define REG_DATA_LSB_CH1        0x03
#define REG_DATA_MSB_CH2        0x04
#define REG_DATA_LSB_CH2        0x05
#define REG_DATA_MSB_CH3        0x06
#define REG_DATA_LSB_CH3        0x07
#define REG_RCOUNT_CH0          0x08
#define REG_RCOUNT_CH1          0x09
#define REG_RCOUNT_CH2          0x0A
#define REG_RCOUNT_CH3          0x0B
#define REG_OFFSET_CH0          0x0C
#define REG_SETTLECOUNT_CH0     0x10
#define REG_SETTLECOUNT_CH1     0x11
#define REG_SETTLECOUNT_CH2     0x12
#define REG_SETTLECOUNT_CH3     0x13
#define REG_CLOCK_DIVIDERS_CH0  0x14
#define REG_CLOCK_DIVIDERS_CH1  0x15
#define REG_CLOCK_DIVIDERS_CH2  0x16
#define REG_CLOCK_DIVIDERS_CH3  0x17
#define REG_STATUS              0x18
#define REG_ERROR_CONFIG        0x19
#define REG_CONFIG              0x1A
#define REG_MUX_CONFIG          0x1B
#define REG_RESET_DEV           0x1C
#define REG_DRIVE_CURRENT_CH0   0x1E
#define REG_DRIVE_CURRENT_CH1   0x1F
#define REG_DRIVE_CURRENT_CH2   0x20
#define REG_DRIVE_CURRENT_CH3   0x21
#define REG_MFG_ID              0x7E
#define REG_DEV_ID              0x7F

#define DEV_ID_FDC2214          0x3055u
#define I2C_TIMEOUT_MS          50u

static volatile bool s_errflag = false;
static uint8_t       s_addr7   = FDC2214_I2C_ADDR_7B;  /* init'te tespit edilir */

/* -------------------------------------------------------------------------- */
/* Düşük seviye I2C yardımcıları                                              */
/* -------------------------------------------------------------------------- */
static bool fdc_write16(uint8_t reg, uint16_t val)
{
    uint8_t b[2] = { (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    return HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(s_addr7 << 1), reg,
                             I2C_MEMADD_SIZE_8BIT, b, 2, I2C_TIMEOUT_MS) == HAL_OK;
}

static bool fdc_read16(uint8_t reg, uint16_t *val)
{
    uint8_t b[2] = {0};
    if (HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(s_addr7 << 1), reg,
                         I2C_MEMADD_SIZE_8BIT, b, 2, I2C_TIMEOUT_MS) != HAL_OK) {
        return false;
    }
    *val = ((uint16_t)b[0] << 8) | b[1];
    return true;
}

static bool fdc_read_ch_raw28(uint8_t ch, uint32_t *out)
{
    uint16_t msb, lsb;
    uint8_t reg_msb = REG_DATA_MSB_CH0 + (ch * 2u);
    uint8_t reg_lsb = REG_DATA_LSB_CH0 + (ch * 2u);
    if (!fdc_read16(reg_msb, &msb)) return false;
    if (!fdc_read16(reg_lsb, &lsb)) return false;
    /* MSB[15:12] = error/status bits; data[27:16]=MSB[11:0], data[15:0]=LSB */
    *out = (((uint32_t)(msb & 0x0FFFu)) << 16) | (uint32_t)lsb;
    if (msb & 0xF000u) s_errflag = true;
    return true;
}

/* -------------------------------------------------------------------------- */
/* Güç / saat sıralaması                                                       */
/* -------------------------------------------------------------------------- */
void fdc2214_power_sequence(void)
{
    /* SD HIGH ile cihazı shutdown'a al (state reset), saat hazır olsun,
       sonra SD release. X403 (40 MHz XO) oturma süresi için bekleme.         */
    HAL_GPIO_WritePin(FDC_SD_PORT,     FDC_SD_PIN,     GPIO_PIN_SET);
    HAL_GPIO_WritePin(FDC_CLK_EN_PORT, FDC_CLK_EN_PIN, GPIO_PIN_SET);
    HAL_Delay(10);                                   /* XO startup            */
    HAL_GPIO_WritePin(FDC_SD_PORT,     FDC_SD_PIN,     GPIO_PIN_RESET);
    HAL_Delay(5);                                    /* cihaz wake-up         */
}

/* -------------------------------------------------------------------------- */
/* Init                                                                        */
/* -------------------------------------------------------------------------- */
bool fdc2214_init(void)
{
    fdc2214_power_sequence();

    /* DEV_ID'yi her iki olası adreste ara (ADDR pini şema teyidine kadar).  */
    uint16_t dev_id = 0;
    s_addr7 = FDC2214_I2C_ADDR_7B;
    if (!fdc_read16(REG_DEV_ID, &dev_id) || dev_id != DEV_ID_FDC2214) {
        s_addr7 = FDC2214_I2C_ADDR_ALT;
        dev_id  = 0;
        if (!fdc_read16(REG_DEV_ID, &dev_id) || dev_id != DEV_ID_FDC2214) {
            s_addr7 = FDC2214_I2C_ADDR_7B;           /* varsayılana dön      */
            return false;
        }
    }

    /* Soft reset */
    if (!fdc_write16(REG_RESET_DEV, 0x8000)) return false;
    HAL_Delay(2);

    /* Kullanılan kanallar: 2 (LP) ve 3 (HP).
     * Kanal başına konfigürasyon:
     *   RCOUNT     = 0xFFFF (max resolution; ~tipik 100 ms conversion)
     *   SETTLECOUNT= 0x000A
     *   FIN_DIV=1, FREF_DIV=1 (CLK_DIVIDERS = 0x1001)
     *   DRIVE_CURRENT — orta-yüksek (0x8C00 ≈ 0.236 mA başlangıç)
     */
    const uint16_t RCOUNT       = 0xFFFFu;
    const uint16_t SETTLECOUNT  = 0x0400u;  /* bring-up: oturma suresi artirildi (~410us) */
    const uint16_t CLKDIV       = 0x1001u;   /* single-ended → FIN_SEL=1 */
    const uint16_t DRIVE        = 0xF800u;  /* bring-up: IDRIVE=31 max — bu sensor temiz osilasyon icin gerekli (IDRIVE=20 denendi 2026-06-25: dC ±20k cop oldu, geri alindi) */

    /* CH2 */
    if (!fdc_write16(REG_RCOUNT_CH2,        RCOUNT))      return false;
    if (!fdc_write16(REG_SETTLECOUNT_CH2,   SETTLECOUNT)) return false;
    if (!fdc_write16(REG_CLOCK_DIVIDERS_CH2,CLKDIV))      return false;
    if (!fdc_write16(REG_DRIVE_CURRENT_CH2, DRIVE))       return false;
    /* CH3 */
    if (!fdc_write16(REG_RCOUNT_CH3,        RCOUNT))      return false;
    if (!fdc_write16(REG_SETTLECOUNT_CH3,   SETTLECOUNT)) return false;
    if (!fdc_write16(REG_CLOCK_DIVIDERS_CH3,CLKDIV))      return false;
    if (!fdc_write16(REG_DRIVE_CURRENT_CH3, DRIVE))       return false;

    /* MUX_CONFIG: AUTOSCAN_EN=1, RR_SEQUENCE=10b (CH0/1/2/...) → bizim için
       sadece CH2 + CH3 dolaştır: RR=11b (CH0+CH1+CH2+CH3) maalesef tek seçenek;
       FDC2214'te yalnızca {CH0,CH1}, {CH0,CH1,CH2} veya {CH0,CH1,CH2,CH3}
       sekansları var. Biz tüm dördünü tarayıp sadece 2 ve 3'ü okuyacağız.
       Bandwidth 10 MHz (DEGLITCH=101b).                                     */
    if (!fdc_write16(REG_MUX_CONFIG, 0xC20Du)) return false;

    /* ERROR_CONFIG: data-ready ve hata flag'leri INT_B'ye yönlendir         */
    if (!fdc_write16(REG_ERROR_CONFIG, 0x0001u)) return false;

    /* CONFIG: active mode, ext-clock, full current activation, INT_B enable */
    /* SLEEP_MODE_EN=0 (b13), RESERVED bitler doc'a göre 1                   */
    /* CONFIG = 0x1E01:
     *   bit 12 RP_OVERRIDE_EN=1, bit 11 SENSOR_ACTIVATE=1 (low-power),
     *   bit 10 reserved=1, bit 9 REF_CLK_SRC=1 (external X403),
     *   bit 7 INTB_DIS=0 (INT_B enabled), bit 0 reserved=1 */
    if (!fdc_write16(REG_CONFIG, 0x1601u)) return false;  /* bring-up: SENSOR_ACTIVATE_SEL=0 (full-current activation) */

    s_errflag = false;
    return true;
}

bool fdc2214_start(void)
{
    /* MUX_CONFIG'i tekrar yaz; CONFIG'i write etmek conversion'u başlatır.  */
    return fdc_write16(REG_CONFIG, 0x1681u);  /* bring-up: SENSOR_ACTIVATE_SEL=0 (full-current activation) */
}

bool fdc2214_read_ch2(uint32_t *raw) { return fdc_read_ch_raw28(2, raw); }
bool fdc2214_read_ch3(uint32_t *raw) { return fdc_read_ch_raw28(3, raw); }

bool fdc2214_read_delta(int32_t *delta)
{
    uint32_t hp, lp;
    if (!fdc_read_ch_raw28(3, &hp)) return false;  /* HP=CH3 */
    if (!fdc_read_ch_raw28(2, &lp)) return false;  /* LP=CH2 */
    *delta = (int32_t)hp - (int32_t)lp;
    return true;
}

bool fdc2214_has_error(void)   { return s_errflag; }
void fdc2214_clear_error(void) { s_errflag = false; }

uint8_t fdc2214_get_addr(void) { return s_addr7; }

bool fdc2214_poll_errb(void)
{
    if (HAL_GPIO_ReadPin(FDC_ERRB_PORT, FDC_ERRB_PIN) == GPIO_PIN_RESET) {
        /* STATUS okuması cihazdaki hata flag'lerini temizler; bayrağı
           uygulama tarafı fdc2214_clear_error() ile düşürür.                */
        uint16_t status = 0;
        (void)fdc_read16(REG_STATUS, &status);
        s_errflag = true;
        return true;
    }
    return false;
}

bool fdc2214_data_ready(void)
{
    /* ERROR_CONFIG'te DRDY → INT_B yönlendirildi (aktif-LOW).               */
    return HAL_GPIO_ReadPin(FDC_INT_PORT, FDC_INT_PIN) == GPIO_PIN_RESET;
}
