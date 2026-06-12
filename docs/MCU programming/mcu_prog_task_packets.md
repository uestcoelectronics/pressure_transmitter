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
