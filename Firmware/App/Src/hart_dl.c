/**
 * hart_dl.c — HART çerçeve parser + cevap kurucu (HAL'siz, host-test edilir).
 * Format ayrıntısı için hart_dl.h başlığına bak.
 */
#include "hart_dl.h"
#include <string.h>

enum { ST_HUNT = 0, ST_ADDR, ST_CMD, ST_BC, ST_DATA, ST_CHK };

void hart_dl_init(hart_dl_t *dl)
{
    memset(dl, 0, sizeof(*dl));
    dl->state = ST_HUNT;
}

static void reset_parser(hart_dl_t *dl)
{
    dl->state   = ST_HUNT;
    dl->pre_cnt = 0;
    dl->got     = 0;
    dl->chk     = 0;
    dl->drop    = false;
}

void hart_dl_gap_check(hart_dl_t *dl, uint32_t now_ms)
{
    if (dl->state != ST_HUNT &&
        (now_ms - dl->last_byte_ms) > HART_GAP_TIMEOUT_MS) {
        reset_parser(dl);           /* yarım çerçeve bayatladı               */
    }
}

void hart_dl_mark_error(hart_dl_t *dl)
{
    /* Parity/framing hatası: bu çerçeve güvenilmez. Çerçeve bitene kadar
     * (gap timeout) her şeyi yut.                                          */
    dl->drop = true;
}

bool hart_dl_input(hart_dl_t *dl, uint8_t b, uint32_t now_ms)
{
    hart_dl_gap_check(dl, now_ms);
    dl->last_byte_ms = now_ms;

    if (dl->drop) return false;     /* hatalı çerçeve; gap resetler          */

    switch (dl->state) {
    case ST_HUNT:
        if (b == 0xFFu) {
            if (dl->pre_cnt < 0xFFu) dl->pre_cnt++;
        } else if (dl->pre_cnt >= 2u) {
            /* Preamble sonrası ilk bayt = delimiter olmalı                  */
            uint8_t ft = b & 0x07u;
            if (ft == HART_FT_STX || ft == HART_FT_ACK || ft == HART_FT_BACK) {
                dl->f.long_addr  = (b & 0x80u) != 0u;
                dl->f.frame_type = ft;
                dl->chk   = b;
                dl->need  = dl->f.long_addr ? 5u : 1u;
                dl->got   = 0;
                dl->state = ST_ADDR;
            } else {
                dl->pre_cnt = 0;    /* geçersiz delimiter → yeniden ara      */
            }
        } else {
            dl->pre_cnt = 0;
        }
        break;

    case ST_ADDR:
        dl->f.addr[dl->got++] = b;
        dl->chk ^= b;
        if (dl->got == dl->need) {
            dl->f.primary_master = (dl->f.addr[0] & 0x80u) != 0u;
            dl->state = ST_CMD;
        }
        break;

    case ST_CMD:
        dl->f.cmd = b;
        dl->chk ^= b;
        dl->state = ST_BC;
        break;

    case ST_BC:
        if (b > HART_MAX_DATA) {    /* taşma — çerçeveyi yut                 */
            dl->drop = true;
            break;
        }
        dl->f.bc = b;
        dl->chk ^= b;
        dl->got  = 0;
        dl->state = (b == 0u) ? ST_CHK : ST_DATA;
        break;

    case ST_DATA:
        dl->f.data[dl->got++] = b;
        dl->chk ^= b;
        if (dl->got == dl->f.bc) dl->state = ST_CHK;
        break;

    case ST_CHK: {
        bool ok = (dl->chk == b);
        reset_parser(dl);
        return ok;                  /* ok=true → dl->f geçerli çerçeve       */
    }

    default:
        reset_parser(dl);
        break;
    }
    return false;
}

uint16_t hart_dl_build_ack(const hart_frame_t *req, uint8_t rc,
                           uint8_t dev_status, const uint8_t *data,
                           uint8_t dlen, uint8_t *out)
{
    uint16_t n = 0;
    uint8_t  i, chk;

    for (i = 0; i < HART_TX_PREAMBLES; i++) out[n++] = 0xFFu;

    /* Delimiter: isteğin adres formatı + ACK tipi                           */
    uint8_t delim = (uint8_t)((req->long_addr ? 0x80u : 0x00u) | HART_FT_ACK);
    out[n++] = delim;

    /* Adres: istekten aynen yansıtılır (master biti korunur, burst=0)       */
    {
        uint8_t alen = req->long_addr ? 5u : 1u;
        for (i = 0; i < alen; i++) {
            uint8_t a = req->addr[i];
            if (i == 0) a &= (uint8_t)~0x40u;   /* burst bitini temizle      */
            out[n++] = a;
        }
    }

    out[n++] = req->cmd;
    out[n++] = (uint8_t)(dlen + 2u);    /* status(2) + data                  */
    out[n++] = rc;                       /* status 1: response code           */
    out[n++] = dev_status;               /* status 2: field device status     */

    for (i = 0; i < dlen; i++) out[n++] = data[i];

    chk = 0;
    for (i = HART_TX_PREAMBLES; i < n; i++) chk ^= out[i];
    out[n++] = chk;

    return n;
}
