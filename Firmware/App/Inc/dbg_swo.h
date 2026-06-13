/**
 * dbg_swo.h
 * SWO/ITM üzerinden non-intrusive canlı telemetri (debug).
 *
 * CMSIS ITM_SendChar() kullanır — debugger/SWV bağlı DEĞİLse no-op (ITM
 * TCR.ITMENA=0), sıfır maliyet. Bu yüzden her zaman çağrılabilir.
 *
 * Donanım: PB3 = SYS_JTDO-SWO (AF0, CubeMX). Trace baud'unu debugger
 * (ST-LINK_gdbserver SWV) çekirdek saatinden ayarlar; firmware yalnız
 * ITM port 0'a yazar. Core halt YOK (semihosting'in aksine) → 4-20 mA
 * kontrol döngüsünü bozmaz.
 *
 * Okuma tarafı (host): ST-Link SWV ile ITM port 0 akışı — bkz.
 * mcu_prog_flash_debug.md.
 */
#ifndef DBG_SWO_H
#define DBG_SWO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CoreDebug TRCENA'yı set eder (ITM kullanımına izin). Bir kez çağrılır. */
void dbg_swo_init(void);

/* NUL-sonlu string'i ITM port 0'a yaz (debugger yoksa no-op). */
void dbg_swo_print(const char *s);

/* 1 Hz telemetri satırı: "P=%.3f,T=%.1f,I=%.2f,ST=0x%02X\n".
 * now_ms ile periyodu kendi içinde yönetir (1 s); her loop'ta çağrılabilir. */
void dbg_swo_telemetry(uint32_t now_ms, float p_bar, float t_c, float ma,
                       uint8_t status_byte);

#ifdef __cplusplus
}
#endif
#endif /* DBG_SWO_H */
