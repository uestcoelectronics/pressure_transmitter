#include "dbg_swo.h"
#include "stm32u3xx_hal.h"     /* CMSIS: CoreDebug, ITM, ITM_SendChar */
#include <stdio.h>

#define DBG_PERIOD_MS   1000u   /* telemetri satırı periyodu */

void dbg_swo_init(void)
{
    /* TRCENA: trace alt sistemine izin ver. ITM TCR/TER'i debugger (SWV)
       konfigüre eder; ITM_SendChar bağlı değilken zaten no-op'tur.          */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
}

void dbg_swo_print(const char *s)
{
    while (*s) {
        (void)ITM_SendChar((uint32_t)(uint8_t)*s);  /* SWV yoksa no-op */
        s++;
    }
}

void dbg_swo_telemetry(uint32_t now_ms, float p_bar, float t_c, float ma,
                       uint8_t status_byte)
{
    static uint32_t s_last = 0;
    if ((now_ms - s_last) < DBG_PERIOD_MS) return;
    s_last = now_ms;

    char line[56];
    int n = snprintf(line, sizeof line,
                     "P=%.3f,T=%.1f,I=%.2f,ST=0x%02X\n",
                     (double)p_bar, (double)t_c, (double)ma,
                     (unsigned)status_byte);
    if (n > 0) dbg_swo_print(line);
}
