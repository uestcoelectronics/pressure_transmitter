/**
 * hart_host_test.c — HART dl+cmds katmanlarının PC'de birim testi.
 * (HAL'siz katmanlar sayesinde donanımsız doğrulama.)
 *
 * Derle + koştur:
 *   gcc -Wall -Wextra -I../Inc ../Src/hart_dl.c ../Src/hart_cmds.c \
 *       hart_host_test.c -o hart_test && ./hart_test
 */
#include "hart_dl.h"
#include "hart_cmds.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static int g_fail;
#define CHECK(cond, msg) do { \
    if (cond) printf("  OK   %s\n", msg); \
    else { printf("  FAIL %s\n", msg); g_fail++; } } while (0)

/* Master isteği kur (test yardımcıları) */
static uint16_t build_stx_short(uint8_t poll, uint8_t cmd,
                                const uint8_t *d, uint8_t dlen, uint8_t *out)
{
    uint16_t n = 0; uint8_t i, chk;
    for (i = 0; i < 5; i++) out[n++] = 0xFF;
    out[n++] = 0x02;                       /* STX, kısa adres               */
    out[n++] = (uint8_t)(0x80u | poll);    /* primary master                */
    out[n++] = cmd;
    out[n++] = dlen;
    for (i = 0; i < dlen; i++) out[n++] = d[i];
    chk = 0; for (i = 5; i < n; i++) chk ^= out[i];
    out[n++] = chk;
    return n;
}

static uint16_t build_stx_long(const uint8_t a[5], uint8_t cmd,
                               const uint8_t *d, uint8_t dlen, uint8_t *out)
{
    uint16_t n = 0; uint8_t i, chk;
    for (i = 0; i < 5; i++) out[n++] = 0xFF;
    out[n++] = 0x82;
    for (i = 0; i < 5; i++) out[n++] = a[i];
    out[n++] = cmd;
    out[n++] = dlen;
    for (i = 0; i < dlen; i++) out[n++] = d[i];
    chk = 0; for (i = 5; i < n; i++) chk ^= out[i];
    out[n++] = chk;
    return n;
}

/* Frame'i parser'a bayt bayt bes; tamamlanınca 1 döner */
static int feed(hart_dl_t *dl, const uint8_t *buf, uint16_t n, uint32_t t)
{
    int done = 0;
    for (uint16_t i = 0; i < n; i++)
        if (hart_dl_input(dl, buf[i], t)) done = 1;
    return done;
}

static float get_f32(const uint8_t *p)
{
    union { float f; uint32_t u; } c;
    c.u = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
          ((uint32_t)p[2] << 8) | p[3];
    return c.f;
}

int main(void)
{
    hart_dl_t dl; hart_db_t db; hart_live_t live;
    uint8_t req[96], ack[96], data[HART_MAX_DATA], rc, dlen;
    uint16_t n;

    hart_dl_init(&dl);
    hart_db_init(&db);
    memset(&live, 0, sizeof live);
    live.pv_bar = 5.25f; live.sv_temp_c = 23.5f; live.tv_temp_c = 31.0f;
    live.loop_ma = 12.4f; live.pct_range = 52.5f;
    live.lrv_bar = 0.0f; live.urv_bar = 10.0f; live.damping_s = 0.5f;

    printf("== 1) Packed ASCII gidis-donus ==\n");
    { uint8_t p[6]; char s[9];
      hart_pack_ascii("PT910", p, 6); hart_unpack_ascii(p, s, 6);
      CHECK(strncmp(s, "PT910   ", 8) == 0, "pack/unpack 'PT910'"); }

    printf("== 2) Cmd 0 (kisa adres, poll=0) ==\n");
    n = build_stx_short(0, 0, NULL, 0, req);
    CHECK(feed(&dl, req, n, 100), "cerceve parse edildi");
    CHECK(dl.f.cmd == 0 && !dl.f.long_addr, "cmd/addr alanlari");
    CHECK(hart_addr_match(&db, &dl.f), "adres eslesti");
    { uint8_t ds = hart_device_status(&db, &live);
      CHECK(ds & HART_DS_COLD_START, "cold-start biti ilk cevapta set"); }
    CHECK(hart_cmds_handle(&db, &dl.f, &live, &rc, data, &dlen), "islendi");
    CHECK(rc == 0 && dlen == 12 && data[0] == 254, "cmd0 payload (FE, 12B)");
    CHECK(data[4] == 5, "universal rev = HART 5");
    CHECK(!db.cold_start, "cold-start cevap sonrasi dustu");
    n = hart_dl_build_ack(&dl.f, rc, 0x20, data, dlen, ack);
    { uint8_t chk = 0; for (uint16_t i = 5; i < n; i++) chk ^= ack[i];
      CHECK(chk == 0, "ACK checksum tutarli (XOR=0 chk dahil)");
      CHECK(ack[5] == 0x06 && ack[6] == 0x80, "ACK delimiter + adres yansima"); }

    printf("== 3) Cmd 1 (PV) float big-endian ==\n");
    n = build_stx_short(0, 1, NULL, 0, req);
    feed(&dl, req, n, 200);
    hart_cmds_handle(&db, &dl.f, &live, &rc, data, &dlen);
    CHECK(rc == 0 && dlen == 5 && data[0] == HART_UNIT_BAR, "birim=bar");
    CHECK(fabsf(get_f32(&data[1]) - 5.25f) < 1e-6f, "PV = 5.25");

    printf("== 4) Cmd 3 (dinamik degiskenler) ==\n");
    n = build_stx_short(0, 3, NULL, 0, req);
    feed(&dl, req, n, 300);
    hart_cmds_handle(&db, &dl.f, &live, &rc, data, &dlen);
    CHECK(rc == 0 && dlen == 19, "uzunluk 19 (akim + 3 degisken)");
    CHECK(fabsf(get_f32(&data[0]) - 12.4f) < 1e-6f, "loop mA");
    CHECK(data[9] == HART_UNIT_CELSIUS &&
          fabsf(get_f32(&data[10]) - 23.5f) < 1e-6f, "SV = 23.5 C");

    printf("== 5) Bozuk checksum reddi ==\n");
    n = build_stx_short(0, 1, NULL, 0, req);
    req[n - 1] ^= 0xFF;
    CHECK(!feed(&dl, req, n, 400), "gecersiz chk parse ETMEDI");

    printf("== 6) Cmd 6 (poll adresi yaz) + yeni adresle eslesme ==\n");
    { uint8_t pa = 7;
      n = build_stx_short(0, 6, &pa, 1, req);
      feed(&dl, req, n, 500);
      hart_cmds_handle(&db, &dl.f, &live, &rc, data, &dlen);
      CHECK(rc == 0 && db.poll_addr == 7 && db.cfg_changed, "yazildi + cfg-changed");
      n = build_stx_short(7, 0, NULL, 0, req); feed(&dl, req, n, 600);
      CHECK(hart_addr_match(&db, &dl.f), "poll=7 eslesiyor");
      n = build_stx_short(0, 0, NULL, 0, req); feed(&dl, req, n, 700);
      CHECK(!hart_addr_match(&db, &dl.f), "poll=0 artik eslesmiyor");
      pa = 99; n = build_stx_short(7, 6, &pa, 1, req); feed(&dl, req, n, 750);
      hart_cmds_handle(&db, &dl.f, &live, &rc, data, &dlen);
      CHECK(rc == HART_RC_INVALID_SEL, "poll>15 -> RC=2"); }

    printf("== 7) Uzun adres cmd 0 ==\n");
    { uint8_t a[5] = { (uint8_t)(0x80u | (db.mfr_id & 0x3Fu)),
                       db.dev_type, db.dev_id[0], db.dev_id[1], db.dev_id[2] };
      n = build_stx_long(a, 0, NULL, 0, req);
      CHECK(feed(&dl, req, n, 800), "uzun cerceve parse");
      CHECK(hart_addr_match(&db, &dl.f), "uzun adres eslesti");
      a[4] ^= 1;
      n = build_stx_long(a, 0, NULL, 0, req); feed(&dl, req, n, 900);
      CHECK(!hart_addr_match(&db, &dl.f), "yanlis dev_id eslesmedi"); }

    printf("== 8) Cmd 11 (tag broadcast) ==\n");
    { uint8_t bc[5] = { 0x80, 0, 0, 0, 0 };   /* broadcast (primary master) */
      n = build_stx_long(bc, 11, db.tag, 6, req);
      feed(&dl, req, n, 1000);
      CHECK(hart_addr_match(&db, &dl.f), "broadcast+cmd11 kabul");
      CHECK(hart_cmds_handle(&db, &dl.f, &live, &rc, data, &dlen) &&
            dlen == 12, "tag uyusti -> cmd0 payload");
      uint8_t yanlis[6]; hart_pack_ascii("YOK", yanlis, 6);
      n = build_stx_long(bc, 11, yanlis, 6, req);
      feed(&dl, req, n, 1100);
      CHECK(!hart_cmds_handle(&db, &dl.f, &live, &rc, data, &dlen),
            "tag uyusmadi -> sessiz"); }

    printf("== 9) Gap timeout yarim cerceveyi atar ==\n");
    n = build_stx_short(7, 1, NULL, 0, req);
    feed(&dl, req, (uint16_t)(n - 3u), 2000);      /* yarim birak          */
    CHECK(feed(&dl, req, n, 2500), "timeout sonrasi yeni cerceve parse edildi");

    printf("== 10) Bilinmeyen komut -> RC 64 ==\n");
    n = build_stx_short(7, 123, NULL, 0, req);
    feed(&dl, req, n, 3000);
    hart_cmds_handle(&db, &dl.f, &live, &rc, data, &dlen);
    CHECK(rc == HART_RC_NOT_IMPLEMENTED && dlen == 0, "RC=64, veri yok");

    printf("\n%s (%d hata)\n", g_fail ? "SONUC: HATA VAR" : "SONUC: TUM TESTLER GECTI", g_fail);
    return g_fail ? 1 : 0;
}
