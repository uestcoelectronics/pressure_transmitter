#include "temp_diode.h"
#include "bsp_pins.h"

/* 1N4148 ~ 100 µA forward current: V_f25 ≈ 600 mV, TC ≈ −2 mV/°C
 * (onsemi 1N914-D.PDF, 1N91x/1N4x48 ailesi). Bias direnci şema teyidi
 * (MANUAL-2) sonrası V_f25 hassaslaştırılmalı; menüden ayarlanabilir.        */
static float s_vf25_mv = 600.0f;
static float s_tc_mv_c = -2.0f;

/* Kanal geçerlilik penceresi: bunun dışı = açık/kısa devre.                  */
#define VF_MIN_MV          200.0f
#define VF_MAX_MV          1000.0f
/* Çapraz makullük eşiği (iki diyot aynı sensör gövdesinde).                  */
#define T_MISMATCH_MAX_C   5.0f

static float s_vf_mv[2]   = { 0.0f, 0.0f };
static float s_temp_c[2]  = { 25.0f, 25.0f };
static bool  s_valid[2]   = { false, false };
static bool  s_consistent = false;
static float s_temp_out_c = 25.0f;    /* arbitrasyon sonrası çıkış           */

void temp_diode_set_calibration(float vf_at_25c_mv, float tc_mv_per_c)
{
    s_vf25_mv = vf_at_25c_mv;
    s_tc_mv_c = tc_mv_per_c;
}

float temp_diode_get_vf25_mv(void) { return s_vf25_mv; }
float temp_diode_get_tc_mv_c(void) { return s_tc_mv_c; }

static void update_channel(uint8_t ch, uint16_t adc_raw)
{
    /* ADC mV = adc_raw * VREFP_MV / ADC_FULLSCALE */
    float vf = ((float)adc_raw * (float)VREFP_MV) / (float)ADC_FULLSCALE;
    s_vf_mv[ch]  = vf;
    s_valid[ch]  = (vf >= VF_MIN_MV) && (vf <= VF_MAX_MV);
    /* T = 25 + (Vf - Vf25) / TC   (TC negatif olduğundan sıcak → Vf düşer)  */
    s_temp_c[ch] = 25.0f + (vf - s_vf25_mv) / s_tc_mv_c;
}

void temp_diode_update(uint16_t adc_raw_d1, uint16_t adc_raw_d2)
{
    update_channel(0, adc_raw_d1);
    update_channel(1, adc_raw_d2);

    if (s_valid[0] && s_valid[1]) {
        float diff = s_temp_c[0] - s_temp_c[1];
        if (diff < 0.0f) diff = -diff;
        if (diff <= T_MISMATCH_MAX_C) {
            s_consistent = true;
            s_temp_out_c = 0.5f * (s_temp_c[0] + s_temp_c[1]);
            return;
        }
        /* İki kanal geçerli ama tutarsız: hangisinin doğru olduğu bilinemez —
           çıkış son tutarlı değerde tutulur.                                 */
        s_consistent = false;
        return;
    }

    s_consistent = false;
    if (s_valid[0])      s_temp_out_c = s_temp_c[0];
    else if (s_valid[1]) s_temp_out_c = s_temp_c[1];
    /* ikisi de geçersiz → son değer korunur                                  */
}

float temp_diode_get_celsius(void)            { return s_temp_out_c; }
float temp_diode_get_ch_celsius(uint8_t ch)   { return s_temp_c[ch & 1u]; }
float temp_diode_get_ch_vf_mv(uint8_t ch)     { return s_vf_mv[ch & 1u]; }
float temp_diode_get_vf_mv(void)              { return s_vf_mv[0]; }
bool  temp_diode_is_consistent(void)          { return s_consistent; }
