#include "ble_proto.h"
#include "ble_uart.h"
#include "cal_storage.h"
#include "temp_diode.h"
#include "stm32u3xx_hal.h"
#include <string.h>

/* -------------------------------------------------------------------------- */
/* AT init durum makinesi                                                      */
/* -------------------------------------------------------------------------- */
typedef enum {
    ST_WAIT_BOOT = 0,   /* reset sonrası modül boot beklemesi (+PWRUP)        */
    ST_PING,            /* AT → OK                                            */
    ST_NAME,            /* AT+ADVNAME=PT910,PT → OK                          */
    ST_ADV,             /* AT+ADVSTA=1 → OK                                  */
    ST_ENTM,            /* AT+ENTM → transparent                            */
    ST_RUN              /* transparent çerçeve protokolü                     */
} proto_state_t;

#define AT_STEP_TIMEOUT_MS   500u
#define AT_STEP_RETRY        3u
#define BOOT_WAIT_MS         300u

static proto_state_t s_state    = ST_WAIT_BOOT;
static uint32_t      s_step_t   = 0;
static uint8_t       s_retry    = 0;
static bool          s_degraded = false;   /* AT konfig başarısız → yine de RUN */

/* AT yanıtı için satır tamponu */
static char    s_line[48];
static uint8_t s_line_n = 0;

/* -------------------------------------------------------------------------- */
/* Çerçeve protokolü                                                           */
/* -------------------------------------------------------------------------- */
#define FRAME_SOF       0xAAu
#define MAX_PAYLOAD     32u

#define CMD_GET_MEAS    0x01u
#define CMD_GET_PARAM   0x02u
#define CMD_SET_PARAM   0x03u
#define CMD_SAVE        0x04u
#define CMD_INFO        0x05u
#define CMD_UNLOCK      0x06u

/* Yanıt durum kodları (payload[0]) */
#define ST_OK           0x00u
#define ST_ERR_CMD      0x01u
#define ST_ERR_PARAM    0x02u
#define ST_ERR_LOCKED   0x03u
#define ST_ERR_CRC      0x04u

/* Parametre ID'leri */
enum {
    PID_P_MIN = 1, PID_P_MAX, PID_DAMPING, PID_KT_ZERO, PID_KT_SPAN,
    PID_VF25, PID_TC, PID_CAP_ZERO, PID_CAP_SPAN
};

/* Yazma koruması — sabit PIN (saha aracında konfigüre edilebilir). */
#define BLE_UNLOCK_PIN   0x00001357u

static bool s_unlocked = false;

/* Çerçeve parser durumu */
typedef enum { F_SOF = 0, F_LEN, F_CMD, F_PAYLOAD, F_CRC_HI, F_CRC_LO } fstate_t;
static fstate_t s_fst   = F_SOF;
static uint8_t  s_flen  = 0;
static uint8_t  s_fcmd  = 0;
static uint8_t  s_fpl[MAX_PAYLOAD];
static uint8_t  s_fpl_n = 0;
static uint16_t s_fcrc  = 0;

/* CRC16-CCITT (poly 0x1021, init 0xFFFF) */
static uint16_t crc16(const uint8_t *d, uint32_t n)
{
    uint16_t c = 0xFFFFu;
    while (n--) {
        c ^= (uint16_t)(*d++) << 8;
        for (int i = 0; i < 8; ++i)
            c = (c & 0x8000u) ? (uint16_t)((c << 1) ^ 0x1021u) : (uint16_t)(c << 1);
    }
    return c;
}

/* -------------------------------------------------------------------------- */
/* AT yardımcıları                                                             */
/* -------------------------------------------------------------------------- */
static void at_send(const char *s)
{
    ble_uart_write((const uint8_t*)s, (uint32_t)strlen(s));
}

static void enter_step(proto_state_t st, const char *cmd)
{
    s_state  = st;
    s_step_t = HAL_GetTick();
    s_retry  = 0;
    s_line_n = 0;
    if (cmd) at_send(cmd);
}

/* AT init fazında satır biriktir; "OK"/"ERROR"/"+PWRUP" tespit et.
 * return: 1=OK, -1=ERROR, 2=PWRUP, 0=henüz tam satır yok                     */
static int at_collect(void)
{
    uint8_t b;
    int result = 0;
    while (ble_uart_read_byte(&b)) {
        if (b == '\n' || b == '\r') {
            if (s_line_n > 0) {
                s_line[s_line_n] = '\0';
                if (strstr(s_line, "OK"))           result = 1;
                else if (strstr(s_line, "ERROR"))   result = -1;
                else if (strstr(s_line, "+PWRUP"))  result = 2;
                s_line_n = 0;
                if (result) return result;
            }
        } else if (s_line_n < sizeof(s_line) - 1u) {
            s_line[s_line_n++] = (char)b;
        }
    }
    return 0;
}

/* AT init adımı: OK gelince next'e geç; timeout/ERROR'da retry, retry biterse
   degraded işaretle ve next'e geç (modül zaten konfigli olabilir).           */
static void at_step(proto_state_t next, const char *cmd)
{
    int r = at_collect();
    if (r == 1) {                          /* OK */
        enter_step(next, cmd);
        return;
    }
    if (r == -1 || (HAL_GetTick() - s_step_t) >= AT_STEP_TIMEOUT_MS) {
        if (s_retry < AT_STEP_RETRY) {
            /* Bekleme penceresini yenile. (Komut yeniden gönderilmez; çoğu
               modül önceki komutu işlemiştir — başarısızsa degraded devreye
               girer.)                                                         */
            s_retry++;
            s_line_n = 0;
            s_step_t = HAL_GetTick();
        } else {
            s_degraded = true;
            enter_step(next, cmd);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Çerçeve yanıtı                                                              */
/* -------------------------------------------------------------------------- */
static void send_frame(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t f[5 + MAX_PAYLOAD];
    uint8_t n = 0;
    f[n++] = FRAME_SOF;
    f[n++] = len;
    f[n++] = cmd;
    for (uint8_t i = 0; i < len; ++i) f[n++] = payload[i];
    uint16_t c = crc16(&f[1], (uint32_t)(2u + len));   /* LEN,CMD,PAYLOAD */
    f[n++] = (uint8_t)(c >> 8);
    f[n++] = (uint8_t)(c & 0xFF);
    ble_uart_write(f, n);
}

static void put_f32(uint8_t *p, float v) { memcpy(p, &v, 4); }
static float get_f32(const uint8_t *p)   { float v; memcpy(&v, p, 4); return v; }

/* Param oku/yaz — return durum kodu */
static uint8_t param_get(uint8_t pid, float *out)
{
    const cal_params_t *c = cal_get();
    switch (pid) {
        case PID_P_MIN:    *out = c->p_min;            return ST_OK;
        case PID_P_MAX:    *out = c->p_max;            return ST_OK;
        case PID_DAMPING:  *out = c->damping_s;        return ST_OK;
        case PID_KT_ZERO:  *out = c->k_t_zero;         return ST_OK;
        case PID_KT_SPAN:  *out = c->k_t_span;         return ST_OK;
        case PID_VF25:     *out = c->vf25_mv;          return ST_OK;
        case PID_TC:       *out = c->tc_mv_c;          return ST_OK;
        case PID_CAP_ZERO: *out = (float)c->cap_at_zero; return ST_OK;
        case PID_CAP_SPAN: *out = (float)c->cap_at_span; return ST_OK;
        default:           return ST_ERR_PARAM;
    }
}

static uint8_t param_set(uint8_t pid, float v)
{
    cal_params_t *c = cal_get_mutable();
    switch (pid) {
        case PID_P_MIN:    c->p_min     = v; break;
        case PID_P_MAX:    c->p_max     = v; break;
        case PID_DAMPING:  c->damping_s = v; break;
        case PID_KT_ZERO:  c->k_t_zero  = v; break;
        case PID_KT_SPAN:  c->k_t_span  = v; break;
        case PID_VF25:     c->vf25_mv   = v;
                           temp_diode_set_calibration(c->vf25_mv, c->tc_mv_c); break;
        case PID_TC:       c->tc_mv_c   = v;
                           temp_diode_set_calibration(c->vf25_mv, c->tc_mv_c); break;
        case PID_CAP_ZERO: c->cap_at_zero = (int32_t)v; break;
        case PID_CAP_SPAN: c->cap_at_span = (int32_t)v; break;
        default:           return ST_ERR_PARAM;
    }
    return ST_OK;
}

/* Çerçeve komut dispatcher — ölçümler GET_MEAS için aktarılır */
static void dispatch(float p_bar, float t_c, float ma, uint8_t devstat)
{
    uint8_t resp[1 + MAX_PAYLOAD];

    switch (s_fcmd) {
    case CMD_GET_MEAS:
        resp[0] = ST_OK;
        put_f32(&resp[1],  p_bar);
        put_f32(&resp[5],  t_c);
        put_f32(&resp[9],  ma);
        resp[13] = devstat;
        send_frame(CMD_GET_MEAS, resp, 14);
        break;

    case CMD_GET_PARAM: {
        if (s_fpl_n < 1) { resp[0] = ST_ERR_PARAM; send_frame(CMD_GET_PARAM, resp, 1); break; }
        float v = 0.0f;
        uint8_t st = param_get(s_fpl[0], &v);
        resp[0] = st;
        resp[1] = s_fpl[0];
        put_f32(&resp[2], v);
        send_frame(CMD_GET_PARAM, resp, (st == ST_OK) ? 6 : 1);
        break;
    }

    case CMD_SET_PARAM: {
        if (!s_unlocked)   { resp[0] = ST_ERR_LOCKED; send_frame(CMD_SET_PARAM, resp, 1); break; }
        if (s_fpl_n < 5)   { resp[0] = ST_ERR_PARAM;  send_frame(CMD_SET_PARAM, resp, 1); break; }
        uint8_t st = param_set(s_fpl[0], get_f32(&s_fpl[1]));
        resp[0] = st;
        resp[1] = s_fpl[0];
        send_frame(CMD_SET_PARAM, resp, 2);
        break;
    }

    case CMD_SAVE:
        if (!s_unlocked)   { resp[0] = ST_ERR_LOCKED; send_frame(CMD_SAVE, resp, 1); break; }
        resp[0] = cal_save() ? ST_OK : ST_ERR_PARAM;
        send_frame(CMD_SAVE, resp, 1);
        break;

    case CMD_INFO: {
        static const char info[] = "PT910 4-20 v0.9";
        resp[0] = ST_OK;
        memcpy(&resp[1], info, sizeof(info));   /* NUL dahil */
        send_frame(CMD_INFO, resp, (uint8_t)(1u + sizeof(info)));
        break;
    }

    case CMD_UNLOCK: {
        uint32_t pin = 0;
        if (s_fpl_n >= 4) memcpy(&pin, s_fpl, 4);
        s_unlocked = (pin == BLE_UNLOCK_PIN);
        resp[0] = s_unlocked ? ST_OK : ST_ERR_LOCKED;
        send_frame(CMD_UNLOCK, resp, 1);
        break;
    }

    default:
        resp[0] = ST_ERR_CMD;
        send_frame(s_fcmd, resp, 1);
        break;
    }
}

/* Transparent kanaldan gelen baytları çerçeveye besle */
static void frame_feed(uint8_t b, float p_bar, float t_c, float ma, uint8_t devstat)
{
    switch (s_fst) {
    case F_SOF:
        if (b == FRAME_SOF) s_fst = F_LEN;
        break;
    case F_LEN:
        s_flen  = (b > MAX_PAYLOAD) ? MAX_PAYLOAD : b;
        s_fpl_n = 0;
        s_fst   = F_CMD;
        break;
    case F_CMD:
        s_fcmd = b;
        s_fst  = (s_flen > 0) ? F_PAYLOAD : F_CRC_HI;
        break;
    case F_PAYLOAD:
        s_fpl[s_fpl_n++] = b;
        if (s_fpl_n >= s_flen) s_fst = F_CRC_HI;
        break;
    case F_CRC_HI:
        s_fcrc = (uint16_t)b << 8;
        s_fst  = F_CRC_LO;
        break;
    case F_CRC_LO: {
        s_fcrc |= b;
        /* CRC doğrula: [LEN,CMD,PAYLOAD] */
        uint8_t tmp[2 + MAX_PAYLOAD];
        tmp[0] = s_flen; tmp[1] = s_fcmd;
        memcpy(&tmp[2], s_fpl, s_flen);
        if (crc16(tmp, (uint32_t)(2u + s_flen)) == s_fcrc) {
            dispatch(p_bar, t_c, ma, devstat);
        } else {
            uint8_t r = ST_ERR_CRC;
            send_frame(s_fcmd, &r, 1);
        }
        s_fst = F_SOF;
        break;
    }
    }
}

/* -------------------------------------------------------------------------- */
/* Genel API                                                                   */
/* -------------------------------------------------------------------------- */
void ble_proto_init(void)
{
    s_state    = ST_WAIT_BOOT;
    s_step_t   = HAL_GetTick();
    s_retry    = 0;
    s_degraded = false;
    s_line_n   = 0;
    s_unlocked = false;
    s_fst      = F_SOF;
}

bool ble_proto_ready(void) { return s_state == ST_RUN; }

void ble_proto_service(uint32_t now_ms, float p_bar, float t_c, float ma,
                       uint8_t status_byte)
{
    switch (s_state) {
    case ST_WAIT_BOOT:
        (void)at_collect();                /* +PWRUP'ı yut */
        if ((now_ms - s_step_t) >= BOOT_WAIT_MS) {
            enter_step(ST_PING, "AT\r\n");
        }
        break;
    case ST_PING:  at_step(ST_NAME, "AT+ADVNAME=PT910,PT\r\n"); break;
    case ST_NAME:  at_step(ST_ADV,  "AT+ADVSTA=1\r\n");         break;
    case ST_ADV:   at_step(ST_ENTM, "AT+ENTM\r\n");             break;
    case ST_ENTM:  at_step(ST_RUN,  NULL);                      break;

    case ST_RUN: {
        uint8_t b;
        uint32_t guard = 0;
        while (ble_uart_read_byte(&b) && guard < 256u) {
            frame_feed(b, p_bar, t_c, ma, status_byte);
            guard++;
        }
        break;
    }
    }
}
