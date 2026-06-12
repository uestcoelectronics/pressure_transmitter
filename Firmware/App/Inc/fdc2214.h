/**
 * fdc2214.h
 * TI FDC2214 — 4-kanal 28-bit kapasitans-dijital dönüştürücü.
 *
 * Bu projedeki kullanım:
 *   CH2 (IN2B = LP, IN2A = floating)   — Low-pressure side
 *   CH3 (IN3B = HP, IN3A = floating)   — High-pressure side
 *
 * Single-ended (differential değil). External clock X403 (CLK_EN pin'i ile
 * enable), tipik 40 MHz. Continuous conversion mode CH2 + CH3.
 *
 * Yalın API: init() → start() → her okumada read_delta() çağır.
 */
#ifndef FDC2214_H
#define FDC2214_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* I2C 7-bit adres: ADDR=0 → 0x2A, ADDR=1 → 0x2B. Şemaya göre teyit edilmeli. */
#define FDC2214_I2C_ADDR_7B   0x2Au

/* Sensor reset + register programlama. Ext osc çalışıyor olmalı (CLK_EN HIGH).
 * return true → başarılı, device ID doğru                                    */
bool     fdc2214_init(void);

/* Continuous conversion mode'u başlat (CH2 + CH3 sequential).               */
bool     fdc2214_start(void);

/* Son okunan CH2 / CH3 28-bit raw değerleri. Bağımsız okunabilir.           */
bool     fdc2214_read_ch2(uint32_t *raw28);
bool     fdc2214_read_ch3(uint32_t *raw28);

/* Delta C = CH3(HP) - CH2(LP) — int32 cast edilmiş. */
bool     fdc2214_read_delta(int32_t *delta);

/* Hata bayrağı (ERRB pin'i ya da status register sonucu). */
bool     fdc2214_has_error(void);
void     fdc2214_clear_error(void);

#ifdef __cplusplus
}
#endif
#endif
