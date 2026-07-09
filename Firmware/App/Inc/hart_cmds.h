/**
 * hart_cmds.h
 * HART universal komut işleyici + cihaz veritabanı (HAL'siz, host-test edilir).
 *
 * Uygulanan komut seti (HART 7 universal):
 *   0  Read Unique Identifier          12 Read Message
 *   1  Read PV                         13 Read Tag/Descriptor/Date
 *   2  Read Loop Current & % of Range  14 Read PV Transducer Info
 *   3  Read Dynamic Variables          15 Read Output Info (range/damping)
 *   6  Write Polling Address (0..63)   16 Read Final Assembly Number
 *   7  Read Loop Configuration         17 Write Message
 *   8  Read Dyn. Var. Classifications  18 Write Tag/Descriptor/Date
 *   9  Read Dev. Vars with Status      19 Write Final Assembly Number
 *   11 Read Unique ID by Tag           20 Read Long Tag
 *   21 Read Unique ID by Long Tag      22 Write Long Tag
 *   38 Reset Cfg-Changed (counter'lı)  48 Read Additional Status
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
#define HART_RC_CTR_MISMATCH     9u   /* cmd 38: cfg-counter uyuşmadı      */
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
#define HART_UNIT_MA             39u
#define HART_UNIT_PERCENT        57u
#define HART_UNIT_NOT_USED       250u

/* HART 7 device-variable kodları (Common Tables)                            */
#define HART_DV_PCT_RANGE        244u
#define HART_DV_LOOP_MA          245u
#define HART_DV_PV               246u
#define HART_DV_SV               247u
#define HART_DV_TV               248u
#define HART_DV_QV               249u

/* Device-variable classification (Common Table 21; üründe teyit edilecek)   */
#define HART_CLASS_NONE          0u
#define HART_CLASS_TEMPERATURE   64u
#define HART_CLASS_PRESSURE      65u

/* Device-variable status: bit7..6 kalite                                    */
#define HART_DVSTAT_GOOD         0xC0u
#define HART_DVSTAT_BAD          0x00u

typedef struct {
    /* Kimlik — FieldComm Group kayıtlı ID alınana kadar geçici değerler     */
    uint8_t mfr_id;         /* 250 = private-label/kayıtsız (geçici)         */
    uint8_t dev_type;
    uint8_t dev_id[3];      /* seri numarasından türetilecek                 */
    uint8_t poll_addr;      /* 0..63 (HART7); cmd 6 ile yazılır              */
    uint8_t min_preambles;  /* master'dan istenen preamble (5)               */
    uint8_t univ_rev;       /* 7 = HART 7 komut seti                         */
    uint8_t dev_rev, sw_rev, hw_rev, flags;

    /* HART 7 kimlik/durum alanları                                          */
    uint16_t dev_type16;        /* expanded device type (cmd0 b1-2)          */
    uint16_t mfr_id16;          /* 16-bit üretici kodu (cmd0 b17-18)         */
    uint16_t priv_label16;      /* private-label distributor (cmd0 b19-20)   */
    uint8_t  device_profile;    /* cmd0 b21 (Common Table 57)                */
    uint8_t  max_dev_vars;      /* cmd0 b13                                  */
    uint16_t cfg_change_count;  /* her konfig yazımında ++ (cmd0 b14-15)     */
    uint8_t  ext_dev_status;    /* extended field device status (cmd0 b16)   */
    uint8_t  loop_current_mode; /* 0=multidrop(sabit) 1=aktif (cmd 6/7)      */
    char     long_tag[32];      /* ISO Latin-1, cmd 20/21/22 (RAM — TODO v3) */

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

/* HART zaman damgası (cmd 9): hart_service her turda günceller.             */
void hart_cmds_set_time(uint32_t now_ms);

/* Yardımcılar (host testinde de kullanılır)                                 */
void hart_pack_ascii(const char *src, uint8_t *dst, uint8_t n_packed);
void hart_unpack_ascii(const uint8_t *src, char *dst, uint8_t n_packed);
void hart_put_f32(uint8_t *p, float v);   /* IEEE754 big-endian              */

#ifdef __cplusplus
}
#endif
#endif /* HART_CMDS_H */
