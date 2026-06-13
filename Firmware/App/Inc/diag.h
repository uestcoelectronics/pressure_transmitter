/**
 * diag.h
 * Temel donanım tanıları (FMEDA referansları PIN_MAPPING "Standards Ref"):
 *   - A.13 Analog signal monitoring: ADC kanal rail-stuck (kopuk/mux arızası)
 *   - A.7  Read-back of digital outputs: output GPIO ODR↔IDR karşılaştırma
 *   - I2C bus health + fiziksel kurtarma (9-clock)
 *
 * Bu modül driver'lara DOKUNMAZ; cihaz sağlığını mevcut API'lerle gözler.
 * ADC tanısı divider-bağımsızdır (mutlak gerilim eşiği yok — yalnız rail-stuck).
 */
#ifndef DIAG_H
#define DIAG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Tanı bayrakları (bit maskesi) */
#define DIAG_F_ADC_VCC      (1u << 0)   /* VCC_FB ham ADC rail'e takılı     */
#define DIAG_F_ADC_ILOOP    (1u << 1)   /* loop akım FB ham ADC rail'e takılı*/
#define DIAG_F_GPIO_RBACK   (1u << 2)   /* output GPIO read-back uyuşmazlığı */
#define DIAG_F_I2C_BUS      (1u << 3)   /* I2C bus kurtarma denendi          */

void     diag_init(void);

/* Periyodik tanı (sensör tikinde ham ADC değerleriyle çağrılır). */
void     diag_service(uint16_t adc_vcc_raw, uint16_t adc_iloop_raw);

uint16_t diag_flags(void);
bool     diag_any(void);        /* herhangi bir tanı bayrağı aktif         */
bool     diag_critical(void);   /* güvenlik-kritik (LOOP_EN read-back)      */

/* Fiziksel I2C bus kurtarma: SCL'yi 9 kez clock'la (takılı SDA serbest
 * kalsın), STOP üret, AF'yi geri ver, HAL I2C'yi yeniden başlat.
 * Çağıran sonra I2C cihazlarını yeniden init etmelidir.                    */
void     diag_i2c_bus_recover(void);

#ifdef __cplusplus
}
#endif
#endif /* DIAG_H */
