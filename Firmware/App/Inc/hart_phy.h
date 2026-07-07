/**
 * hart_phy.h
 * HART fiziksel katman — DAC8742H FSK modem (B404, UART modu) + LPUART1.
 *
 * Donanım (PIN_MAPPING "4-20 mA + HART" sütunu, şema sayfa 4):
 *   PA2  LPUART1_TX (AF8) → B404 UART_IN
 *   PA3  LPUART1_RX (AF8) ← B404 UART_OUT
 *   PB1  TI_RTS  (GPIO)   → B404 UART_RTS  (LOW = gönder/modülatör aktif)
 *   PC12 RST#TI  (GPIO)   → B404 RST       (aktif-LOW; LOW = power-down)
 *   PD2  CLK_EN  (GPIO)   → X402 4 MHz XO enable (X403/FDC ile ortak net)
 *   B404 CD çıkışı MCU'ya BAĞLI DEĞİL → taşıyıcı algılama yok; çerçeve
 *   senkronizasyonu karakter-arası gap timeout ile yapılır (hart_dl).
 *   B404 IF_SEL dahili pull-down = UART modu (datasheet §7.3.9).
 *
 * UART: 1200 baud, 8 veri + ODD parity + 1 stop → HAL'de WORDLENGTH_9B.
 * LPUART1 CubeMX'te TANIMLI DEĞİL; bu modül GPIO+UART+NVIC'i kendi kurar
 * ve LPUART1_IRQHandler'ı tanımlar (ble_uart'ın USART3 deseniyle aynı,
 * .ioc değişmez).
 *
 * TX akışı: RTS LOW → ~3 ms taşıyıcı oturma → UART IT gönderim →
 * TC callback'inde RTS HIGH (half-duplex, datasheet §7.4.1).
 */
#ifndef HART_PHY_H
#define HART_PHY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Modem reset + osilatör bekleme + LPUART1 kurulum + IT RX başlat.
 * NOT: içinde ~60 ms bloklayan bekleme var; yalnız init'te çağır.           */
void hart_phy_init(void);

/* RX ring'den bayt çek. false = veri yok.                                   */
bool hart_phy_read_byte(uint8_t *out);

/* Parity/framing hatası oldu mu? (okunduğunda temizlenir)                   */
bool hart_phy_take_rx_error(void);

/* Cevap çerçevesi gönder (non-bloklayan; IT). false = TX meşgul.            */
bool hart_phy_send(const uint8_t *buf, uint16_t len);

bool hart_phy_tx_busy(void);

/* ble_uart.c'deki ortak HAL_UART_RxCpltCallback'ten çağrılır.               */
void hart_phy_on_rx_cplt(void);

#ifdef __cplusplus
}
#endif
#endif /* HART_PHY_H */
