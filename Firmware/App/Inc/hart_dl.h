/**
 * hart_dl.h
 * HART veri-bağ (data-link) katmanı — çerçeve ayrıştırma + cevap kurma.
 *
 * HAL'e BAĞIMLI DEĞİL: saf C; PC'de host testiyle doğrulanabilir
 * (bkz. Test/hart_host_test.c).
 *
 * HART çerçevesi (FSK/UART 1200 bps, 8 veri + ODD parity + 1 stop):
 *   [preamble ≥5 x 0xFF][delimiter][address 1|5][cmd][byte count][data...][chk]
 *   chk = delimiter'dan son data baytına kadar XOR.
 *
 * Delimiter (alt 3 bit çerçeve tipi, bit7 adres formatı):
 *   0x02 STX (master→slave, kısa adres)   0x82 STX (uzun adres)
 *   0x06 ACK (slave→master, kısa)         0x86 ACK (uzun)
 *   0x01/0x81 BACK (burst)                — burst bu sürümde yok
 *
 * Kısa adres  (1 bayt): bit7=primary master, bit6=burst, bit3..0=poll adresi.
 * Uzun adres (5 bayt): b0(bit7 master, bit6 burst, bit5..0 = mfr_id&0x3F),
 *                      b1=device type, b2..b4=device ID (24 bit).
 */
#ifndef HART_DL_H
#define HART_DL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HART_MAX_DATA        64u   /* universal komut max ~24; pay bırakıldı */
#define HART_MAX_FRAME       (1u + 5u + 1u + 1u + HART_MAX_DATA + 1u)
#define HART_TX_PREAMBLES    5u    /* cevap preamble sayısı (min 5)          */
#define HART_GAP_TIMEOUT_MS  35u   /* >3 karakter süresi (1 kar. ≈ 9.17 ms)  */

/* Delimiter alt 3 bit */
#define HART_FT_BACK  0x01u
#define HART_FT_STX   0x02u
#define HART_FT_ACK   0x06u

typedef struct {
    bool    long_addr;      /* delimiter bit7                                */
    uint8_t frame_type;     /* HART_FT_*                                     */
    uint8_t addr[5];        /* kısa adreste yalnız addr[0] geçerli           */
    bool    primary_master; /* adres bayt0 bit7                              */
    uint8_t cmd;
    uint8_t bc;             /* data uzunluğu                                 */
    uint8_t data[HART_MAX_DATA];
} hart_frame_t;

typedef struct {
    uint8_t      state;         /* iç parser durumu                          */
    uint8_t      pre_cnt;
    uint8_t      need, got;
    uint8_t      chk;
    uint32_t     last_byte_ms;  /* gap timeout için                          */
    bool         drop;          /* parity/taşma → çerçeveyi sonuna dek yut   */
    hart_frame_t f;
} hart_dl_t;

void hart_dl_init(hart_dl_t *dl);

/* Bayt besle. true dönerse dl->f içinde GEÇERLİ (checksum OK) çerçeve var.  */
bool hart_dl_input(hart_dl_t *dl, uint8_t byte, uint32_t now_ms);

/* Karakter arası boşluk aşıldıysa parser'ı resetle. Periyodik çağır.        */
void hart_dl_gap_check(hart_dl_t *dl, uint32_t now_ms);

/* UART hata (parity/framing) bildirimi: aktif çerçeve çöpe atılır.          */
void hart_dl_mark_error(hart_dl_t *dl);

/* İsteğe ACK cevabı kur. out en az HART_TX_PREAMBLES+HART_MAX_FRAME olmalı.
 * rc = response code (status bayt 1), dev_status = status bayt 2.
 * return: toplam uzunluk (preamble dahil).                                  */
uint16_t hart_dl_build_ack(const hart_frame_t *req, uint8_t rc,
                           uint8_t dev_status, const uint8_t *data,
                           uint8_t dlen, uint8_t *out);

#ifdef __cplusplus
}
#endif
#endif /* HART_DL_H */
