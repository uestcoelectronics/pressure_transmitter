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
    MI_KT,
    MI_VF25,
    MI_TC,
    MI_SAVE_EXIT,
    MI_EXIT,
    MI__COUNT
} menu_item_t;

static const char *MENU_LABELS[MI__COUNT] = {
    "Cal Zero",
    "Cal Span",
    "P_min",
    "P_max",
    "Damping (s)",
    "k_T (bar/C)",
    "Vf25 (mV)",
    "TC (mV/C)",
    "Save & Exit",
    "Exit (no save)"
};

/* -------------------------------------------------------------------------- */
static sm_state_t  s_state    = SM_NORMAL;
static int         s_menu_idx = 0;
static float       s_edit_val = 0.0f;
static float       s_edit_step= 0.1f;
static int32_t     s_live_dc  = 0;
static float       s_live_p   = 0.0f;
static menu_item_t s_editing  = MI__COUNT;

void sm_init(void)
{
    s_state    = SM_NORMAL;
    s_menu_idx = 0;
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
}

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
        case MI_KT:       s_edit_val = c->k_t;       s_edit_step = 0.001f;break;
        case MI_VF25:     s_edit_val = temp_diode_get_vf25_mv(); s_edit_step = 1.0f; break;
        case MI_TC:       s_edit_val = temp_diode_get_tc_mv_c(); s_edit_step = 0.1f; break;
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
        case MI_KT:       c->k_t       = s_edit_val; break;
        case MI_VF25:     temp_diode_set_calibration(s_edit_val,
                                                     temp_diode_get_tc_mv_c()); break;
        case MI_TC:       temp_diode_set_calibration(temp_diode_get_vf25_mv(),
                                                     s_edit_val); break;
        default: break;
    }
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
            s_state    = SM_MENU;
            s_menu_idx = 0;
        }
        break;

    /* ---------------- MENU ---------------- */
    case SM_MENU:
        if (e == BTN_EVT_UP_SHORT || e == BTN_EVT_UP_LONG) {
            s_menu_idx = (s_menu_idx + MI__COUNT - 1) % MI__COUNT;
        } else if (e == BTN_EVT_DN_SHORT || e == BTN_EVT_DN_LONG) {
            s_menu_idx = (s_menu_idx + 1) % MI__COUNT;
        } else if (e == BTN_EVT_SET_SHORT) {
            menu_item_t mi = (menu_item_t)s_menu_idx;
            if (mi == MI_CAL_ZERO || mi == MI_CAL_SPAN) {
                s_editing = mi;
                s_state   = SM_CAL_LIVE;
            } else if (mi == MI_SAVE_EXIT) {
                (void)cal_save();
                s_state = SM_NORMAL;
            } else if (mi == MI_EXIT) {
                /* RAM kopyasını flash'tan yeniden yükle (değişiklikleri iptal et) */
                cal_init();
                s_state = SM_NORMAL;
            } else {
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
            if (s_editing == MI_CAL_ZERO) {
                c->cap_at_zero = s_live_dc;
            } else if (s_editing == MI_CAL_SPAN) {
                c->cap_at_span = s_live_dc;
            }
            s_state = SM_MENU;
        } else if (e == BTN_EVT_SET_SHORT) {
            s_state = SM_MENU;  /* yakalamadan çık */
        }
        break;

    default:
        s_state = SM_NORMAL;
        break;
    }
    (void)c;
}
