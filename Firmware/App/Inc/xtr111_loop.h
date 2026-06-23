/**
 * xtr111_loop.h
 * TI XTR111 üzerinden 4-20 mA loop akımı sürüşü.
 *
 * Donanım transfer fonksiyonu (xtr111_loop.c ile senkron — şema teyidi
 * MANUAL-2 madde 4):
 *   I_OUT = 10 * V_SET / R_SET,  R_SET = R409 = 1.2 kΩ → I[mA] = V_SET × 8.333
 *      V_SET = 0.48 V → 4 mA,  V_SET = 2.40 V → 20 mA
 *   V_SET, DAC1_OUT1'den (12-bit, VREF = 3.3 V) gelir.
 *
 * Readback: INA190 (gain=100, R_shunt=1 Ω) → V_fb[V] = 0.1 × I[mA]
 *      4 mA → 0.40 V → ADC ≈ 496,  20 mA → 2.00 V → ADC ≈ 2481
 *
 * Safe state: loop_enable=0 → XTR111 supply kesilir, output high-impedance.
 * FLT_4-20mA# (PA6) düşerse loop_set_safe_state() çağrılır; 5 s sonra
 * otomatik retry (FLT hâlâ asserted ise ertelenir — EXTI yalnız kenar görür).
 */
#ifndef XTR111_LOOP_H
#define XTR111_LOOP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NAMUR NE43 akım seviyeleri                                               */
#define LOOP_ALARM_LOW_MA    3.6f   /* arıza sinyali (düşük)                */
#define LOOP_SAT_LOW_MA      3.8f   /* ölçüm alt satürasyonu                */
#define LOOP_SAT_HIGH_MA     20.5f  /* ölçüm üst satürasyonu                */
#define LOOP_ALARM_HIGH_MA   21.0f  /* arıza sinyali (yüksek)               */

void   loop_init(void);                /* DAC + enable pin'i başlat       */
void   loop_enable(bool on);           /* PB2 kontrolü                    */
bool   loop_is_enabled(void);

/* mA seti — komut penceresi [ALARM_LOW .. ALARM_HIGH] = 3.6..21.0 mA       */
/* (alarm akımları dahil; ölçüm satürasyonu pressure_to_ma'da uygulanır).   */
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

/* FLT (PA6) düşen-kenar handler (EXTI callback'inden). Enable sonrası settling
   penceresindeki güç-açılış glitch'ini yok sayar; pencere dışı = gerçek arıza. */
void   loop_on_flt_edge(void);

/* BRING-UP: DAC override uygula (g_loop_dbg_dac >=0 ise). 100 ms tikinde çağır. */
void   loop_dbg_apply(void);
bool   loop_is_in_fault(void);
void   loop_clear_fault(void);

/* Periyodik servis (~100 ms çağır): sapma monitörü + fault auto-retry.
 *  - Sapma: enabled+!fault iken |cmd−meas| > 0.3 mA kesintisiz 2 s →
 *    deviation bayrağı (tanı; çıkış DEĞİŞMEZ); eşik altı 2 s → temizlenir.
 *  - Retry: FLT latch'inden 5 s sonra, FLT pini deasserted ise fault
 *    temizlenir ve loop yeniden enable edilir.                             */
void   loop_service(uint32_t now_ms);
bool   loop_has_deviation(void);

#ifdef __cplusplus
}
#endif
#endif
