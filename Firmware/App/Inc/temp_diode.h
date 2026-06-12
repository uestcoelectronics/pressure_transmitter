/**
 * temp_diode.h
 * 1N914 forward-bias sıcaklık sensörü okuma.
 *
 * Devre (kullanıcı tarifi): 3V3 ── 27 kΩ ── katot ── diot ── anot ── GND
 * Bu standart "değil" — okuyabilmek için TMP_ADC1 pininin diyot anodunda
 * olması daha mantıklı (katot toprakta). Devrede ADC pininin tam olarak
 * hangi diot ucuna bağlı olduğunu donanım kontrolünden teyit etmen lazım.
 *
 * Aşağıdaki formül V_f = V_pin (anot-katot arası gerilim) varsayar.
 * V_f25 (T_ref'de Vf, mV) ve TC (mV/°C) parametreleri kalibrasyonla
 * ayarlanabilir; varsayılan değerler 1N914 tipiktir.
 */
#ifndef TEMP_DIODE_H
#define TEMP_DIODE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Yeni bir ADC örneği gelmesi gereken yerden çağrılır. */
void   temp_diode_update_from_adc(uint16_t adc_raw);

/* Son hesaplanan değerler. */
float  temp_diode_get_celsius(void);
float  temp_diode_get_vf_mv(void);

/* Karakterizasyon parametreleri (Vf 25°C'de, TC mV/°C — negatif). */
void   temp_diode_set_calibration(float vf_at_25c_mv, float tc_mv_per_c);

#ifdef __cplusplus
}
#endif
#endif
