#include "temp_diode.h"
#include "bsp_pins.h"

/* 1N914 ~ 100 µA forward current (3.3 V, 27 kΩ): V_f25 ≈ 600 mV, TC ≈ -2 mV/°C
 * Bu değerler kart kabulü sırasında doğrulanmalı.                            */
static float s_vf25_mv = 600.0f;
static float s_tc_mv_c = -2.0f;

static float s_vf_mv   = 0.0f;
static float s_temp_c  = 25.0f;

void temp_diode_set_calibration(float vf_at_25c_mv, float tc_mv_per_c)
{
    s_vf25_mv = vf_at_25c_mv;
    s_tc_mv_c = tc_mv_per_c;
}

void temp_diode_update_from_adc(uint16_t adc_raw)
{
    /* ADC mV = adc_raw * VREFP_MV / ADC_FULLSCALE */
    s_vf_mv  = ((float)adc_raw * (float)VREFP_MV) / (float)ADC_FULLSCALE;
    /* T = 25 + (Vf - Vf25) / TC   (TC negatif olduğundan sıcak → Vf düşer) */
    s_temp_c = 25.0f + (s_vf_mv - s_vf25_mv) / s_tc_mv_c;
}

float temp_diode_get_celsius(void) { return s_temp_c; }
float temp_diode_get_vf_mv(void)   { return s_vf_mv;  }
