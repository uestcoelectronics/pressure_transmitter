/**
 * tmp108.h
 * TI TMP108 (B701) — ORTAM sıcaklığı monitörü.
 *
 * Rol ayrımı (kullanıcı teyitli mimari):
 *   - TMP108  → yalnız ortam sıcaklığı izleme + 60 °C üstü alert.
 *               Basınç kompanzasyonunda KULLANILMAZ.
 *   - 1N4148 diyotlar (PC0/PC1) → sensör sıcaklık kompanzasyonu (temp_diode).
 *
 * ALERT çıkışı FLT_TEMP# (PB5, aktif-LOW, EXTI5) hattına bağlı.
 * Comparator mode + 4 °C histerezis: T > 60 °C'de düşer, ~56 °C'de kalkar.
 * I2C adresi A0 pinine bağlı (0x48..0x4B) — init adres taraması yapar.
 */
#ifndef TMP108_H
#define TMP108_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TMP108_ALERT_HIGH_C   60.0f   /* T_HIGH eşiği                         */

/* Adres tarama + konfigürasyon (continuous, 1 Hz, comparator, POL aktif-LOW,
 * HYS=4 °C) + THIGH/TLOW yazımı ve read-back doğrulaması.
 * return false → cihaz bulunamadı/doğrulanamadı (ölümcül değil: ölçüm sürer,
 * tanı bayrağı tmp108_is_ok() ile izlenir).                                   */
bool    tmp108_init(void);

/* Periyodik ortam sıcaklığı okuma — ~1 Hz çağır. Pin seviyesinden overtemp
 * durumunu da günceller (histeresisle otomatik temizlenme).                  */
void    tmp108_poll(void);

/* Son okunan ortam sıcaklığı (°C). init/okuma başarısızsa son geçerli değer. */
float   tmp108_get_ambient_c(void);

/* FLT_TEMP# EXTI falling kenarından çağrılır (main.c USER CODE 4).           */
void    tmp108_on_alert_edge(void);

/* Ortam aşırı-sıcaklık durumu (60 °C alarmı) — EXTI veya pin seviyesinden.   */
bool    tmp108_overtemp(void);

/* ALERT pininin anlık durumu (aktif-LOW → true = asserted).                  */
bool    tmp108_alert_active(void);

/* Cihaz sağlığı: init başarısı ve son I2C işlemleri.                         */
bool    tmp108_is_ok(void);

/* Tespit edilen 7-bit I2C adresi (init sonrası anlamlı).                     */
uint8_t tmp108_get_addr(void);

#ifdef __cplusplus
}
#endif
#endif /* TMP108_H */
