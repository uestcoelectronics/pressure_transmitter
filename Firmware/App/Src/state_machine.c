#include "state_machine.h"
#include "cal_storage.h"
#include "lcd400.h"
#include "temp_diode.h"
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* Menü tanımları                                                              */
/* -------------------------------------------------------------------------- */
typedef enum {
    MI_CAL_ZERO = 0,
    MI_CAL_SPAN,
    MI_P_MIN,
    MI_P_MAX,
    MI_DAMPING,
    MI_KT,            /* k_T zero (offset) */
    MI_KT_SPAN,       /* k_T span (gain)   */
    MI_VF25,
    MI_TC,
    MI_SAVE_EXIT,
    MI_EXIT,
    MI__COUNT         /* pressure_app render_menu "/11" ile senkron tut */
} menu_item_t;

static const char *MENU_LABELS[MI__COUNT] = {
    "Cal Zero",
    "Cal Span",
    "P_min",
    "P_max",
    "Damping (s)",
    "kT zero (bar/C)",
    "kT span (bar/C)",
    "Vf25 (mV)",
    "TC (mV/C)",
    "Save & Exit",
    "Exit (no save)"
};

/* -------------------------------------------------------------------------- */
/* Kalibrasyon sağlamlaştırma eşikleri (donanım testinde ayarlanabilir)        */
/* -------------------------------------------------------------------------- */
#define CAL_STAB_WIN          8       /* ~0.8 s @ 100 ms örnekleme            */
#define CAL_STAB_P2P_MAX      2000    /* ΔC peak-to-peak stabilite eşiği      */
#define CAL_MIN_SPAN_COUNTS   10000   /* zero↔span arası asgari ΔC farkı      */

/* -------------------------------------------------------------------------- */
static sm_state_t  s_state    = SM_NORMAL;
static int         s_menu_idx = 0;
static float       s_edit_val = 0.0f;
static float       s_edit_step= 0.1f;
static int32_t     s_live_dc  = 0;
static float       s_live_p   = 0.0f;
static menu_item_t s_editing  = MI__COUNT;

/* Stabilite penceresi (CAL_LIVE) */
static int32_t     s_dc_win[CAL_STAB_WIN];
static uint8_t     s_dc_win_n   = 0;
static uint8_t     s_dc_win_idx = 0;

/* Kirli bayrağı (menü oturumunda değişiklik var mı) + transient mesaj +
   Exit (no save) çift onay durumu                                            */
static bool        s_dirty        = false;
static bool        s_confirm_exit = false;
static const char *s_info_msg     = "";
static const char  MSG_UNSTABLE[] = "UNSTABLE - WAIT";

static void reset_stab_window(void)
{
    s_dc_win_n   = 0;
    s_dc_win_idx = 0;
}

void sm_init(void)
{
    s_state    = SM_NORMAL;
    s_menu_idx = 0;
    s_dirty        = false;
    s_confirm_exit = false;
    s_info_msg     = "";
    reset_stab_window();
}

sm_state_t  sm_get_state(void)         { return s_state; }
int         sm_get_menu_idx(void)      { return s_menu_idx; }
float       sm_get_edit_value(void)    { return s_edit_val; }
const char* sm_get_edit_label(void)
{
    if (s_state == SM_MENU)        return MENU_LABELS[s_menu_idx];
    if (s_state == SM_EDIT_FLOAT)  return MENU_LABELS[s_editing];
    if (s_state == SM_CAL_LIVE) {
        return (s_editing == MI_CAL_ZERO) ? "Cal Zero (live)" : "Cal Span (live)";
    }
    return "";
}

void sm_update_live(int32_t deltaC_raw, float pressure_eu)
{
    s_live_dc = deltaC_raw;
    s_live_p  = pressure_eu;

    s_dc_win[s_dc_win_idx] = deltaC_raw;
    s_dc_win_idx = (uint8_t)((s_dc_win_idx + 1u) % CAL_STAB_WIN);
    if (s_dc_win_n < CAL_STAB_WIN) s_dc_win_n++;

    /* UNSTABLE red mesajı stabilite sağlanınca otomatik temizlensin
       (SPAN TOO CLOSE stabil durumda üretilir — ona dokunma)               */
    if (s_state == SM_CAL_LIVE && s_info_msg == MSG_UNSTABLE &&
        sm_cal_live_stable()) {
        s_info_msg = "";
    }
}

bool sm_cal_live_stable(void)
{
    if (s_dc_win_n < CAL_STAB_WIN) return false;
    int32_t mn = s_dc_win[0], mx = s_dc_win[0];
    for (uint8_t i = 1; i < CAL_STAB_WIN; i++) {
        if (s_dc_win[i] < mn) mn = s_dc_win[i];
        if (s_dc_win[i] > mx) mx = s_dc_win[i];
    }
    return (mx - mn) <= CAL_STAB_P2P_MAX;
}

const char* sm_get_info_msg(void) { return s_info_msg; }

/* -------------------------------------------------------------------------- */
/* Helper: edit moduna geç                                                     */
/* -------------------------------------------------------------------------- */
static void enter_edit(menu_item_t mi)
{
    cal_params_t *c = cal_get_mutable();
    s_editing = mi;
    switch (mi) {
        case MI_P_MIN:    s_edit_val = c->p_min;     s_edit_step = 0.1f;  break;
        case MI_P_MAX:    s_edit_val = c->p_max;     s_edit_step = 0.5f;  break;
        case MI_DAMPING:  s_edit_val = c->damping_s; s_edit_step = 0.1f;  break;
        case MI_KT:       s_edit_val = c->k_t_zero;  s_edit_step = 0.001f;break;
        case MI_KT_SPAN:  s_edit_val = c->k_t_span;  s_edit_step = 0.001f;break;
        case MI_VF25:     s_edit_val = c->vf25_mv;   s_edit_step = 1.0f;  break;
        case MI_TC:       s_edit_val = c->tc_mv_c;   s_edit_step = 0.1f;  break;
        default: break;
    }
    s_state = SM_EDIT_FLOAT;
}

static void commit_edit(void)
{
    cal_params_t *c = cal_get_mutable();
    switch (s_editing) {
        case MI_P_MIN:    c->p_min     = s_edit_val; break;
        case MI_P_MAX:    c->p_max     = s_edit_val; break;
        case MI_DAMPING:  c->damping_s = s_edit_val; break;
        case MI_KT:       c->k_t_zero  = s_edit_val; break;
        case MI_KT_SPAN:  c->k_t_span  = s_edit_val; break;
        /* vf25/tc tek kaynağı cal_params; runtime'ı da senkronla            */
        case MI_VF25:     c->vf25_mv   = s_edit_val;
                          temp_diode_set_calibration(c->vf25_mv, c->tc_mv_c); break;
        case MI_TC:       c->tc_mv_c   = s_edit_val;
                          temp_diode_set_calibration(c->vf25_mv, c->tc_mv_c); break;
        default: break;
    }
    s_dirty = true;
}

/* Kayıt öncesi tutarlılık denetimi — geçersizse mesaj döner, NULL = geçerli  */
static const char* cal_validate(const cal_params_t *c)
{
    int32_t span = c->cap_at_span - c->cap_at_zero;
    if (span < 0) span = -span;
    if (span < CAL_MIN_SPAN_COUNTS) return "CAL INVALID:dC";
    if (c->p_max <= c->p_min)       return "CAL INVALID:P";
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Olay işleyici                                                                */
/* -------------------------------------------------------------------------- */
void sm_handle_event(btn_event_t e)
{
    cal_params_t *c = cal_get_mutable();

    switch (s_state) {
    /* ---------------- NORMAL ---------------- */
    case SM_NORMAL:
        if (e == BTN_EVT_SET_LONG) {
            s_state        = SM_MENU;
            s_menu_idx     = 0;
            s_dirty        = false;
            s_confirm_exit = false;
            s_info_msg     = "";
        }
        break;

    /* ---------------- MENU ---------------- */
    case SM_MENU:
        if (e == BTN_EVT_UP_SHORT || e == BTN_EVT_UP_LONG) {
            s_menu_idx = (s_menu_idx + MI__COUNT - 1) % MI__COUNT;
            s_confirm_exit = false;
            s_info_msg     = "";
        } else if (e == BTN_EVT_DN_SHORT || e == BTN_EVT_DN_LONG) {
            s_menu_idx = (s_menu_idx + 1) % MI__COUNT;
            s_confirm_exit = false;
            s_info_msg     = "";
        } else if (e == BTN_EVT_SET_SHORT) {
            menu_item_t mi = (menu_item_t)s_menu_idx;
            if (mi == MI_CAL_ZERO || mi == MI_CAL_SPAN) {
                s_editing = mi;
                reset_stab_window();
                s_info_msg = "";
                s_state    = SM_CAL_LIVE;
            } else if (mi == MI_SAVE_EXIT) {
                const char *err = cal_validate(c);
                if (err) {
                    s_info_msg = err;          /* kaydetme, menüde kal       */
                } else if (cal_save()) {
                    s_info_msg = "";
                    s_state = SM_NORMAL;
                } else {
                    s_info_msg = "FLASH WRITE ERR";
                }
            } else if (mi == MI_EXIT) {
                if (s_dirty && !s_confirm_exit) {
                    /* Çift onay: değişiklikler atılacak                      */
                    s_confirm_exit = true;
                    s_info_msg     = "Discard? SET again";
                } else {
                    /* RAM kopyasını flash'tan geri yükle (iptal)             */
                    cal_init();
                    temp_diode_set_calibration(c->vf25_mv, c->tc_mv_c);
                    s_confirm_exit = false;
                    s_info_msg     = "";
                    s_state = SM_NORMAL;
                }
            } else {
                s_info_msg = "";
                enter_edit(mi);
            }
        }
        break;

    /* ---------------- EDIT FLOAT ---------------- */
    case SM_EDIT_FLOAT:
        if (e == BTN_EVT_UP_SHORT || e == BTN_EVT_UP_LONG) {
            s_edit_val += s_edit_step;
        } else if (e == BTN_EVT_DN_SHORT || e == BTN_EVT_DN_LONG) {
            s_edit_val -= s_edit_step;
        } else if (e == BTN_EVT_SET_SHORT) {
            commit_edit();
            s_state = SM_MENU;
        } else if (e == BTN_EVT_SET_LONG) {
            /* iptal */
            s_state = SM_MENU;
        }
        break;

    /* ---------------- CAL LIVE (Zero/Span yakalama) ---------------- */
    case SM_CAL_LIVE:
        if (e == BTN_EVT_SET_LONG) {
            if (!sm_cal_live_stable()) {
                s_info_msg = MSG_UNSTABLE;        /* yakalama reddi, kal     */
                break;
            }
            if (s_editing == MI_CAL_ZERO) {
                c->cap_at_zero = s_live_dc;
                s_dirty    = true;
                s_info_msg = "ZERO CAPTURED";
                s_state    = SM_MENU;
            } else if (s_editing == MI_CAL_SPAN) {
                int32_t span = s_live_dc - c->cap_at_zero;
                if (span < 0) span = -span;
                if (span < CAL_MIN_SPAN_COUNTS) {
                    s_info_msg = "SPAN TOO CLOSE";  /* red, CAL_LIVE'da kal  */
                    break;
                }
                c->cap_at_span = s_live_dc;
                s_dirty    = true;
                s_info_msg = "SPAN CAPTURED";
                s_state    = SM_MENU;
            }
        } else if (e == BTN_EVT_SET_SHORT) {
            s_info_msg = "";
            s_state = SM_MENU;  /* yakalamadan çık */
        }
        break;

    default:
        s_state = SM_NORMAL;
        break;
    }
    (void)c;
}
