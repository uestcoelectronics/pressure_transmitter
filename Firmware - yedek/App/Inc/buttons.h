/**
 * buttons.h
 * 3 Hall-switch butonu (UP=PA11, DOWN=PA12, SET=PC13). Aktif-LOW.
 *
 * Event modeli — yalın:
 *   BTN_EVT_NONE
 *   BTN_EVT_UP_SHORT     ( <600 ms basıp bırakma)
 *   BTN_EVT_UP_LONG      (≥600 ms basılı tutma — basılı tutarken her 200 ms repeat)
 *   BTN_EVT_DN_SHORT / DN_LONG
 *   BTN_EVT_SET_SHORT / SET_LONG
 *
 * EXTI ISR'larından btn_on_edge() çağrılır; ana loop btn_poll(now_ms) ile
 * süre denetimi yapar; btn_get_event() son üretileni döndürür.
 */
#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BTN_EVT_NONE = 0,
    BTN_EVT_UP_SHORT,
    BTN_EVT_UP_LONG,
    BTN_EVT_DN_SHORT,
    BTN_EVT_DN_LONG,
    BTN_EVT_SET_SHORT,
    BTN_EVT_SET_LONG
} btn_event_t;

typedef enum {
    BTN_ID_UP = 0,
    BTN_ID_DN = 1,
    BTN_ID_SET = 2,
    BTN_ID__COUNT
} btn_id_t;

void        buttons_init(void);

/* EXTI ISR'dan: kenar geldiğinde MCU pin'i okuyup buraya geç. */
void        buttons_on_edge(btn_id_t id);

/* Her ana-loop tick'te (örn. 5 ms) çağrılır. Şu anki ms sayacı verilir. */
void        buttons_poll(uint32_t now_ms);

/* Bekleyen olayı tüketir (yoksa NONE). */
btn_event_t buttons_get_event(void);

#ifdef __cplusplus
}
#endif
#endif
