/**
 * hart.h
 * HART üst katman — phy + dl + cmds'i birbirine bağlar.
 * pressure_app yalnız bu iki fonksiyonu çağırır (ble_proto deseniyle aynı).
 */
#ifndef HART_H
#define HART_H

#include "hart_cmds.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Modem + UART + cihaz DB kurulumu. pressure_app_init() sonunda çağır.
 * (İçinde ~60 ms bloklayan modem-reset beklemesi var.)                      */
void hart_init(void);

/* Ana döngüden HER geçişte çağır (cevap süresi limiti ~256 ms).
 * live: anlık ölçümler/durum — pressure_app doldurur.                       */
void hart_service(uint32_t now_ms, const hart_live_t *live);

/* Tanı/telemetri için sayaçlar */
uint32_t hart_rx_frames(void);   /* bize gelen geçerli istek sayısı         */
uint32_t hart_tx_frames(void);   /* gönderilen cevap sayısı                 */

#ifdef __cplusplus
}
#endif
#endif /* HART_H */
