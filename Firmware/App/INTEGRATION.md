# PressureTx 4-20 mA — Firmware Entegrasyon Rehberi

## 1. CubeMX projesini üret

`Firmware/PressureTx_4_20.ioc` dosyasını oluşturduktan ve **Generate Code** dedikten sonra şu klasör yapısı oluşur:

```
Firmware/
  PressureTx_4_20/
    Core/  Drivers/  cmake/  CMakeLists.txt  PressureTx_4_20.ioc
```

## 2. App/ klasörünü taşı

`Firmware/App/` klasörünü `Firmware/PressureTx_4_20/App/` altına taşı:

```
PressureTx_4_20/
  App/
    Inc/  (9 başlık dosyası)
    Src/  (8 kaynak dosya)
```

## 3. CMakeLists.txt'e ekleme

`PressureTx_4_20/CMakeLists.txt` içinde `add_executable(...)` satırından sonra şunu ekle:

```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/App/Src/fdc2214.c
    ${CMAKE_SOURCE_DIR}/App/Src/xtr111_loop.c
    ${CMAKE_SOURCE_DIR}/App/Src/temp_diode.c
    ${CMAKE_SOURCE_DIR}/App/Src/lcd400.c
    ${CMAKE_SOURCE_DIR}/App/Src/buttons.c
    ${CMAKE_SOURCE_DIR}/App/Src/cal_storage.c
    ${CMAKE_SOURCE_DIR}/App/Src/state_machine.c
    ${CMAKE_SOURCE_DIR}/App/Src/pressure_app.c
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/App/Inc
)
```

## 4. main.c değişiklikleri

CubeMX `main.c`'de **USER CODE** bölümlerini bozmaz. Şu üç ekleme yeter:

**Includes (USER CODE BEGIN Includes):**
```c
#include "pressure_app.h"
#include "buttons.h"
#include "xtr111_loop.h"
```

**Init (USER CODE BEGIN 2 — peripheral init'lerden sonra):**
```c
pressure_app_init();
```

**Main loop (while(1) içine):**
```c
while (1) {
    pressure_app_loop();
    /* USER CODE BEGIN 3 */
}
```

**EXTI callback (USER CODE BEGIN 4):**
```c
void HAL_GPIO_EXTI_Falling_Callback(uint16_t pin)
{
    switch (pin) {
        case GPIO_PIN_11: buttons_on_edge(BTN_ID_UP);  break;
        case GPIO_PIN_12: buttons_on_edge(BTN_ID_DN);  break;
        case GPIO_PIN_13: buttons_on_edge(BTN_ID_SET); break;
        case GPIO_PIN_6:  loop_set_safe_state();       break;  /* PA6 FLT */
        default: break;
    }
}
```
(pressure_app.c içindeki __weak versiyon yerine main.c'deki güçlü tanım kullanılacak.)

## 5. CubeMX'te kontrol etmen gereken ayarlar

- **ADC1**: DMA Continuous Requests = Enabled, DMA Mode = Circular ya da Normal (ikisi de çalışır), regular rank sırası **PC0 → PC4 → PC5** olmalı (`bsp_pins.h` içindeki ADC_RANK_* sırasıyla aynı).
- **DAC1_OUT1**: Buffer = Enabled.
- **TIM1**: PWM Generation CH1, Counter Period (ARR) = 99 (yani 100 step), CKD prescaler = uygun (örn. 48 MHz → PSC=47 → 1 MHz → 10 kHz PWM).
- **SPI2**: Data Size = 8 bit, MSB first, Mode 0 (CPOL=Low, CPHA=1 Edge), prescaler ile baud ~5 MHz.
- **I2C1**: Standard veya Fast Mode (400 kHz), tüm cihazlar pull-up'ı PCB'den alıyor.

## 6. Build & flash

VS Code'da:
```
cmake -S . -B build -G Ninja
cmake --build build
```
veya STM32 VS Code extension'ın "Build" butonu. ST-Link ile flash et.

## 7. İlk açılış prosedürü

1. **DAC ve I_OUT doğrulaması** (yükten önce):
   - Loop konektörüne 250 Ω yük + 24 V seri ölç (multimetre A modunda).
   - Boot anında loop disabled (3.6 mA değil 0 mA görmelisin, çünkü PB2=LOW = XTR111 supply off).
   - `pressure_app_init` sonunda `loop_enable(true)` çağrılıyor; sensör başarılı ise 4 mA komutu görünmeli.
2. **Buton testi**:
   - SET'i uzun bas (≥600 ms) → menüye girmeli.
   - UP/DOWN ile gez, SET kısa bas ile gir.
3. **Kalibrasyon prosedürü**:
   - Menü → `P_min` → değer ata (örn. 0.0) → SET kısa = kaydet.
   - Menü → `P_max` → değer ata (örn. 10.0) → SET kısa = kaydet.
   - Sensöre **düşük basıncı uygula** → Menü → `Cal Zero` → ekran ΔC değerini canlı gösterir → SET uzun bas = yakala.
   - Sensöre **yüksek basıncı uygula** → Menü → `Cal Span` → SET uzun bas = yakala.
   - Menü → `Save & Exit` → SET kısa = flash'a yaz, NORMAL'a dön.
4. **Loop çıkış doğrulaması**:
   - Düşük basınçta loop ~4 mA okumalı (multimetre).
   - Yüksek basınçta loop ~20 mA okumalı.
5. **Damping testi**:
   - `Damping (s)` değerini 2.0 yap, basıncı hızlı değiştir; çıkışın yavaşladığını gör.

## 8. Bilinen TODO'lar (bir sonraki turda)

- `lcd400.c` içindeki `lcd_send_init_seq` ve `lcd_flush` controller-spesifik komutlarla doldurulmalı (LCD400 datasheet'i kontrolör seçtikten sonra — `BDS154S10Z0TG01` controller'ını teyit etmek gerekiyor; ST7789 mu, başka mı?).
- 1N914 diyot bağlantısının ADC pin'inin **anot** tarafında olduğunu donanım kontrolünden teyit et.
- FDC2214 I2C ADDR pin'inin (0x2A veya 0x2B) şemadaki bağlantısını teyit et — gerekirse `FDC2214_I2C_ADDR_7B` makrosunu güncelle.
- 4-20 mA loop dış zener clamping (D402) sayesinde aşırı gerilimden korumalı, ama yine de loop konektörüne ilk takmadan önce 24 V'u **yavaşça** yükselt.
