#include "buttons.h"
#include "bsp_pins.h"
#include "stm32u3xx_hal.h"

#define DEBOUNCE_MS      20u
#define LONG_PRESS_MS    600u
#define REPEAT_MS        200u

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    bool          pressed;       /* debounce'lu durum */
    uint32_t      edge_ms;       /* son kenar zamanı */
    uint32_t      press_start_ms;
    uint32_t      last_repeat_ms;
    bool          long_fired;
    bool          raw_now;       /* o anki ham durum (aktif-LOW → true=basılı) */
} btn_state_t;

static btn_state_t s_btn[BTN_ID__COUNT];
static volatile btn_event_t s_pending = BTN_EVT_NONE;

static const btn_event_t SHORT_EVT[BTN_ID__COUNT] = {
    BTN_EVT_UP_SHORT, BTN_EVT_DN_SHORT, BTN_EVT_SET_SHORT
};
static const btn_event_t LONG_EVT[BTN_ID__COUNT] = {
    BTN_EVT_UP_LONG,  BTN_EVT_DN_LONG,  BTN_EVT_SET_LONG
};

static void post_event(btn_event_t e)
{
    /* Sadece en son olayı tut — yalın. */
    s_pending = e;
}

static bool raw_pressed(btn_id_t id)
{
    /* Aktif-LOW: pin LOW → basılı */
    return HAL_GPIO_ReadPin(s_btn[id].port, s_btn[id].pin) == GPIO_PIN_RESET;
}

void buttons_init(void)
{
    s_btn[BTN_ID_UP] .port = BTN_UP_PORT;  s_btn[BTN_ID_UP] .pin = BTN_UP_PIN;
    s_btn[BTN_ID_DN] .port = BTN_DN_PORT;  s_btn[BTN_ID_DN] .pin = BTN_DN_PIN;
    s_btn[BTN_ID_SET].port = BTN_SET_PORT; s_btn[BTN_ID_SET].pin = BTN_SET_PIN;
    for (int i = 0; i < BTN_ID__COUNT; ++i) {
        s_btn[i].pressed       = false;
        s_btn[i].edge_ms       = 0;
        s_btn[i].press_start_ms= 0;
        s_btn[i].last_repeat_ms= 0;
        s_btn[i].long_fired    = false;
        s_btn[i].raw_now       = false;
    }
    s_pending = BTN_EVT_NONE;
}

void buttons_on_edge(btn_id_t id)
{
    if ((unsigned)id >= BTN_ID__COUNT) return;
    s_btn[id].edge_ms = HAL_GetTick();
    s_btn[id].raw_now = raw_pressed(id);   /* anlık ham örnek */
}

void buttons_poll(uint32_t now_ms)
{
    for (int i = 0; i < BTN_ID__COUNT; ++i) {
        btn_state_t *b = &s_btn[i];
        bool raw = raw_pressed((btn_id_t)i);

        if (raw != b->pressed) {
            /* Debounce: kenar geçtikten DEBOUNCE_MS sonra kabul et */
            if ((now_ms - b->edge_ms) >= DEBOUNCE_MS) {
                b->pressed = raw;
                if (raw) {
                    b->press_start_ms = now_ms;
                    b->last_repeat_ms = now_ms;
                    b->long_fired     = false;
                } else {
                    /* Bırakma: short_press eğer long fire etmediyse */
                    if (!b->long_fired) {
                        post_event(SHORT_EVT[i]);
                    }
                }
            }
        } else if (b->pressed) {
            /* Hâlâ basılı — long-press / repeat */
            uint32_t held = now_ms - b->press_start_ms;
            if (!b->long_fired && held >= LONG_PRESS_MS) {
                post_event(LONG_EVT[i]);
                b->long_fired = true;
                b->last_repeat_ms = now_ms;
            } else if (b->long_fired && (now_ms - b->last_repeat_ms) >= REPEAT_MS) {
                /* UP/DOWN için tekrar — SET için tekrar üretmiyoruz */
                if (i == BTN_ID_UP || i == BTN_ID_DN) {
                    post_event(LONG_EVT[i]);
                }
                b->last_repeat_ms = now_ms;
            }
        }
    }
}

btn_event_t buttons_get_event(void)
{
    btn_event_t e = s_pending;
    s_pending = BTN_EVT_NONE;
    return e;
}
