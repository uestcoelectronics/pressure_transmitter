/**
 * ble_uart.h
 * DL-CC2340-B BLE modülü — UART taşıma katmanı (USART3).
 *
 * Datasheet (DreamLNK DL-CC2340-B V1.0):
 *   - Varsayılan 115200 8N1; AT formatı "AT+<cmd> p1,p2"→"OK\r\n"/"ERROR:<n>\r\n"
 *   - Pinler: DIO20 UART-TX→MCU RX (PC10), DIO22 UART-RX←MCU TX (PC11),
 *     DIO24 MODE (sleep ctrl, HIGH=wake → BLE_MODE/PB12),
 *     DIO21 AUX (data-ready/busy → BLE_EVENT/PB0), RESET aktif-LOW (PA15 OD),
 *     VCC enable (BLE_PWR_ON/PC2). Reset duration <100 ms.
 *
 * Bu katman yalnız bayt borusu + güç/pin yönetimi sağlar; AT protokolü
 * ve uygulama çerçeveleri CARD-5.2 (ble_proto) sorumluluğundadır.
 *
 * NOT: USART3 NVIC, CubeMX'te enable DEĞİL — ble_uart_init() HAL_NVIC ile
 * açar ve USART3_IRQHandler bu modülde tanımlanır (.ioc değişmez).
 */
#ifndef BLE_UART_H
#define BLE_UART_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Güç sırası + pin kurulumu + USART3 IT RX başlatma. */
void     ble_uart_init(void);

/* Modülü reset'le (RESET aktif-LOW darbe) + reset süresi bekle. */
void     ble_reset(void);

/* TX (bloklamalı, düşük hızlı config trafiği için yeterli).
 * return: gönderilen bayt (timeout'ta < len olabilir).                     */
uint32_t ble_uart_write(const uint8_t *buf, uint32_t len);

/* RX ring buffer'dan tek bayt al. return false → veri yok. */
bool     ble_uart_read_byte(uint8_t *out);

/* RX ring buffer'da bekleyen bayt sayısı. */
uint32_t ble_uart_available(void);

/* AUX (DIO21/BLE_EVENT): modülde okunmayı bekleyen veri var mı (HIGH). */
bool     ble_data_pending(void);

/* RX ring overflow sayacı (tanı). */
uint32_t ble_uart_overflows(void);

#ifdef __cplusplus
}
#endif
#endif /* BLE_UART_H */
