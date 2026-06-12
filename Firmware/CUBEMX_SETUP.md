# STM32CubeMX Kurulum Rehberi — PressureTx 4-20 mA

CubeMX **6.x** versiyonu varsayılmıştır. STM32U3 ailesi için en az CubeMX **6.11+** lazım — yoksa STM32U385 görünmez.

## 0. Hazırlık

- STM32CubeMX 6.11 (veya üstü) açık
- File → Updater Settings → "Manage embedded software packages" → **STM32CubeU3** firmware paketi yüklü
- Proje hedef dizini hazır: `C:\PressureTransmitter\Firmware\`

## 1. Yeni proje aç

1. Açılış ekranında **ACCESS TO MCU SELECTOR**
2. Üstteki arama kutusuna: `STM32U385RGT`
3. Listeden **STM32U385RGTx** (LQFP64) seç
4. Sağ üstte **Start Project**

## 2. Pinout & Configuration — sol panelden Categories

CubeMX açıldığında ana ekran **Pinout & Configuration** tab'ında olacak. Sol tarafta **Categories** listesi (System Core / Analog / Timers / Connectivity / Computing / Security…) görünür. Her peripheral'a tek tek girilecek.

> **Genel kural**: bir peripheral'i enable ettiğinde MCU resmi üstündeki pin'ler yeşile döner. Bu adımlar bittikten sonra resmi kontrol et — tabloda söylediğim pin'lerin hepsi yeşil ve doğru fonksiyonda olmalı.

### 2.1 System Core → RCC

- **High Speed Clock (HSE)**: `Crystal/Ceramic Resonator`
- **Low Speed Clock (LSE)**: `Crystal/Ceramic Resonator`

Bu sayede PC14/PC15 ve PH0/PH1 otomatik osc giriş/çıkışına atanır.

### 2.2 System Core → SYS

- **Debug**: `Serial Wire`
- **Timebase Source**: `TIM6`  (HAL tick'i SysTick yerine TIM6'ya bırakıyoruz — RTOS uyumluluğu için iyi alışkanlık)

### 2.3 System Core → GPIO

Burada **manual** olarak bazı pin'leri ayarlayacağız. **MCU resmi üstüne tıklayarak** pin'leri seç ve her birinde aşağıdaki konfigürasyonu yap. Her pin için aynı diyalog açılır — alanları doldur, sonra GPIO sekmesinde detayları gör.

**GPIO Output (Push-Pull) pin'leri** — her birine tıklayıp pull-down ile aç:

| Pin | User Label | Output Level | Mode | Pull | Speed |
|---|---|---|---|---|---|
| PC2 | `BLE_PWR_ON` | Low | Output PP | No pull | Low |
| PC3 | `WDI` | Low | Output PP | No pull | Low |
| PB2 | `LOOP_EN` | Low | Output PP | No pull | Low |
| PB10 | `LED` | Low | Output PP | No pull | Low |
| PB12 | `BLE_MODE` | Low | Output PP | No pull | Low |
| PA10 | `LCD_PWR_ON` | Low | Output PP | No pull | Low |
| PC8 | `LCD_RST` | Low | Output PP | No pull | Low |
| PC6 | `LCD_CS` | High | Output PP | No pull | High |
| PC7 | `LCD_DC` | Low | Output PP | No pull | Low |
| PC9 | `LCD_CS2` | High | Output PP | No pull | Low |
| PC12 | `RST_TI` | High | Output PP | No pull | Low |
| PD2 | `FDC_CLK_EN` | High | Output PP | No pull | Low |
| PB6 | `FDC_SD` | Low | Output PP | No pull | Low |

**GPIO Output (Open-Drain)** — bir tek:

| Pin | User Label | Output Level | Mode | Pull |
|---|---|---|---|---|
| PA15 | `BLE_RESET` | High | Output OD | No pull |

**GPIO Input + EXTI** — her birini "External Interrupt Mode with Falling edge trigger" yap, pull-up aktif:

| Pin | User Label | Mode | Pull |
|---|---|---|---|
| PA0 | `FDC_DRDY` | EXTI Falling | No pull (harici) |
| PA1 | `FDC_ERRB` | EXTI Falling | No pull |
| PA6 | `LOOP_FLT` | EXTI Falling | **Pull-up** |
| PA11 | `BTN_UP` | EXTI Falling | **Pull-up** |
| PA12 | `BTN_DN` | EXTI Falling | **Pull-up** |
| PC13 | `BTN_SET` | EXTI Falling | **Pull-up** |
| PB0 | `BLE_EVENT` | EXTI Falling | No pull |
| PB5 | `FLT_TEMP` | EXTI Falling | No pull |
| PB7 | `FDC_INT` | EXTI Falling | No pull |

**GPIO Input (no EXTI)** — sadece okunan pin'ler:

| Pin | User Label | Mode | Pull |
|---|---|---|---|
| PB4 | `TEST_MODE` | Input | **Pull-down** |

> **İpucu**: Pin'e tıklayıp "GPIO_EXTI…" seçtikten sonra **GPIO** kategorisine girip o pin'i seç, "User Label" alanına yukarıdaki ismi yaz — kod üretildiğinde `BTN_UP_Pin`, `BTN_UP_GPIO_Port` gibi makrolar otomatik oluşur.

### 2.4 Connectivity → I2C1

- Mode: `I2C`
- **Parameter Settings** sekmesi:
  - I2C Speed Mode: `Fast Mode`
  - I2C Speed: `400 kHz`
  - Address Length: `7-bit`
  - Geri kalan default
- Pin'ler otomatik olarak **PB8 = SCL**, **PB9 = SDA** olur. Eğer farklı bir PB6/PB7 önerilirse, MCU resminden manuel olarak PB8 ve PB9'a alternatif fonksiyonu zorla.
- **NVIC Settings** sekmesi: `I2C1 event interrupt` ve `I2C1 error interrupt` — şimdilik **disable** (polling kullanıyoruz).

### 2.5 Connectivity → SPI2

- Mode: `Full-Duplex Master`
- Hardware NSS: `Disable` (CS'i manuel sürüyoruz — PC6)
- **Parameter Settings**:
  - Frame Format: `Motorola`
  - Data Size: `8 Bits`
  - First Bit: `MSB First`
  - Prescaler: `8` (PCLK=48 MHz ise SPI clock = 6 MHz — ST7789V için güvenli)
  - CPOL: `Low`
  - CPHA: `1 Edge`
  - CRC Calculation: `Disabled`
  - NSSP Mode: `Disabled`
- Pin'ler: **PB13 = SCK**, **PB14 = MISO**, **PB15 = MOSI** otomatik.
- NVIC: disabled (polling transmit).

### 2.6 Analog → ADC1

- Mode: paneldeki kanalları tek tek aç:
  - **IN1 Single-ended** (PC0 için)
  - **IN11 Single-ended** (PC4 için)
  - **IN12 Single-ended** (PC5 için)
- **Parameter Settings**:
  - ADCs_Common_Settings → Clock Prescaler: `Synchronous clock mode divided by 4`
  - ADC_Settings:
    - Resolution: `12-bit`
    - Data Alignment: `Right alignment`
    - Scan Conversion Mode: `Enabled`
    - Continuous Conversion Mode: `Disabled`
    - Discontinuous Conversion Mode: `Disabled`
    - DMA Continuous Requests: `Enabled`
    - End Of Conversion Selection: `End of sequence of conversion`
    - Overrun behaviour: `Overrun data overwritten`
    - Low Power Auto Wait: `Disabled`
  - ADC_Regular_ConversionMode:
    - Number Of Conversion: `3`
    - External Trigger Conversion Source: `Regular Conversion launched by software`
    - External Trigger Conversion Edge: `None`
    - **Rank 1 → Channel 1 (PC0)**, Sampling Time: `247.5 Cycles`
    - **Rank 2 → Channel 11 (PC4)**, Sampling Time: `247.5 Cycles`
    - **Rank 3 → Channel 12 (PC5)**, Sampling Time: `247.5 Cycles`

**GPDMA1 ön gereksinim**: System Core → GPDMA1 kategorisine bir kez gir (içerideki 12 kanalı `Disable`'da bırak — sadece enable olması yeter).

- **DMA Settings** sekmesi (ADC1 içinde) → **Add** butonu:
  - DMA Request: `ADC1`
  - Channel: `GPDMA1 Channel 0` (CubeMX otomatik atar)
  - Direction: `Peripheral To Memory`
  - Mode: `Normal`
  - Increment Address: Peripheral **unchecked**, Memory **checked**
  - Data Width: Peripheral = `Half Word`, Memory = `Half Word`
  - Priority: `Low`
- **NVIC Settings** sekmesi: GPDMA1 Channel 0 global interrupt = **enable**, ADC1 global interrupt = **enable**.

### 2.7 Analog → DAC1

- Tikle: `OUT1 mode → Connected to external pin only`
- **Parameter Settings**:
  - DAC Out1 Settings:
    - Output Buffer: `Enable`
    - Trigger: `None`
    - User Trimming: `Factory Trimming`
- Pin: PA4 otomatik atanır.

### 2.8 Timers → TIM1

- Clock Source: `Internal Clock`
- Channel1: `PWM Generation CH1`
- **Parameter Settings**:
  - Counter Settings:
    - Prescaler (PSC): `47` (PCLK=48 MHz → 1 MHz tick)
    - Counter Mode: `Up`
    - Counter Period (ARR): `99` (1 MHz / 100 = 10 kHz PWM, 100 step duty)
    - Internal Clock Division: `No Division`
    - Auto-Reload Preload: `Enable`
  - PWM Generation Channel 1:
    - Mode: `PWM mode 1`
    - Pulse: `60` (boot duty %60)
    - Output compare preload: `Enable`
    - Fast Mode: `Disable`
    - CH Polarity: `High`
- Pin PA8 otomatik atanır.

### 2.9 Timers → TIM6

**Bu adımı ATLA**. SYS → Timebase Source = TIM6 seçtiğin için CubeMX TIM6'yı otomatik olarak rezerve etti ve 1 ms HAL tick üretecek şekilde kendiliğinden ayarlıyor. Listede **sönük (greyed-out)** görünmesi normal — elle ayar yapmana gerek yok, NVIC otomatik enable.

### 2.10 Connectivity → USART3 (BLE — opsiyonel ama hazır bırak)

- Mode: `Asynchronous`
- Baud Rate: `115200`, Word: 8, Parity: None, Stop: 1
- Pin'ler **PC10 (RX), PC11 (TX)** otomatik.
- NVIC: USART3 global interrupt — şimdilik disable.

### 2.11 System Core → IWDG (önerilir — emniyet için)

- Activated
- Prescaler: `64`, Reload: `1875` → ~3 saniye watchdog timeout
- Window: disabled

> **Not**: IWDG'yi etkin edersen `pressure_app.c` üst tarafına `#define USE_IWDG 1` ekle (veya CMakeLists'te `-DUSE_IWDG=1`). Yoksa `HAL_IWDG_Refresh` kısmı zaten `#ifdef` ile gizlidir.

### 2.12 NVIC genel kontrol

**System Core → NVIC** sekmesine geç ve aşağıdaki kesmelerin aktif olduğunu **enabled** sütununda doğrula:

- EXTI Line 0 interrupt (PA0 FDC_DRDY)
- EXTI Line 1 interrupt (PA1 FDC_ERRB)
- EXTI Line 4 interrupt (PB4? — hayır, PB4 input. PA6 için **EXTI Line 5..9**? PA6 = EXTI6 → `EXTI Line 4..15` grup interrupt'inde olur. STM32U3'te EXTI çoğunlukla pin başına ayrı veya 4-15 grup şeklindedir.)
- **EXTI Line 5..9** (PA6, PB5, PB7 → 6,5,7 hattı)
- **EXTI Line 10..15** (PA11, PA12, PC13 → 11,12,13 hattı)
- EXTI Line 0 (PB0) — yukarıdaki Line 0 ile aynı interrupt vector (PA0 ve PB0 ikisi de EXTI0)
- GPDMA1 Channel 0 global interrupt
- ADC1 global interrupt
- TIM6 global interrupt

**Preemption priority** hepsi `5`, sub priority `0` ile bırakabilirsin — RTOS yok, hepsi eşit yarışta sorun değil.

## 3. Clock Configuration

STM32U3 ailesinde HSE → PLL → SYSCLK akışı belirgin değil. Yerine **MSIS** (Multi-Speed Internal) osilatörü kullanıyoruz — kart üstündeki 24 MHz HSE crystal'ı RTC/CSS yedeği olarak duruyor.

Üstteki **Clock Configuration** sekmesine geç.

1. **MSIS Source** kutusuna `96` yaz (MHz)
2. **System Clock Mux** seçicisinde **MSIS** yuvarlağını seç
3. **AHB Prescaler** = `/2` → **HCLK = 48 MHz** ✓
4. **APB1/APB2/APB3 Prescaler** hepsi `/1` → tüm peripheral clock'ları 48 MHz
5. **DAC Clock Mux**: LSI (default) — DAC sample-hold için yeterli
6. **I2C1 Clock Mux**: `PCLK1` → 48 MHz
7. **USART3 Clock Mux**: `PCLK1` → 48 MHz
8. Sağ üstte sarı/kırmızı uyarı çıkmamalı; tüm "To … (MHz)" kutuları `48` veya buna oranlı değerler göstermeli.

> MSIS ±1% doğruluk — I2C/SPI/UART/PWM/ADC için bol bol yeterli. HSE 24 MHz crystal RTC için kalıyor ya da gelecekte daha hassas zamanlama isterse aktive ederiz.

## 4. Project Manager

Üstteki **Project Manager** sekmesi:

### 4.1 Project sekmesi

- Project Name: `PressureTx_4_20`
- Project Location: `C:\PressureTransmitter\Firmware`
- Application Structure: `Advanced`
- Toolchain Folder Location: tick **Default**
- Toolchain / IDE: **CMake**
- Linker Settings → Minimum Heap: `0x200`, Minimum Stack: `0x800` (default genelde yeter; gerekirse arttırırsın)

### 4.2 Code Generator sekmesi

- **Copy only the necessary library files** ✓
- **Generate peripheral initialization as a pair of '.c/.h' files per peripheral** ✓
- **Keep User Code when re-generating** ✓
- **Delete previously generated files when not re-generated** ✗ (off — yoksa App/ silinir)
- HAL Settings → bırak default

### 4.3 Advanced Settings sekmesi

- Driver Selector: tüm peripheral'lar **HAL** olarak işaretli kalsın.

## 5. Generate Code

Sağ üstte **GENERATE CODE** butonuna bas. Bir hata diyalogu çıkmazsa şu mesajla biter:
```
The Code is successfully generated under: C:\PressureTransmitter\Firmware\PressureTx_4_20
```

Dosya yapısı şöyle olur:
```
Firmware/
  PressureTx_4_20/
    cmake/
    Core/
      Inc/  main.h, gpio.h, i2c.h, spi.h, adc.h, dac.h, tim.h, usart.h, iwdg.h, ...
      Src/  main.c, gpio.c, i2c.c, spi.c, adc.c, dac.c, tim.c, usart.c, iwdg.c, stm32u3xx_it.c, ...
    Drivers/
    CMakeLists.txt
    PressureTx_4_20.ioc
```

## 6. App/ klasörünü taşı

PowerShell veya File Explorer ile:
```
Firmware\App  →  Firmware\PressureTx_4_20\App
```

(Yani mevcut `Firmware\App` klasörünü `PressureTx_4_20\` içine kopyala.)

## 7. CMakeLists.txt'i güncelle

`Firmware\PressureTx_4_20\CMakeLists.txt`'i aç, dosyanın sonuna (en alta) ekle:

```cmake
# ---- Custom application sources ----
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
# IWDG enable ettiysen:
# target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE USE_IWDG=1)
```

## 8. main.c entegrasyonu (4 yer)

`Firmware\PressureTx_4_20\Core\Src\main.c` aç, dört **USER CODE** bloğunu doldur:

**a) USER CODE BEGIN Includes** (dosyanın üstü):
```c
#include "pressure_app.h"
#include "buttons.h"
#include "xtr111_loop.h"
```

**b) USER CODE BEGIN 2** (HAL_Init ve peripheral init'lerden sonra, while döngüsünden önce):
```c
pressure_app_init();
```

**c) USER CODE BEGIN WHILE** (while döngüsünün ilk satırı):
```c
pressure_app_loop();
```

**d) USER CODE BEGIN 4** (dosyanın sonuna doğru, fonksiyon tanımı için):
```c
void HAL_GPIO_EXTI_Falling_Callback(uint16_t pin)
{
    switch (pin) {
        case BTN_UP_Pin:   buttons_on_edge(BTN_ID_UP);   break;
        case BTN_DN_Pin:   buttons_on_edge(BTN_ID_DN);   break;
        case BTN_SET_Pin:  buttons_on_edge(BTN_ID_SET);  break;
        case LOOP_FLT_Pin: loop_set_safe_state();        break;
        default: break;
    }
}
```

> User Label'ları PressureTx 4-20'da yukarıdaki gibi yazdığın için CubeMX otomatik `BTN_UP_Pin`, `BTN_DN_Pin`, `BTN_SET_Pin`, `LOOP_FLT_Pin` makrolarını üretir.

## 9. Build

VS Code'da `Firmware\PressureTx_4_20\` klasörünü aç (STM32 VS Code Extension yüklüyse):

- **Ctrl+Shift+P** → "CMake: Configure" → kit seç (arm-none-eabi)
- **F7** veya `cmake --build build` ile derle

Komut satırından:
```
cd Firmware\PressureTx_4_20
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Çıktı: `build\PressureTx_4_20.elf` ve `.hex`/`.bin`.

## 10. Hata olası noktaları

| Hata | Olası neden | Çözüm |
|---|---|---|
| `STM32U385RGTx not found` | Eski CubeMX | CubeMX 6.11+ ve STM32CubeU3 paketi |
| Linker error: `_HAL_FLASH_Program_QUADWORD` undefined | Wrong HAL ver | `Drivers/STM32U3xx_HAL_Driver/Inc/stm32u3xx_hal_flash_ex.h` içinde quadword fonksiyonunun bulunması lazım — paket güncel mi kontrol et |
| `hdma_adc1` undefined | DMA eklenmedi | ADC1'in DMA Settings sekmesinden Add ile DMA ekle, regenerate |
| `BTN_UP_Pin undefined` | User Label yazılmadı | GPIO pin'inde User Label alanı doluysa pin tabanlı makrolar üretilir |
| ST7789V karanlık ekran | MADCTL yanlış | `lcd400.c` içinde `lcd_data8(0x00)` → `0xC0` veya `0xA0` dene |
| LCD ters renk (kırmızı/mavi swap) | MADCTL RGB/BGR bit | MADCTL'e `0x08` OR ekle (BGR) |
| Loop akımı hiç çıkmıyor | LOOP_EN LOW kalmış | Boot'ta `pressure_app_init` içinde sensör OK olunca `loop_enable(true)` çağırılıyor — sensör fail'se enable edilmez |

---

İlk derlemeyi başarıyla yaptıktan sonra bana haber ver. Eğer build error verirse error mesajını paylaş, tek tek düzeltelim.
