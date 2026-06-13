/**
 * ble_proto.h
 * DL-CC2340-B BLE konfigürasyon protokolü (ble_uart taşıma katmanı üstünde).
 *
 * İki faz:
 *   1) AT init (non-bloklayan): RESET → ADVNAME → ADVSTA=1 → ENTM.
 *      Her komuta "OK" beklenir (timeout/retry); başarısızsa degraded→RUN
 *      (modül önceden AT+SAVE ile konfigli olabilir).
 *   2) RUN (transparent): CRC16'lı çerçeve protokolü.
 *      Çerçeve: 0xAA | LEN | CMD | PAYLOAD[LEN] | CRC16(hi,lo)
 *               CRC16-CCITT, [LEN,CMD,PAYLOAD] üzerinde.
 *      Komutlar: GET_MEAS / GET_PARAM / SET_PARAM / SAVE / INFO / UNLOCK.
 *      Yazma (SET/SAVE) sabit PIN ile UNLOCK gerektirir.
 *
 * Non-bloklayan: ble_proto_service() ana döngüden çağrılır; 4-20 mA güvenlik
 * döngüsünü durdurmaz.
 */
#ifndef BLE_PROTO_H
#define BLE_PROTO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AT init durum makinesini başlatır (ble_uart_init sonrası çağrılır). */
void ble_proto_init(void);

/* Periyodik servis: AT init ilerletme + transparent çerçeve işleme.
 * p_bar/t_c/ma = anlık ölçümler (GET_MEAS yanıtı için); status_byte =
 * cihaz durum bitleri.                                                      */
void ble_proto_service(uint32_t now_ms, float p_bar, float t_c, float ma,
                       uint8_t status_byte);

/* BLE init AT konfigürasyonu tamamlandı (transparent moda geçildi) mı? */
bool ble_proto_ready(void);

#ifdef __cplusplus
}
#endif
#endif /* BLE_PROTO_H */
