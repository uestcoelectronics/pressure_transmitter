/**
 * temp_diode.h
 * 1N4148 forward-bias sıcaklık sensörleri — SENSÖR KOMPANZASYONU için.
 *
 * Donanım (kullanıcı teyitli, 2026-06-12):
 *   - İki adet 1N4148 (sensör modülü içinde): PC0 = TMP_ADC1, PC1 = TMP_ADC2
 *   - Datasheet: onsemi 1N914-D.PDF (1N91x/1N4x48 ailesi) — DATASHEETS\__TI_DATASHEETS\
 *   - TMP108 ortam izleme AYRI roldedir (tmp108.h) — kompanzasyona girmez.
 *
 * Model: T = 25 + (V_f − V_f25) / TC      (TC negatif: sıcak → V_f düşer)
 * Varsayılanlar (~100 µA bias): V_f25 ≈ 600 mV, TC ≈ −2 mV/°C.
 * Bias direnci şema teyidi (MANUAL-2) sonrası V_f25 hassaslaştırılabilir;
 * her iki parametre menüden ayarlanabilir.
 *
 * Çift kanal arbitrasyonu:
 *   - Kanal geçerlilik: 200 mV ≤ V_f ≤ 1000 mV (açık/kısa devre tespiti)
 *   - İki kanal geçerli ve |T1−T2| ≤ 5 °C → çıkış = ortalama
 *   - Tek kanal geçerli → o kanal + tutarsızlık bayrağı
 *   - Hiçbiri geçerli değil / tutarsız → çıkış son tutarlı değerde tutulur + bayrak
 */
#ifndef TEMP_DIODE_H
#define TEMP_DIODE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Her ADC tarama sonucunda iki diyot kanalıyla çağrılır
 * (raw1 = ADC_RANK_TDIODE / PC0, raw2 = ADC_RANK_TDIODE2 / PC1).            */
void   temp_diode_update(uint16_t adc_raw_d1, uint16_t adc_raw_d2);

/* Kompanzasyon çıkışı (arbitrasyon sonrası, °C).                            */
float  temp_diode_get_celsius(void);

/* Tanı: kanal bazlı son değerler (ch = 0 veya 1).                           */
float  temp_diode_get_ch_celsius(uint8_t ch);
float  temp_diode_get_ch_vf_mv(uint8_t ch);

/* Geriye uyum: kanal 0 V_f değeri.                                          */
float  temp_diode_get_vf_mv(void);

/* true → iki kanal geçerli ve tutarlı (|T1−T2| ≤ eşik).                     */
bool   temp_diode_is_consistent(void);

/* Karakterizasyon parametreleri (iki diyot için ortak).                     */
void   temp_diode_set_calibration(float vf_at_25c_mv, float tc_mv_per_c);
float  temp_diode_get_vf25_mv(void);
float  temp_diode_get_tc_mv_c(void);

#ifdef __cplusplus
}
#endif
#endif
