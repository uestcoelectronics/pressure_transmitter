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

## Erişilemeyen araçlar
- `pdftoppm` yok → PDF sayfalarını görüntü olarak okuyamıyorum (şema/çizim içerikleri). Metin çıkarımı çalışıyor.
- Donanım debug (GDB/OpenOCD oturumu) — kart bağlı olsa bile etkileşimli debug bu oturumlarda planlanmadı

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
