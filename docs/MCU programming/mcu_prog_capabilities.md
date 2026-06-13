# mcu_prog — Capabilities

## Claude'un doğrudan yapabildikleri
- C kaynak/başlık okuma-yazma (App/), CMakeLists düzenleme
- Build çalıştırma: `cmake --preset Debug && cmake --build build/Debug` (toolchain PATH'te ✓)
- xlsx okuma (openpyxl ✓ — PIN_MAPPING, BOM)
- PDF **metin** çıkarımı (pypdf ✓ — datasheetlerin metin kısmı)
- Web araştırması (WebSearch/WebFetch)
- Statik analiz, diff inceleme, map dosyası inceleme

## Kullanıcı / manuel aksiyon gerektirenler
- Karta flash + donanım testi — ST-Link, multimetre, basınç referansı
- `.ioc` değişikliği + Generate Code — STM32CubeMX
- Şema görsel inceleme (4-20.PDF) — PDF görüntü render'ı yok
- BLE uçtan uca test — telefon (nRF Connect)
- git init kararı, commit/push izinleri

## Flash & Debug (2026-06-13 tespit — bkz. mcu_prog_flash_debug.md)
- **Flash:** EVET — STM32_Programmer_CLI v2.21 kurulu; `mode=UR -d ELF --verify -rst` ile headless yükleme + doğrulama
- **Canlı kaynak debug:** EVET — ST-LINK_gdbserver 7.13 (`AppData\Local\stm32cube\bundles\...`) + arm-none-eabi-gdb kurulu; breakpoint/step/`print var`/backtrace scriptlenebilir
- **Post-mortem bellek:** EVET — Programmer CLI `-r32 ADDR` ile RAM/flash okuma (gdbserver'sız)
- **SWO/ITM telemetri:** mümkün (PB3=SWO kartta); printf→ITM eklenirse Claude canlı okur
- **Tek fiziksel gereksinim:** ST-Link board'a+USB'ye TAKILI olmalı (kullanıcı)

## Erişilemeyen araçlar
- `pdftoppm` yok → PDF sayfalarını görüntü olarak okuyamıyorum (şema/çizim içerikleri). Metin çıkarımı çalışıyor.
- Fiziksel dünya: ST-Link takma, 24V/250Ω/multimetre, basınç referansı, buton, LCD gözlemi — kullanıcı yapar

## Bilinen eksik yetenekler → roadmap etkisi
- Şema okuyamama → MANUAL-2 teyitleri kart kapılarına bağlandı
- Donanım erişimi yok → seviye 4-5 doğrulamalar hep MANUAL; kod kartları seviye 2'de teslim edilir

## Önerilen gelecek otomasyonlar
- Poppler (pdftoppm) kurulumu → şema sayfalarını görsel okuma
- Host-side unit test (CMake native target + sahte HAL) → seviye 3 doğrulama Claude'da kalır
- ST-Link CLI (STM32_Programmer_CLI) PATH'e eklenirse flash işlemi komutla yapılabilir (yine kullanıcı gözetiminde)

## Bu görev için manuel araç gereksinimleri
- Unity Editor: NO | Blender: NO | Mobil cihaz: YES (BLE testi) | Firebase/Play Console: NO
- Harici API kimlik bilgisi: NO
- Donanım: YES (kart, ST-Link, multimetre, 24 V, 250 Ω, basınç referansı)
- STM32CubeMX: YES (.ioc değişiklikleri)
