# mcu_prog — Task Packets

> Her Execute Card oturumunda bir paket eklenir. Eski paketlerin üzerine yazılmaz.
> Henüz paket yok — ilk paket CARD-0.1 onaylandığında oluşturulacak.

## TASK PACKET CARD-0.1 — 2026-06-12

**Goal:** cal_storage.c flash yazımını STM32U3 HAL'in desteklediği DOUBLEWORD (8 bayt) tipine geçirip build'i düzeltmek.
**Non-goals:** Kalibrasyon format/davranış değişikliği; başka dosya.
**Current state:** Build FAIL — `cal_storage.c:112 FLASH_TYPEPROGRAM_QUADWORD undeclared` (tek hata).
**Exact files inspected before coding:** `App/Src/cal_storage.c` (95-123), `Drivers/.../stm32u3xx_hal_flash.h` (260-271: yalnız DOUBLEWORD/BURST).
**Files allowed to edit:** `App/Src/cal_storage.c`
**Files forbidden to edit:** Drivers/**, Core/**, cmake/**
**Expected behavior after:** Temiz build; cal_save 8-bayt adımlarla, 8-bayt hizalı tamponla yazar; CRC/magic değişmez.
**Validation commands:** `cmake --build build/Debug`
**Manual validation scenario:** (kart gelince) kalibrasyon kaydet → güç çevir → değerler kalıcı.
**Rollback plan:** `App/Src/cal_storage.c.bak` kopyasından geri yükle (git yok).
**Diff budget:** 1 değişen, 0 yeni.
**Done criteria:** [ ] 0 error build; [ ] uyarılar changelog'da.
**Stop conditions:** Başka dosyada hata çıkarsa dur; flash API farklı imza isterse dur.

## TASK PACKET CARD-0.2 (+0.3) — 2026-06-12

**Goal:** bsp_pins.h'ı PIN_MAPPING ile bire bir hizalamak; git init.
**Exact files inspected:** `App/Inc/bsp_pins.h` (tam), `Core/Src/gpio.c` (tam), `Core/Src/adc.c` (60-94).
**Files edited:** `App/Inc/bsp_pins.h`; yeni `.gitignore`.
**Bulgular:** (1) .ioc tüm FMEDA pinlerini zaten tanımlıyor; CLK_EN boot'ta HIGH. (2) ADC BUG: rank 2/3'e kanal atanmamış — hep CHANNEL_1; .ioc düzeltmesi MANUAL-3'e işlendi. (3) CD_IRQ (PA0) kullanılamaz — EXTI0 hattı BLE_EVENT'te.
**Validation:** build PASS (0 err / 0 warn). Seviye 2.
**Rollback:** git checkout (main, e2120fe sonrası commit).

## TASK PACKET CARD-1.1 — 2026-06-12

**Goal:** FDC2214 uygulama tarafı bring-up: güç/saat sıralaması app kontrolünde, ERRB/INT_B kullanımı, I2C adres (0x2A/0x2B) otomatik tespiti.
**Non-goals:** Ölçüm matematiği, MUX/kanal konfig değişikliği, main.c düzenleme (ERRB polling ile — EXTI'ye gerek yok).
**Current state:** gpio.c CLK_EN=HIGH/SD=LOW'u boot'ta kuruyor; fdc2214_init bunları tekrarlıyor ama sıralama/zamanlama yok; ERRB/INT_B pinleri hiç okunmuyor; adres sabit 0x2A.
**Exact files inspected:** fdc2214.c (tam), fdc2214.h (tam), pressure_app.c (tam), main.c USER CODE 4, gpio.c (önceki kart).
**Files allowed to edit:** App/Src/fdc2214.c, App/Inc/fdc2214.h, App/Src/pressure_app.c
**Files forbidden:** Core/**, Drivers/**, .ioc
**Expected behavior after:**
- Açılışta belirli sıra: CLK_EN→osc otur→SD release→bekle→DEV_ID; 0x2A başarısızsa 0x2B dene
- Init başarısızsa bir kez güç çevrimiyle retry
- 100 ms döngüde ERRB yoklanır; hata → STATUS oku (temizler), alarm-low + ekranda "SENSOR ERR"; ERRB düzelir + okuma başarılıysa otomatik temizlenir
- INT_B data-ready: hazır değilse o tikte okuma atlanır (son değer korunur)
**Validation commands:** cmake --build build/Debug
**Manual validation scenario:** Donanımda: boot logunda sensor OK; ERRB'yi tetikleyecek koşulda (sensör kablosu çıkar) alarm-low + SENSOR ERR; geri takınca otomatik düzelme.
**Rollback plan:** git checkout -- Firmware/App/Src/fdc2214.c Firmware/App/Inc/fdc2214.h Firmware/App/Src/pressure_app.c
**Diff budget:** 3 değişen, 0 yeni.
**Done criteria:** temiz build; sıralama+adres tespiti+ERRB/INT_B kodda; donanım kısmı MANUAL-4.
**Stop conditions:** main.c değişikliği gerekirse dur (polling yaklaşımı bunu önlüyor).
