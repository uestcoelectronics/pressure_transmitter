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
    db->univ_rev      = 5u;    /* HART 5 komut seti                         */
    db->dev_rev       = 1u;
    db->sw_rev        = 1u;
    db->hw_rev        = 1u;
    db->flags         = 0u;

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
        return f->cmd == 11u;      /* broadcast yalnız "ID by tag" için     */
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
    d[1]  = db->mfr_id;
    d[2]  = db->dev_type;
    d[3]  = db->min_preambles;
    d[4]  = db->univ_rev;
    d[5]  = db->dev_rev;
    d[6]  = db->sw_rev;
    d[7]  = db->hw_rev;         /* bit0..2 fiziksel sinyal kodu = 0 (FSK)   */
    d[8]  = db->flags;
    d[9]  = db->dev_id[0];
    d[10] = db->dev_id[1];
    d[11] = db->dev_id[2];
    return 12u;
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
        if (req->data[0] > 15u)      { *rc = HART_RC_INVALID_SEL;   break; }
        db->poll_addr   = req->data[0];
        db->cfg_changed = true;
        data[0] = db->poll_addr;
        *dlen = 1u;
        break;

    case 11:                                /* Read Unique ID by Tag        */
        if (req->bc < 6u) { *rc = HART_RC_TOO_FEW_BYTES; break; }
        if (memcmp(req->data, db->tag, 6u) != 0)
            return false;                   /* tag bizim değil → SESSİZ     */
        *dlen = cmd0_payload(db, data);
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
        db->cfg_changed = true;
        memcpy(data, db->message, 24u);
        *dlen = 24u;
        break;

    case 18:                                /* Write Tag/Descriptor/Date    */
        if (req->bc < 21u) { *rc = HART_RC_TOO_FEW_BYTES; break; }
        memcpy(db->tag,        &req->data[0],  6u);
        memcpy(db->descriptor, &req->data[6],  12u);
        memcpy(db->date,       &req->data[18], 3u);
        db->cfg_changed = true;
        memcpy(data, req->data, 21u);
        *dlen = 21u;
        break;

    case 19:                                /* Write Final Assembly Number  */
        if (req->bc < 3u) { *rc = HART_RC_TOO_FEW_BYTES; break; }
        memcpy(db->final_asm, req->data, 3u);
        db->cfg_changed = true;
        memcpy(data, db->final_asm, 3u);
        *dlen = 3u;
        break;

    case 38:                                /* Reset Config-Changed (CP)    */
        db->cfg_changed = false;
        break;

    case 48:                                /* Read Additional Status (CP)  */
        memset(data, 0, 8u);
        data[0] = live->extra_status;       /* cihaz-özel durum bitleri     */
        *dlen = 8u;
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
