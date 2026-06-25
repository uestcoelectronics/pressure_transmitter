#include "cal_storage.h"
#include "stm32u3xx_hal.h"
#include <string.h>

/* Watchdog beslemesi (pressure_app.c'de tanımlı) — flash erase/program ana
   döngüyü bloklar; uzun işlem sırasında harici WDT+IWDG starvation olmasın.
   Katman bağımsızlığı için header include yerine extern bildirimi.          */
extern void wdt_feed_raw(void);

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

/* v1 payload düzeni (migrasyon için dondurulmuş kopya — DEĞİŞTİRME)          */
typedef struct __attribute__((packed)) {
    int32_t  cap_at_zero;
    int32_t  cap_at_span;
    float    p_min;
    float    p_max;
    float    k_t;              /* v2'de k_t_zero oldu */
    float    t_ref;
    float    damping_s;
} cal_params_v1_t;

typedef struct __attribute__((packed)) {
    uint32_t         magic;
    uint32_t         version;
    cal_params_v1_t  params;
    uint32_t         crc32;
} cal_record_v1_t;

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
    s_cal.k_t_zero    = 0.0f;        /* devre dışı */
    s_cal.t_ref       = 25.0f;
    s_cal.damping_s   = 0.5f;
    s_cal.k_t_span    = 0.0f;        /* devre dışı */
    s_cal.vf25_mv     = 600.0f;      /* 1N4148 @ ~100 µA */
    s_cal.tc_mv_c     = -2.0f;
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

    /* v1 → v2 migrasyonu: eski kayıt geçerliyse alanları taşı, yenileri
       default bırak. (Bir sonraki Save & Exit v2 olarak yazar.)             */
    const cal_record_v1_t *rec1 = (const cal_record_v1_t*)CAL_FLASH_ADDR;
    if (rec1->magic == CAL_MAGIC && rec1->version == 1u) {
        uint32_t crc = crc32_calc((const uint8_t*)&rec1->params,
                                  sizeof(cal_params_v1_t));
        if (crc == rec1->crc32) {
            cal_load_defaults();                 /* yeni alanlar default     */
            s_cal.cap_at_zero = rec1->params.cap_at_zero;
            s_cal.cap_at_span = rec1->params.cap_at_span;
            s_cal.p_min       = rec1->params.p_min;
            s_cal.p_max       = rec1->params.p_max;
            s_cal.k_t_zero    = rec1->params.k_t;
            s_cal.t_ref       = rec1->params.t_ref;
            s_cal.damping_s   = rec1->params.damping_s;
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

    FLASH_EraseInitTypeDef er = {0};
    er.TypeErase = FLASH_TYPEERASE_PAGES;
    er.Page      = CAL_PAGE_INDEX;
    er.NbPages   = 1;
    /* Bank seçimi STM32U3'te tek bank organizasyonunda kullanılmıyor.        */

    /* STM32U3 flash ECC'si 128-bit (quad-word) granülaritede: bir 128-bit
       kelimenin yalnız bir doubleword'ünü yazıp diğerini boş bırakmak son
       yazımda PGSERR üretir. Tamponu 16 baytın (2 doubleword) katına yuvarla
       → her 128-bit kelime tam yazılır, kalan 0xFF (erased) ile doldurulur.  */
    uint64_t buf[((sizeof(cal_record_t) + 15u) / 16u) * 2u];
    memset(buf, 0xFF, sizeof buf);
    memcpy(buf, &rec, sizeof rec);

    /* Erase+program+geri-okuma doğrulamasını en çok 3 kez dene. Sistematik
       sahte PGSERR 16-bayt doldurmayla giderildi; bu döngü marjinal besleme
       (VDD ~2.8V) kaynaklı KALAN aralıklı yazım hatalarını yutar. Tek güvenilir
       başarı ölçütü = geri okuma (HAL bayrağına değil fiilî içeriğe güven).   */
    for (uint32_t attempt = 0; attempt < 3u; ++attempt) {
        HAL_FLASH_Unlock();
        /* Önceki işlem/denemeden kalmış hata bayraklarını temizle — yoksa
           WaitForLastOperation sahte hata döndürebilir.                       */
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_SR_ERRORS | FLASH_FLAG_EOP);

        wdt_feed_raw();                       /* uzun erase öncesi besle       */
        uint32_t page_err = 0;
        if (HAL_FLASHEx_Erase(&er, &page_err) == HAL_OK) {
            for (uint32_t i = 0; i < sizeof(buf); i += 8) {
                (void)HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                                        CAL_FLASH_ADDR + i,
                                        (uint32_t)((const uint8_t*)buf + i));
            }
        }
        HAL_FLASH_Lock();

        if (memcmp((const void*)CAL_FLASH_ADDR, buf, sizeof buf) == 0) {
            return true;
        }
    }
    return false;
}
