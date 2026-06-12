/**
 * cal_storage.h
 * Kalibrasyon parametrelerinin internal flash'a yazılması/okunması.
 * STM32U385RG: 1 MB flash, 8 KB sayfa boyutu. Son sayfayı (page 127)
 * kalibrasyon için ayırıyoruz. Linker'ı değiştirmiyoruz; sadece
 * runtime'da o adrese yazıyoruz.
 *
 * Format: magic + version + payload + CRC32. CRC bozulursa default'a düşer.
 */
#ifndef CAL_STORAGE_H
#define CAL_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAL_MAGIC      0x43414C30u   /* "CAL0" */
#define CAL_VERSION    2u            /* v2: k_t_span + vf25/tc persistansı   */

typedef struct {
    /* Kapasitans (raw — FDC2214'ten gelen 28-bit count olarak) */
    int32_t  cap_at_zero;      /* P_min uygulandığında okunan ΔC */
    int32_t  cap_at_span;      /* P_max uygulandığında okunan ΔC */

    /* Mühendislik birimleri (kullanıcının atadığı) */
    float    p_min;            /* 4 mA'ya karşılık gelen basınç (bar) */
    float    p_max;            /* 20 mA'ya karşılık gelen basınç (bar) */

    /* Sıcaklık kompansasyonu v2 (zero + span ayrı):
     *   ΔT = T − T_ref,  frac = ham okuma oranı (0..1)
     *   P_comp = P_raw − (k_t_zero + k_t_span · frac) · ΔT
     * k_t_span=0 ile v1 davranışına eşdeğer.                              */
    float    k_t_zero;         /* bar / °C (v1'deki k_t), default 0.0 */
    float    t_ref;            /* °C, default 25.0 */

    /* Damping (IIR low-pass time constant, saniye) */
    float    damping_s;        /* default 0.5 s */

    /* --- v2 alanları --- */
    float    k_t_span;         /* bar / °C @ tam skala, default 0.0 */
    float    vf25_mv;          /* 1N4148 V_f @25 °C (mV), default 600 */
    float    tc_mv_c;          /* 1N4148 TC (mV/°C, negatif), default −2 */
} cal_params_t;

/* Modülü init et: flash'tan okumayı dener; CRC bozuksa default yükler. */
void          cal_init(void);

/* Aktif parametre setine pointer döner — read-only. */
const cal_params_t* cal_get(void);

/* RAM içindeki kopyayı değiştirmek için (menüden ayar yaparken). */
cal_params_t* cal_get_mutable(void);

/* RAM'deki cal_params_t'yi flash'a yazar. Başarılı=true.        */
bool          cal_save(void);

/* Fabrika ayarlarına döner (RAM'de). cal_save() ile yazılır.    */
void          cal_load_defaults(void);

#ifdef __cplusplus
}
#endif
#endif /* CAL_STORAGE_H */
