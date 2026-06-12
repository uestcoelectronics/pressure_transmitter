#include "cal_storage.h"
#include "stm32u3xx_hal.h"
#include <string.h>

/* -------------------------------------------------------------------------- */
/* Flash sayfası seçimi                                                        */
/* -------------------------------------------------------------------------- */
/* STM32U385RG: 1 MB flash, 0x0800_0000..0x080F_FFFF, 8 KB sayfa => 128 sayfa */
/* Son sayfayı kullanıyoruz.                                                   */
#define CAL_PAGE_INDEX     127u
#define CAL_FLASH_ADDR     (0x08000000u + (CAL_PAGE_INDEX * 0x2000u))

/* Header tanımı — sondaki CRC dahil hizalı kalsın diye doubleword multiple   */
typedef struct __attribute__((packed)) {
    uint32_t      magic;
    uint32_t      version;
    cal_params_t  params;
    uint32_t      crc32;
} cal_record_t;

/* RAM'deki aktif parametreler */
static cal_params_t s_cal;

/* -------------------------------------------------------------------------- */
/* CRC32 (poly 0xEDB88320, software — küçük tablo bile gerek yok)             */
/* -------------------------------------------------------------------------- */
static uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    while (len--) {
        crc ^= *data++;
        for (int i = 0; i < 8; ++i) {
            uint32_t mask = -((int32_t)(crc & 1u));
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

/* -------------------------------------------------------------------------- */
/* Defaults                                                                    */
/* -------------------------------------------------------------------------- */
void cal_load_defaults(void)
{
    s_cal.cap_at_zero = 0;
    s_cal.cap_at_span = 1000000;     /* yer tutucu — kalibrasyona kadar */
    s_cal.p_min       = 0.0f;
    s_cal.p_max       = 10.0f;       /* default 0-10 bar */
    s_cal.k_t         = 0.0f;        /* devre dışı */
    s_cal.t_ref       = 25.0f;
    s_cal.damping_s   = 0.5f;
}

/* -------------------------------------------------------------------------- */
/* Init: flash'tan oku, CRC tut yoksa default yükle                            */
/* -------------------------------------------------------------------------- */
void cal_init(void)
{
    const cal_record_t *rec = (const cal_record_t*)CAL_FLASH_ADDR;

    if (rec->magic == CAL_MAGIC && rec->version == CAL_VERSION) {
        uint32_t crc = crc32_calc((const uint8_t*)&rec->params, sizeof(cal_params_t));
        if (crc == rec->crc32) {
            s_cal = rec->params;
            return;
        }
    }
    cal_load_defaults();
}

const cal_params_t* cal_get(void)         { return &s_cal; }
cal_params_t*       cal_get_mutable(void) { return &s_cal; }

/* -------------------------------------------------------------------------- */
/* Save: sayfayı sil + record'u programla (doubleword birimleriyle)            */
/* -------------------------------------------------------------------------- */
bool cal_save(void)
{
    cal_record_t rec;
    memset(&rec, 0, sizeof rec);
    rec.magic   = CAL_MAGIC;
    rec.version = CAL_VERSION;
    rec.params  = s_cal;
    rec.crc32   = crc32_calc((const uint8_t*)&rec.params, sizeof(cal_params_t));

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef er = {0};
    er.TypeErase = FLASH_TYPEERASE_PAGES;
    er.Page      = CAL_PAGE_INDEX;
    er.NbPages   = 1;
    /* Bank seçimi STM32U3'te tek bank organizasyonunda kullanılmıyor;
       gerekirse er.Banks = FLASH_BANK_1 ekleyin. */

    uint32_t page_err = 0;
    if (HAL_FLASHEx_Erase(&er, &page_err) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }

    /* Double-word (8 byte) program — STM32U3 HAL yalnız DOUBLEWORD/BURST    */
    /* destekler (QUADWORD U5 ailesine ait)                                  */
    uint32_t addr = CAL_FLASH_ADDR;

    /* Yuvarlat: 8'in katı olacak şekilde hizalı tampona kopyala (kalan 0xFF)*/
    uint64_t buf[(sizeof(cal_record_t) + 7u) / 8u];
    memset(buf, 0xFF, sizeof buf);
    memcpy(buf, &rec, sizeof rec);

    for (uint32_t i = 0; i < sizeof(buf); i += 8) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                              addr + i,
                              (uint32_t)((const uint8_t*)buf + i)) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
    }

    HAL_FLASH_Lock();
    return true;
}
