/**
 * xtr111_loop.h
 * TI XTR111 üzerinden 4-20 mA loop akımı sürüşü.
 *
 * Donanım transfer fonksiyonu:
 *   I_loop = 10 * V_SET / R_SET,   R_SET = 15 Ω  →  I = V_SET / 1.5
 *      V_SET = 0.6 V → 4 mA
 *      V_SET = 3.0 V → 20 mA
 *   V_SET, DAC1_OUT1'den (12-bit, VREF = 3.3 V) gelir.
 *
 * Readback: INA190 (gain=100, R_shunt=1 Ω) → V_fb = 100 * 1 * I = 100*I
 *      4 mA → 0.40 V → ADC ≈ 496
 *      20 mA → 2.00 V → ADC ≈ 2481
 *
 * Safe state: loop_enable=0 → XTR111 supply kesilir, output high-impedance.
 * FLT_4-20mA# (PA6) düşerse loop_set_safe_state() çağrılmalı.
 */
#ifndef XTR111_LOOP_H
#define XTR111_LOOP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void   loop_init(void);                /* DAC + enable pin'i başlat       */
void   loop_enable(bool on);           /* PB2 kontrolü                    */
bool   loop_is_enabled(void);

/* mA seti (3.8..20.5 mA dış değerler clamp edilir; alarm akımları için    */
/* HART uyumlu 3.6 mA / 22 mA tercih edilirse extend edilebilir).          */
void   loop_set_current_ma(float ma);

/* Son komut edilen mA değeri */
float  loop_get_commanded_ma(void);

/* INA190 ADC raw'dan ölçülen mA */
void   loop_update_feedback_from_adc(uint16_t adc_raw);
float  loop_get_measured_ma(void);

/* Komut vs ölçüm sapması (mA cinsinden) — diagnostik. */
float  loop_get_error_ma(void);

/* FLT_4-20mA# pin'i geçince çağrılır — loop'u devre dışı bırakır + bayrak. */
void   loop_set_safe_state(void);
bool   loop_is_in_fault(void);
void   loop_clear_fault(void);

#ifdef __cplusplus
}
#endif
#endif
