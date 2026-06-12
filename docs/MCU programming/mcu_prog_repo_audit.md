# mcu_prog — Repo Audit

**Tarih:** 2026-06-12 | **Repo:** `C:\PressureTransmitter` | **Git:** ❌ DEĞİL (git repo yok — rollback riski!)

## Repo Yapısı

```
C:\PressureTransmitter\
├── 4-20.PDF                  # Şema (4-20 konfig) — görüntü tabanlı, Claude doğrudan okuyamıyor (pdftoppm yok)
├── PIN_MAPPING.xlsx          # MCU Pin FMEDA (EN 61508/EN 50402) — pin haritası OTORİTE kaynağı
├── BOM\                      # 5 konfig BOM'u (4_20, 4_20_HART, LP_HART, LP_PA, MODBUS)
├── DATASHEETS\               # Komponent datasheetleri (PDF)
├── docs\MCU programming\     # Bu workspace
└── Firmware\                 # STM32CubeMX + CMake projesi
    ├── PressureTransmitter.ioc   # CubeMX projesi
    ├── CMakeLists.txt / CMakePresets.json (Debug/Release, Ninja)
    ├── STM32U385xx_FLASH.ld / _RAM.ld / startup_stm32u385xx.s
    ├── App\Inc + App\Src     # Uygulama katmanı (8 modül) + INTEGRATION.md
    ├── Core\                 # CubeMX üretimi (main.c, periph init)
    ├── Drivers\              # CMSIS + STM32U3xx HAL (DOKUNMA)
    └── cmake\                # toolchain + stm32cubemx alt projesi (DOKUNMA)
```

## Dil / Build Sistemi

- **MCU:** STM32U385RGT7 (Cortex-M33, LQFP64, 1 MB flash)
- **Dil:** C11, bare-metal superloop (RTOS yok)
- **Build:** CMake 3.30 + Ninja + arm-none-eabi-gcc 15.2.1 (xpack) — hepsi PATH'te mevcut
- **Build komutu:** `cd Firmware && cmake --preset Debug && cmake --build build/Debug`
- **Hedef:** `PressureTransmitter.elf`, define: `USE_IWDG=1`
- **Build baseline (2026-06-12): BAŞARISIZ** — `App/Src/cal_storage.c:112: error: 'FLASH_TYPEPROGRAM_QUADWORD' undeclared`. STM32U3 HAL yalnızca `FLASH_TYPEPROGRAM_DOUBLEWORD` (8 bayt) ve `_BURST` destekler. Tek hata; gerisi derleniyor.

## App Katmanı Modülleri (hepsi yazılmış, stub yok)

| Modül | Görev | Durum |
|---|---|---|
| `buttons.c` (110 sat.) | 3 Hall switch (UP=PA11, DN=PA12, SET=PC13), debounce 20ms, long-press 600ms, repeat 200ms, event tabanlı | Tam |
| `cal_storage.c` (124 sat.) | Kalibrasyon parametreleri → dahili flash page 127 (son 8 KB), CRC32 + magic "CAL0" | Tam ama **build hatası** (QUADWORD) |
| `fdc2214.c` (163 sat.) | FDC2214 I2C @0x2A, CH2/CH3 okuma, delta (CH3−CH2) | Tam ama **destek pinleri eksik** (aşağıda) |
| `lcd400.c` (254 sat.) | ST7789V 240×240, 5×7 font, 20 sütun × 4 satır, dirty-row, TIM1_CH1 backlight PWM | Tam; ST7789V **datasheet'le teyitli** |
| `pressure_app.c` (256 sat.) | Orkestratör: superloop 5/100/250/100 ms dilimli; ΔC→P (2 nokta + sıcaklık komp.), P→mA, IIR damping, fault → 3.6 mA | Tam |
| `state_machine.c` (180 sat.) | UI: NORMAL → MENU (10 öğe) → EDIT_FLOAT / CAL_LIVE | Tam |
| `temp_diode.c` (28 sat.) | Diyot modeli: T = 25 + (V_f − V_f25)/TC — donanımda **1N4148** (kullanıcı teyitli); model doğru, PC1 ikinci kanal eksik | Tam (genişletilecek) |
| `xtr111_loop.c` (90 sat.) | DAC1_OUT1 → XTR111 SET, INA190 geri besleme (PC5), LOOP_EN PB2, fault PA6 | Tam |

## Çevre Birimleri (Core/, CubeMX üretimi)

ADC1 (DMA circular, 3 kanal: PC0/PC4/PC5), DAC1_OUT1 (PA4), I2C1 400k (PB8/PB9), SPI2 ~5MHz (LCD), USART3 115200 (BLE — sürücü yok), TIM1_CH1 PWM (backlight), IWDG (~256 ms), GPDMA1_CH0. Saat: MSIS RC (HSE/LSE kristaller kullanılmıyor — kartta var).

## Firmware ↔ PIN_MAPPING.xlsx Uyumsuzlukları (KRİTİK BULGULAR)

PIN_MAPPING.xlsx ("MCU Pin Map" sayfası) FMEDA pin haritası, şemadan türetilmiş otorite kaynak:

1. **Sıcaklık ölçümü (KULLANICI TEYİDİ 2026-06-12):** PC0 = TMP_ADC1, PC1 = TMP_ADC2 → **1N4148 diyotlar** (sensör kompanzasyonu için; pin haritasındaki "thermistor" etiketi yanıltıcı). Firmware'in diyot modeli kavramsal olarak DOĞRU (1N914 yerine 1N4148 — aynı onsemi datasheet ailesi: `DATASHEETS\__TI_DATASHEETS\1N914-D.PDF`). **B701 TMP108** ise yalnızca ORTAM sıcaklığı izler: 60 °C üstünde ALERT → **FLT_TEMP# = PB5** kesmesi. Firmware'de eksikler: TMP108 sürücüsü (alert konfigi), PC1 ikinci diyot kanalı, FLT_TEMP# işleme.
2. **FDC2214 destek pinleri firmware'de hiç sürülmüyor/okunmuyor:**
   - **CLK_EN = PD2** → X403 (40 MHz) osilatör enable. **Sürülmezse FDC2214 saatsiz kalabilir → sensör ölü.**
   - **SD = PB6** (shutdown), **INT_B = PB7**, **ERRB = PA1**, **CD_IRQ = PA0**
3. **LED:** Pin haritası **PB10 = LED_CTRL**; `bsp_pins.h`'deki LED tanımı doğrulanmalı.
4. **LCD:** **LCD_PWR_ON = PA10** (ekran güç enable) ve **TFT_TE = PC9** — firmware'de sürülüp sürülmediği doğrulanmalı.
5. **ADC kanalları:** PC0=IN1, PC1=IN2, PC4=IN13, PC5=IN14. Firmware taraması PC1'i içermiyor.
6. **Kristal etiket çelişkisi:** Pin haritası X400 = 32.768 kHz LSE diyor; 4-20 BOM'unda X400 = 24.000 MHz. Designator karışıklığı — şemadan teyit gerekli.

## Uyumlu Olanlar (teyitli)

- Butonlar: PA11/PA12/PC13 = SW_HALL1/2/3 (AH1913) ✓
- DAC PA4 → XTR111 SET, LOOP_EN PB2, FLT PA6, INA190 PC5 ✓
- BLE pinleri: USART3 PC10/PC11, BLE_PWR_ON PC2, BLE_MODE PB12, BLE_RESET PA15 (OD), BLE_EVENT PB0 ✓ (sürücü yok)
- WDI PC3 (TPS3851H30 U701) ✓
- LCD denetleyici ST7789V — `DATASHEETS\DS154S10Z0TG01.pdf` s.4 "Drive: ST7789V, 240RGB×240, SPI-4wire" ✓
- X403 = 40 MHz (FDC2214 saat varsayımıyla uyumlu) ✓

## Genişleme Noktaları

- Yeni modüller: `App/Src/*.c` + `App/Inc/*.h` + `Firmware/CMakeLists.txt` target_sources listesi
- `main.c`: yalnızca `USER CODE BEGIN/END` blokları içinde değişiklik
- Çevre birimi konfig değişikliği gerekirse: `.ioc` → CubeMX'te regenerate (MANUEL adım) — Core/ dosyalarını elle USER CODE dışında düzenleme

## Risk Alanları

- **Git yok** → değişiklik öncesi yedek/git init şart
- IWDG 256 ms vs bloklayan işlemler (flash page erase, I2C 50 ms timeout) — cal_save sırasında watchdog reset riski
- TPS3851**H** = windowed watchdog: kick aralığı pencere dışına düşerse reset (zamanlama datasheet'ten doğrulanmadı; datasheet klasörde YOK)
- `Core/` CubeMX üretimi: USER CODE dışı el değişiklikleri regenerate'te silinir

## Do-Not-Break

- Boot'ta loop disabled (PB2=LOW) güvenli başlangıç davranışı
- Fault → 3.6 mA alarm-low davranışı
- Kalibrasyon flash formatı (magic/CRC) — format değişirse sürüm alanı artırılmalı
- Watchdog kick disiplini (100 ms)
- 3 buton UI akışı (INTEGRATION.md §7 prosedürü)

## Do-Not-Touch

- `Firmware/Drivers/**` (HAL + CMSIS)
- `Firmware/cmake/stm32cubemx/**`, `Firmware/cmake/gcc-arm-none-eabi.cmake`
- `startup_stm32u385xx.s`, `*.ld` linker scriptleri
- `Core/**` USER CODE blokları dışındaki bölgeler
- `Firmware/build/**` (üretilen)

## Test Altyapısı

Yok. INTEGRATION.md §7'de manuel ilk açılış prosedürü var (multimetre + 250 Ω yük + ST-Link).
