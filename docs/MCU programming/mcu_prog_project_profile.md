# PressureTransmitter — Project Profile (repo-düzeyi, ease-me görevlerinde yeniden kullanılır)

- **Proje tipi:** Gömülü firmware (endüstriyel basınç transmitteri, 5 donanım konfigürasyonu)
- **MCU:** STM32U385RGT7 (Cortex-M33, 1 MB flash, LQFP64)
- **Dil/standart:** C11, bare-metal superloop, STM32 HAL
- **Build:** `cd C:\PressureTransmitter\Firmware && cmake --preset Debug && cmake --build build/Debug` (Ninja, arm-none-eabi-gcc 15.2.1 xpack)
- **Release:** `cmake --preset Release && cmake --build build/Release`
- **Test komutu:** YOK (test altyapısı kurulmadı)
- **Çalıştırma:** Donanımda — ST-Link ile flash (manuel)

## Önemli klasörler
| Yol | Amaç |
|---|---|
| `Firmware/App/{Inc,Src}` | Uygulama katmanı — geliştirme burada |
| `Firmware/Core/` | CubeMX üretimi — yalnız USER CODE blokları |
| `Firmware/PressureTransmitter.ioc` | CubeMX projesi — elle düzenleme YOK |
| `PIN_MAPPING.xlsx` | Pin FMEDA — **pin otorite kaynağı** (5 konfig sütunu) |
| `BOM/` | Konfig başına dizgi listesi |
| `DATASHEETS/` | Komponent PDF'leri |
| `4-20.PDF` | 4-20 konfig şeması (görüntü tabanlı) |
| `docs/MCU programming/` | ease-me workspace (mcu_prog görevi) |

## Yasak klasörler
`Firmware/Drivers/**` (HAL/CMSIS), `Firmware/cmake/**`, `Firmware/build/**`, `startup_*.s`, `*.ld`, `Core/**` USER CODE dışı, `~$*.xlsx` (Office kilit dosyaları)

## Mimari ilkeler
- Modül başına tek sorumluluk; donanım erişimi `bsp_pins.h` makroları üzerinden
- Olay tabanlı UI (buttons → state_machine → render); bloklamayan zaman dilimli superloop
- Güvenli varsayılan: boot'ta loop kapalı, fault → 3.6 mA, watchdog daima beslenir
- Flash kalibrasyon kaydı: magic + version + CRC32; format değişimi = version artışı + migrasyon

## Kod stili
- snake_case fonksiyonlar, modül öneki (`fdc2214_`, `loop_`, `sm_`); `s_` statik değişken öneki
- Yorumlar Türkçe; başlıklarda donanım referansları (pin, IC designator)
- Dinamik bellek yok; statik tamponlar

## Manuel araçlar
STM32CubeMX (.ioc), ST-Link (flash), multimetre/basınç referansı (doğrulama), telefon nRF Connect (BLE)

## Bilinen tuzaklar
- STM32U3 HAL flash: QUADWORD yok → DOUBLEWORD/BURST (U5'ten kod taşırken dikkat)
- TPS3851H30 windowed watchdog — erken kick de reset üretir
- CubeMX regenerate USER CODE dışını ezer
- PIN_MAPPING "off-BOM" notları ↔ BOM gerçeği çelişebilir; BOM güncel kabul edilir
- Repo git DEĞİL (2026-06-12 itibarıyla) — CARD-0.3'te düzelmesi planlı

## Kullanıcı tercihleri
- Türkçe iletişim; planlama → onay → kart kart uygulama (ease-me); commit/push yalnız açık izinle
