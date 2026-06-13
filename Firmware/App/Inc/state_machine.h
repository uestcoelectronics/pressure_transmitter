/**
 * state_machine.h
 * Kullanıcı arayüzü state machine'i.
 *
 *  NORMAL                          : ölçüm + ekran (ana çalışma)
 *  ├─ SET long (≥600 ms)           : MENU_ROOT
 *
 *  MENU_ROOT                       : menü öğeleri arasında dolaş
 *  Öğeler:
 *      1) Cal Zero   (sensör low ref'te → ΔC kaydet)
 *      2) Cal Span   (sensör high ref'te → ΔC kaydet)
 *      3) P_min set  (mühendislik birimi)
 *      4) P_max set  (mühendislik birimi)
 *      5) Damping    (saniye)
 *      6) k_T        (sıcaklık katsayısı)
 *      7) Vf25 / TC  (diyot kalibrasyonu)
 *      8) Save & Exit
 *      9) Exit (kaydetme)
 *  ├─ UP/DOWN: öğeyi değiştir
 *  ├─ SET short: seçili öğeye gir
 *
 *  EDIT alt-state: float değer girişi (UP/DOWN ile +/-, SET ile kaydet)
 *      veya CAL_LIVE alt-state: sensör değerlerini canlı göster, SET long ile yakala
 */
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>
#include <stdbool.h>
#include "buttons.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SM_NORMAL = 0,
    SM_MENU,
    SM_EDIT_FLOAT,
    SM_CAL_LIVE,
    SM_SAVE_PENDING
} sm_state_t;

#define SM_NORMAL_PAGES   3u    /* NORMAL ekran sayfaları: MAIN/SENSOR/LOOP   */

void        sm_init(void);

/* Bir buton olayı işle. */
void        sm_handle_event(btn_event_t e);

/* Periyodik servis (5 ms tikte çağrılır): zaman tabanını günceller ve
 * menü/edit/cal ekranlarında 60 s eylemsizlik sonrası NORMAL'e döner
 * (kaydedilmemiş değişiklikler iptal edilir).                              */
void        sm_tick(uint32_t now_ms);

/* NORMAL ekranda aktif sayfa (0..SM_NORMAL_PAGES-1). */
uint8_t     sm_get_normal_page(void);

/* Geçerli backlight yüzdesi (runtime — kalıcı değil, boot'ta %60). */
uint8_t     sm_get_backlight_pct(void);

/* Ana loop'un display ihtiyacı için. */
sm_state_t  sm_get_state(void);
int         sm_get_menu_idx(void);
float       sm_get_edit_value(void);
const char* sm_get_edit_label(void);

/* sm_update_live(deltaC_raw, pressure_eu) — CAL_LIVE state'inde çağrılır. */
void        sm_update_live(int32_t deltaC_raw, float pressure_eu);

/* CAL_LIVE: son örnek penceresi dolu ve ΔC peak-to-peak eşik içinde mi?
 * Yakalama yalnız stabilken kabul edilir.                                  */
bool        sm_cal_live_stable(void);

/* Transient bilgi mesajı (yakalama reddi, kayıt hatası, çıkış onayı...).
 * Boş string = mesaj yok. Menü navigasyonunda temizlenir.                  */
const char* sm_get_info_msg(void);

#ifdef __cplusplus
}
#endif
#endif
