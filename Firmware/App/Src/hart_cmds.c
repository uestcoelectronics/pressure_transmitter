/**
 * hart_cmds.c — HART universal komut işleyici (HAL'siz).
 * Komut listesi ve veri eşlemesi için hart_cmds.h başlığına bak.
 */
#include "hart_cmds.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* Yardımcılar                                                          */
/* ------------------------------------------------------------------ */

void hart_put_f32(uint8_t *p, float v)
{
    union { float f; uint32_t u; } c;
    c.f = v;
    p[0] = (uint8_t)(c.u >> 24);
    p[1] = (uint8_t)(c.u >> 16);
    p[2] = (uint8_t)(c.u >> 8);
    p[3] = (uint8_t)(c.u);
}

/* Packed ASCII: 6-bit karakterler, 4 karakter → 3 bayt.
 * Kodlama: ASCII 0x40..0x5F → 0x00..0x1F, 0x20..0x3F → 0x20..0x3F.        */
static uint8_t pack6(char c)
{
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (c < 0x20 || c > 0x5F) c = ' ';
    return (uint8_t)(c & 0x3Fu);
}

void hart_pack_ascii(const char *src, uint8_t *dst, uint8_t n_packed)
{
    uint8_t i, si = 0, len = (uint8_t)strlen(src);
    for (i = 0; i < n_packed; i += 3u) {
        uint8_t c0 = pack6(si < len ? src[si] : ' '); si++;
        uint8_t c1 = pack6(si < len ? src[si] : ' '); si++;
        uint8_t c2 = pack6(si < len ? src[si] : ' '); si++;
        uint8_t c3 = pack6(si < len ? src[si] : ' '); si++;
        dst[i]      = (uint8_t)((c0 << 2) | (c1 >> 4));
        dst[i + 1u] = (uint8_t)((c1 << 4) | (c2 >> 2));
        dst[i + 2u] = (uint8_t)((c2 << 6) | c3);
    }
}

void hart_unpack_ascii(const uint8_t *src, char *dst, uint8_t n_packed)
{
    uint8_t i, di = 0;
    for (i = 0; i < n_packed; i += 3u) {
        uint8_t c[4];
        c[0] = (uint8_t)(src[i] >> 2);
        c[1] = (uint8_t)(((src[i] & 0x03u) << 4) | (src[i + 1u] >> 4));
        c[2] = (uint8_t)(((src[i + 1u] & 0x0Fu) << 2) | (src[i + 2u] >> 6));
        c[3] = (uint8_t)(src[i + 2u] & 0x3Fu);
        for (uint8_t k = 0; k < 4u; k++)
            dst[di++] = (char)(c[k] < 0x20u ? c[k] + 0x40u : c[k]);
    }
    dst[di] = '\0';
}

/* ------------------------------------------------------------------ */
/* Cihaz veritabanı                                                     */
/* ------------------------------------------------------------------ */

void hart_db_init(hart_db_t *db)
{
    memset(db, 0, sizeof(*db));
    db->mfr_id        = 250u;  /* kayıtsız/örnek — FieldComm ID alınmalı    */
    db->dev_type      = 0x01u; /* PT910 ailesi için seçilecek               */
    db->dev_id[0]     = 0x00u; /* üretimde seri numarasından yazılacak      */
    db->dev_id[1]     = 0x00u;
    db->dev_id[2]     = 0x01u;
    db->poll_addr     = 0u;
    db->min_preambles = 5u;
    db->univ_rev      = 7u;    /* HART 7 komut seti                         */
    db->dev_rev       = 1u;
    db->sw_rev        = 1u;
    db->hw_rev        = 1u;
    db->flags         = 0u;

    /* HART 7: expanded device type, uzun adresle bit-uyumlu kurulur:
     * addr b0(5..0) = dev_type16 bit13..8, addr b1 = dev_type16 bit7..0.
     * Eski (mfr_id&0x3F)<<8 | dev_type seçimi adresi DEĞİŞTİRMEZ.           */
    db->dev_type16        = (uint16_t)(((uint16_t)(db->mfr_id & 0x3Fu) << 8) | db->dev_type);
    db->mfr_id16          = db->mfr_id;   /* FieldComm 16-bit kod alınınca güncelle */
    db->priv_label16      = db->mfr_id;   /* private-label yok → üretici     */
    db->device_profile    = 65u;          /* Common Table 57: process automation (teyit et) */
    db->max_dev_vars      = 3u;           /* PV, SV, TV                      */
    db->cfg_change_count  = 0u;
    db->ext_dev_status    = 0u;
    db->loop_current_mode = 1u;           /* poll 0 → loop aktif             */
    memset(db->long_tag, 0, sizeof db->long_tag);
    memcpy(db->long_tag, "PT910 PRESSURE TRANSMITTER", 26);

    hart_pack_ascii("PT910",  db->tag,        sizeof db->tag);
    hart_pack_ascii("PRESSURE XMTR", db->descriptor, sizeof db->descriptor);
    db->date[0] = 1u; db->date[1] = 7u; db->date[2] = (uint8_t)(2026 - 1900);
    hart_pack_ascii("PROTOTYPE", db->message, sizeof db->message);

    /* Sensör limitleri — ÜRÜN SPEC'İNE GÖRE GÜNCELLENMELİ                  */
    db->usl_bar      = 10.0f;
    db->lsl_bar      = 0.0f;
    db->min_span_bar = 0.5f;

    db->cold_start  = true;
    db->cfg_changed = false;
}

/* Konfig değişikliği: bayrak + HART7 16-bit sayaç (cmd0/38'de görünür)  */
static void cfg_mark(hart_db_t *db)
{
    db->cfg_changed = true;
    db->cfg_change_count++;
}

/* cmd 9 zaman damgası kaynağı (1/32 ms birimi; hart_service besler)     */
static uint32_t s_time_ms;
void hart_cmds_set_time(uint32_t now_ms) { s_time_ms = now_ms; }

/* ------------------------------------------------------------------ */
/* Adres eşleme                                                         */
/* ------------------------------------------------------------------ */

static bool long_addr_is_broadcast(const hart_frame_t *f)
{
    return ((f->addr[0] & 0x3Fu) == 0u) && f->addr[1] == 0u &&
           f->addr[2] == 0u && f->addr[3] == 0u && f->addr[4] == 0u;
}

bool hart_addr_match(const hart_db_t *db, const hart_frame_t *f)
{
    if (!f->long_addr) {
        return (uint8_t)(f->addr[0] & 0x3Fu) == db->poll_addr;
    }
    if (long_addr_is_broadcast(f)) {
        return f->cmd == 11u || f->cmd == 21u;   /* ID by (long) tag        */
    }
    return (uint8_t)(f->addr[0] & 0x3Fu) == (uint8_t)(db->mfr_id & 0x3Fu) &&
           f->addr[1] == db->dev_type &&
           f->addr[2] == db->dev_id[0] &&
           f->addr[3] == db->dev_id[1] &&
           f->addr[4] == db->dev_id[2];
}

/* ------------------------------------------------------------------ */
/* Status bayt 2                                                        */
/* ------------------------------------------------------------------ */

uint8_t hart_device_status(const hart_db_t *db, const hart_live_t *live)
{
    uint8_t s = 0;
    if (live->malfunction)      s |= HART_DS_MALFUNCTION;
    if (db->cfg_changed)        s |= HART_DS_CFG_CHANGED;
    if (db->cold_start)         s |= HART_DS_COLD_START;
    if (live->extra_status)     s |= HART_DS_MORE_STATUS;   /* cmd 48'e bak */
    if (live->loop_saturated)   s |= HART_DS_LOOP_SATURATED;
    if (live->pv_out_of_limits) s |= HART_DS_PV_OOL;
    return s;
}

/* ------------------------------------------------------------------ */
/* Komut işleyiciler                                                    */
/* ------------------------------------------------------------------ */

static uint8_t cmd0_payload(const hart_db_t *db, uint8_t *d)
{
    d[0]  = 254u;               /* sabit "expansion" baytı                  */
    d[1]  = (uint8_t)(db->dev_type16 >> 8);   /* expanded device type       */
    d[2]  = (uint8_t)(db->dev_type16);
    d[3]  = db->min_preambles;  /* master→slave min preamble                */
    d[4]  = db->univ_rev;       /* = 7                                      */
    d[5]  = db->dev_rev;
    d[6]  = db->sw_rev;
    d[7]  = db->hw_rev;         /* bit0..2 fiziksel sinyal kodu = 0 (FSK)   */
    d[8]  = db->flags;
    d[9]  = db->dev_id[0];
    d[10] = db->dev_id[1];
    d[11] = db->dev_id[2];
    d[12] = db->min_preambles;  /* slave→master min preamble                */
    d[13] = db->max_dev_vars;
    d[14] = (uint8_t)(db->cfg_change_count >> 8);
    d[15] = (uint8_t)(db->cfg_change_count);
    d[16] = db->ext_dev_status;
    d[17] = (uint8_t)(db->mfr_id16 >> 8);
    d[18] = (uint8_t)(db->mfr_id16);
    d[19] = (uint8_t)(db->priv_label16 >> 8);
    d[20] = (uint8_t)(db->priv_label16);
    d[21] = db->device_profile;
    return 22u;
}

/* cmd 9 tek slot doldur: 8 bayt (kod, sınıf, birim, f32, durum)            */
static void dv_slot(uint8_t *d, uint8_t code, const hart_live_t *live)
{
    uint8_t cls = HART_CLASS_NONE, unit = HART_UNIT_NOT_USED;
    uint8_t st  = HART_DVSTAT_GOOD;
    float   v   = 0.0f;
    switch (code) {
    case HART_DV_PV:        cls = HART_CLASS_PRESSURE;    unit = HART_UNIT_BAR;     v = live->pv_bar;    break;
    case HART_DV_SV:        cls = HART_CLASS_TEMPERATURE; unit = HART_UNIT_CELSIUS; v = live->sv_temp_c; break;
    case HART_DV_TV:        cls = HART_CLASS_TEMPERATURE; unit = HART_UNIT_CELSIUS; v = live->tv_temp_c; break;
    case HART_DV_LOOP_MA:   unit = HART_UNIT_MA;      v = live->loop_ma;   break;
    case HART_DV_PCT_RANGE: unit = HART_UNIT_PERCENT; v = live->pct_range; break;
    default:                                          /* desteklenmeyen kod */
        st = HART_DVSTAT_BAD;
        { union { float f; uint32_t u; } nan; nan.u = 0x7FA00000u; v = nan.f; }
        break;
    }
    if (live->malfunction && st == HART_DVSTAT_GOOD) st = HART_DVSTAT_BAD;
    d[0] = code; d[1] = cls; d[2] = unit;
    hart_put_f32(&d[3], v);
    d[7] = st;
}

bool hart_cmds_handle(hart_db_t *db, const hart_frame_t *req,
                      const hart_live_t *live, uint8_t *rc,
                      uint8_t *data, uint8_t *dlen)
{
    *rc = HART_RC_OK;
    *dlen = 0;

    switch (req->cmd) {

    case 0:                                 /* Read Unique Identifier       */
        *dlen = cmd0_payload(db, data);
        break;

    case 1:                                 /* Read Primary Variable        */
        data[0] = HART_UNIT_BAR;
        hart_put_f32(&data[1], live->pv_bar);
        *dlen = 5u;
        break;

    case 2:                                 /* Read Loop Current + % Range  */
        hart_put_f32(&data[0], live->loop_ma);
        hart_put_f32(&data[4], live->pct_range);
        *dlen = 8u;
        break;

    case 3:                                 /* Read Dynamic Variables       */
        hart_put_f32(&data[0], live->loop_ma);
        data[4] = HART_UNIT_BAR;      hart_put_f32(&data[5],  live->pv_bar);
        data[9] = HART_UNIT_CELSIUS;  hart_put_f32(&data[10], live->sv_temp_c);
        data[14] = HART_UNIT_CELSIUS; hart_put_f32(&data[15], live->tv_temp_c);
        *dlen = 19u;                        /* 3 dinamik değişkenli cihaz   */
        break;

    case 6:                                 /* Write Polling Address        */
        if (req->bc < 1u)            { *rc = HART_RC_TOO_FEW_BYTES; break; }
        if (req->data[0] > 63u)      { *rc = HART_RC_INVALID_SEL;   break; }
        if (req->bc >= 2u && req->data[1] > 1u) { *rc = HART_RC_INVALID_SEL; break; }
        db->poll_addr = req->data[0];
        if (req->bc >= 2u) db->loop_current_mode = req->data[1];
        else               db->loop_current_mode = (req->data[0] == 0u) ? 1u : 0u;
        cfg_mark(db);
        data[0] = db->poll_addr;
        data[1] = db->loop_current_mode;
        *dlen = 2u;
        break;

    case 7:                                 /* Read Loop Configuration      */
        data[0] = db->poll_addr;
        data[1] = db->loop_current_mode;
        *dlen = 2u;
        break;

    case 8:                                 /* Read Dyn.Var Classifications */
        data[0] = HART_CLASS_PRESSURE;      /* PV                           */
        data[1] = HART_CLASS_TEMPERATURE;   /* SV                           */
        data[2] = HART_CLASS_TEMPERATURE;   /* TV                           */
        data[3] = HART_CLASS_NONE;          /* QV yok                       */
        *dlen = 4u;
        break;

    case 9: {                               /* Read Dev Vars with Status    */
        if (req->bc < 1u) { *rc = HART_RC_TOO_FEW_BYTES; break; }
        uint8_t n = req->bc; if (n > 8u) n = 8u;
        data[0] = db->ext_dev_status;
        for (uint8_t i = 0; i < n; i++)
            dv_slot(&data[1u + 8u*i], req->data[i], live);
        uint32_t ts = s_time_ms * 32u;      /* HART zamanı: 1/32 ms         */
        uint8_t  off = (uint8_t)(1u + 8u*n);
        data[off]     = (uint8_t)(ts >> 24);
        data[off+1u]  = (uint8_t)(ts >> 16);
        data[off+2u]  = (uint8_t)(ts >> 8);
        data[off+3u]  = (uint8_t)(ts);
        *dlen = (uint8_t)(off + 4u);
        break;
    }

    case 11:                                /* Read Unique ID by Tag        */
        if (req->bc < 6u) { *rc = HART_RC_TOO_FEW_BYTES; break; }
        if (memcmp(req->data, db->tag, 6u) != 0)
            return false;                   /* tag bizim değil → SESSİZ     */
        *dlen = cmd0_payload(db, data);
        break;

    case 20:                                /* Read Long Tag                */
        memcpy(data, db->long_tag, 32u);
        *dlen = 32u;
        break;

    case 21:                                /* Read Unique ID by Long Tag   */
        if (req->bc < 32u) { *rc = HART_RC_TOO_FEW_BYTES; break; }
        if (memcmp(req->data, db->long_tag, 32u) != 0)
            return false;                   /* bizim değil → SESSİZ         */
        *dlen = cmd0_payload(db, data);
        break;

    case 22:                                /* Write Long Tag               */
        if (req->bc < 32u) { *rc = HART_RC_TOO_FEW_BYTES; break; }
        memcpy(db->long_tag, req->data, 32u);
        cfg_mark(db);
        memcpy(data, db->long_tag, 32u);
        *dlen = 32u;
        break;

    case 12:                                /* Read Message                 */
        memcpy(data, db->message, 24u);
        *dlen = 24u;
        break;

    case 13:                                /* Read Tag/Descriptor/Date     */
        memcpy(&data[0],  db->tag,        6u);
        memcpy(&data[6],  db->descriptor, 12u);
        memcpy(&data[18], db->date,       3u);
        *dlen = 21u;
        break;

    case 14:                                /* Read PV Transducer Info      */
        memcpy(&data[0], db->sensor_sn, 3u);
        data[3] = HART_UNIT_BAR;
        hart_put_f32(&data[4],  db->usl_bar);
        hart_put_f32(&data[8],  db->lsl_bar);
        hart_put_f32(&data[12], db->min_span_bar);
        *dlen = 16u;
        break;

    case 15:                                /* Read Output Information      */
        data[0] = 1u;                       /* alarm: LOW (NAMUR 3.6 mA)    */
        data[1] = 0u;                       /* transfer fn: linear          */
        data[2] = HART_UNIT_BAR;            /* range birimi                 */
        hart_put_f32(&data[3],  live->urv_bar);   /* URV (20 mA noktası)    */
        hart_put_f32(&data[7],  live->lrv_bar);   /* LRV (4 mA noktası)     */
        hart_put_f32(&data[11], live->damping_s); /* damping [s]            */
        data[15] = 251u;                    /* write-protect: yok           */
        data[16] = 250u;                    /* private-label dist.: yok     */
        *dlen = 17u;
        break;

    case 16:                                /* Read Final Assembly Number   */
        memcpy(data, db->final_asm, 3u);
        *dlen = 3u;
        break;

    case 17:                                /* Write Message                */
        if (req->bc < 24u) { *rc = HART_RC_TOO_FEW_BYTES; break; }
        memcpy(db->message, req->data, 24u);
        cfg_mark(db);
        memcpy(data, db->message, 24u);
        *dlen = 24u;
        break;

    case 18:                                /* Write Tag/Descriptor/Date    */
        if (req->bc < 21u) { *rc = HART_RC_TOO_FEW_BYTES; break; }
        memcpy(db->tag,        &req->data[0],  6u);
        memcpy(db->descriptor, &req->data[6],  12u);
        memcpy(db->date,       &req->data[18], 3u);
        cfg_mark(db);
        memcpy(data, req->data, 21u);
        *dlen = 21u;
        break;

    case 19:                                /* Write Final Assembly Number  */
        if (req->bc < 3u) { *rc = HART_RC_TOO_FEW_BYTES; break; }
        memcpy(db->final_asm, req->data, 3u);
        cfg_mark(db);
        memcpy(data, db->final_asm, 3u);
        *dlen = 3u;
        break;

    case 38:                                /* Reset Config-Changed Flag    */
        /* HART7: master kendi bildiği sayacı gönderir; uyuşmazsa temizleme
         * yapılmaz (başka master arada konfig değiştirmiş demektir).       */
        if (req->bc >= 2u) {
            uint16_t m = (uint16_t)(((uint16_t)req->data[0] << 8) | req->data[1]);
            if (m != db->cfg_change_count) { *rc = HART_RC_CTR_MISMATCH; break; }
        }
        db->cfg_changed = false;
        data[0] = (uint8_t)(db->cfg_change_count >> 8);
        data[1] = (uint8_t)(db->cfg_change_count);
        *dlen = 2u;
        break;

    case 48:                                /* Read Additional Status       */
        memset(data, 0, 9u);
        data[0] = live->extra_status;       /* cihaz-özel durum bitleri     */
        data[6] = db->ext_dev_status;       /* HART7 extended device status */
        /* data[7] op-mode=0, data[8] standardized status0=0                */
        *dlen = 9u;
        break;

    default:
        *rc = HART_RC_NOT_IMPLEMENTED;
        break;
    }

    /* İlk başarılı cevapla cold-start bayrağı düşer (basitleştirme:
     * master-başına ayrı takip yerine global — HART_INTEGRATION.md).       */
    if (*rc == HART_RC_OK) db->cold_start = false;

    return true;
}
