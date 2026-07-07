/**
 * hart.c — HART orkestratörü: phy'den bayt al → dl ile çöz → cmds ile
 * işle → dl ile cevap kur → phy ile gönder.
 */
#include "hart.h"
#include "hart_phy.h"
#include "hart_dl.h"

static hart_dl_t  s_dl;
static hart_db_t  s_db;
static uint8_t    s_txbuf[HART_TX_PREAMBLES + HART_MAX_FRAME];
static uint32_t   s_rx_cnt, s_tx_cnt;

void hart_init(void)
{
    hart_dl_init(&s_dl);
    hart_db_init(&s_db);
    hart_phy_init();
}

uint32_t hart_rx_frames(void) { return s_rx_cnt; }
uint32_t hart_tx_frames(void) { return s_tx_cnt; }

void hart_service(uint32_t now_ms, const hart_live_t *live)
{
    uint8_t b;

    if (hart_phy_take_rx_error())
        hart_dl_mark_error(&s_dl);      /* bozuk çerçeveyi yut              */

    hart_dl_gap_check(&s_dl, now_ms);

    while (hart_phy_read_byte(&b)) {
        if (!hart_dl_input(&s_dl, b, now_ms))
            continue;

        /* Geçerli çerçeve. Yalnız bize adresli STX (istek) işlenir;
         * hattaki ACK/BACK trafiği (başka slave cevapları) yok sayılır.    */
        const hart_frame_t *f = &s_dl.f;
        if (f->frame_type != HART_FT_STX)     continue;
        if (!hart_addr_match(&s_db, f))       continue;

        s_rx_cnt++;

        /* Status'u İŞLEMEDEN ÖNCE üret: cold-start/cfg-changed bitleri
         * bu cevapta görünür, handler başarıyla bitince düşer.             */
        uint8_t dev_status = hart_device_status(&s_db, live);

        uint8_t rc, dlen;
        uint8_t data[HART_MAX_DATA];
        if (!hart_cmds_handle(&s_db, f, live, &rc, data, &dlen))
            continue;                    /* cmd 11 tag uyuşmadı → sessiz    */

        uint16_t n = hart_dl_build_ack(f, rc, dev_status, data, dlen, s_txbuf);
        if (hart_phy_send(s_txbuf, n))
            s_tx_cnt++;
        /* TX meşgulse cevap düşer; master retry eder (half-duplex'te
         * normalde imkânsız — master cevabı beklemeden yeni istek atmaz). */
    }
}
