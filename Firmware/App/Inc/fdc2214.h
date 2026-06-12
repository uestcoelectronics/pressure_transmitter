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

/* I2C 7-bit adres: ADDR=0 → 0x2A, ADDR=1 → 0x2B. Şema teyidine kadar init()
 * her ikisini de dener; kullanılan adres fdc2214_get_addr() ile okunur.       */
#define FDC2214_I2C_ADDR_7B   0x2Au
#define FDC2214_I2C_ADDR_ALT  0x2Bu

/* Güç/saat sıralaması: CLK_EN HIGH → osc oturma → SD release → oturma.
 * init() içinden çağrılır; başarısız init sonrası retry için ayrıca da
 * çağrılabilir (SD toggle ile cihaz state'i sıfırlanır).                      */
void     fdc2214_power_sequence(void);

/* Sensor reset + register programlama. Güç sıralamasını kendisi uygular,
 * DEV_ID'yi 0x2A → 0x2B sırasıyla arar.
 * return true → başarılı, device ID doğru                                    */
bool     fdc2214_init(void);

/* Tespit edilen 7-bit I2C adresi (init sonrası anlamlı).                     */
uint8_t  fdc2214_get_addr(void);

/* ERRB pinini yokla (aktif-LOW). Asserted ise STATUS register'ı okunur
 * (okuma flag'leri temizler) ve dahili hata bayrağı kurulur.
 * return true → ERRB şu an asserted                                          */
bool     fdc2214_poll_errb(void);

/* INT_B pini (aktif-LOW): yeni dönüşüm verisi hazır mı?                      */
bool     fdc2214_data_ready(void);

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
