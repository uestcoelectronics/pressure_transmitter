/**
 * hart_cmds.h
 * HART universal komut işleyici + cihaz veritabanı (HAL'siz, host-test edilir).
 *
 * Uygulanan komut seti (HART 5 universal + 2 common-practice):
 *   0  Read Unique Identifier          11 Read Unique ID by Tag
 *   1  Read PV                         12 Read Message
 *   2  Read Loop Current & % of Range  13 Read Tag/Descriptor/Date
 *   3  Read Dynamic Variables          14 Read PV Transducer Info
 *   6  Write Polling Address           15 Read Output Info (range/damping)
 *   16 Read Final Assembly Number      17 Write Message
 *   18 Write Tag/Descriptor/Date       19 Write Final Assembly Number
 *   38 Reset Config-Changed Flag (CP)  48 Read Additional Status (CP)
 *
 * Değişken eşlemesi (bu cihaz):
 *   PV = basınç [bar, unit 7]   (kalibre edilmiş, damping'li değer)
 *   SV = proses sıcaklığı [°C, unit 32] (1N4148 kompanzasyon diyotları)
 *   TV = kart sıcaklığı  [°C, unit 32] (TMP108)
 *   Loop akımı = INA190'dan ÖLÇÜLEN mA (komut edilen değil — dürüst raporlama)
 *
 * NOT (kalıcılık): tag/message/poll adresi bu sürümde RAM'de tutulur;
 * güç kesilince default'a döner. Kalıcılık için cal_storage v3 genişlemesi
 * gerekir (bkz. HART_INTEGRATION.md, "Açık işler").
 */
#ifndef HART_CMDS_H
#define HART_CMDS_H

#include "hart_dl.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HART response code'ları (status bayt 1)                                   */
#define HART_RC_OK               0u
#define HART_RC_INVALID_SEL      2u
#define HART_RC_TOO_FEW_BYTES    5u
#define HART_RC_NOT_IMPLEMENTED  64u

/* Field device status bitleri (status bayt 2)                               */
#define HART_DS_MALFUNCTION      0x80u
#define HART_DS_CFG_CHANGED      0x40u
#define HART_DS_COLD_START       0x20u
#define HART_DS_MORE_STATUS      0x10u
#define HART_DS_LOOP_FIXED       0x08u
#define HART_DS_LOOP_SATURATED   0x04u
#define HART_DS_NONPV_OOL        0x02u
#define HART_DS_PV_OOL           0x01u

/* HART Common Table birim kodları (kullanılanlar)                           */
#define HART_UNIT_BAR            7u
#define HART_UNIT_CELSIUS        32u

typedef struct {
    /* Kimlik — FieldComm Group kayıtlı ID alınana kadar geçici değerler     */
    uint8_t mfr_id;         /* 250 = private-label/kayıtsız (geçici)         */
    uint8_t dev_type;
    uint8_t dev_id[3];      /* seri numarasından türetilecek                 */
    uint8_t poll_addr;      /* 0..15 (HART5); cmd 6 ile yazılır              */
    uint8_t min_preambles;  /* master'dan istenen preamble (5)               */
    uint8_t univ_rev;       /* 5 = HART 5 komut seti                         */
    uint8_t dev_rev, sw_rev, hw_rev, flags;

    /* Packed-ASCII/metin alanları (cmd 12/13/17/18)                         */
    uint8_t tag[6];         /* 8 karakter packed                             */
    uint8_t descriptor[12]; /* 16 karakter packed                            */
    uint8_t date[3];        /* gün, ay, yıl-1900                             */
    uint8_t message[24];    /* 32 karakter packed                            */
    uint8_t final_asm[3];

    /* Sensör bilgisi (cmd 14) — ürün limitlerine göre doldur                */
    uint8_t sensor_sn[3];
    float   usl_bar, lsl_bar, min_span_bar;

    /* Durum bayrakları                                                      */
    bool    cold_start;     /* boot'ta true; ilk başarılı cevapla düşer      */
    bool    cfg_changed;    /* yazma komutlarıyla set; cmd 38 ile temizlenir */
} hart_db_t;

/* Anlık ölçümler + durum — pressure_app her serviste doldurur               */
typedef struct {
    float pv_bar;           /* damping'li, kalibre basınç                    */
    float sv_temp_c;        /* proses sıcaklığı (temp_diode)                 */
    float tv_temp_c;        /* kart sıcaklığı (tmp108)                       */
    float loop_ma;          /* INA190 ölçülen loop akımı                     */
    float pct_range;        /* (PV-LRV)/(URV-LRV)*100                        */
    float lrv_bar, urv_bar; /* cal'dan: p_min / p_max                        */
    float damping_s;
    bool  malfunction;      /* cihaz arızası (diag/loop fault)               */
    bool  loop_saturated;   /* NAMUR satürasyon aktif                        */
    bool  pv_out_of_limits;
    uint8_t extra_status;   /* cmd 48 ilk baytı (bstat)                      */
} hart_live_t;

/* Varsayılan kimlik + alanlarla doldur.                                     */
void hart_db_init(hart_db_t *db);

/* Bu çerçeve bize mi? (kısa: poll adresi; uzun: mfr/type/id; cmd 11: tag)   */
bool hart_addr_match(const hart_db_t *db, const hart_frame_t *f);

/* Komutu işle. return: cevap gönderilecek mi (cmd 11 tag uyuşmazsa false).
 * rc/data/dlen çıkışları hart_dl_build_ack'e verilir.                       */
bool hart_cmds_handle(hart_db_t *db, const hart_frame_t *req,
                      const hart_live_t *live, uint8_t *rc,
                      uint8_t *data, uint8_t *dlen);

/* Status bayt 2'yi üret.                                                    */
uint8_t hart_device_status(const hart_db_t *db, const hart_live_t *live);

/* Yardımcılar (host testinde de kullanılır)                                 */
void hart_pack_ascii(const char *src, uint8_t *dst, uint8_t n_packed);
void hart_unpack_ascii(const uint8_t *src, char *dst, uint8_t n_packed);
void hart_put_f32(uint8_t *p, float v);   /* IEEE754 big-endian              */

#ifdef __cplusplus
}
#endif
#endif /* HART_CMDS_H */
