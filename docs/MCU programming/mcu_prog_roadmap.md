# mcu_prog — Roadmap

**Proje hedefi:** STM32U385RGT7 tabanlı basınç transmitteri — 4-20 mA konfigürasyonu için üretime hazır firmware: doğru sensör okuma (FDC2214), sıcaklık kompanzasyonu, 3 butonlu LCD menü/kalibrasyon, BLE konfigürasyon, sağlam watchdog/fault davranışı.
**Görev adı:** mcu_prog | **Tarih:** 2026-06-12 | **Branch:** yok (git repo değil — Faz 0'da çözülecek)

---

## Phase P0 — Baseline & Donanım Gerçeği Mutabakatı
**Goal:** Derlenen bir firmware + pin haritasıyla bire bir uyumlu BSP tabanı.
**Scope:** cal_storage flash API düzeltmesi (DOUBLEWORD), bsp_pins.h ↔ PIN_MAPPING.xlsx mutabakatı, git init + .gitignore.
**Non-goals:** Yeni özellik, davranış değişikliği.
**Dependencies:** —
**Likely files touched:** `App/Src/cal_storage.c`, `App/Inc/bsp_pins.h`, `.gitignore` (yeni)
**Manual steps required:** YES — git init onayı; X400/X401 kristal designator teyidi (şema)
**Target validation level:** 2 (temiz build)
**Risks:** CubeMX regenerate ile USER CODE dışı kayıplar
**Done criteria:**
- [ ] `cmake --build build/Debug` hatasız ve uyarılar kayıtlı
- [ ] bsp_pins.h pin haritasıyla uyumlu (PB10 LED, PA10 LCD_PWR_ON, PD2 CLK_EN, PB6 SD, PB7 INT_B, PA1 ERRB, PA0 CD_IRQ tanımlı)
- [ ] git repo + ilk commit (kullanıcı onayıyla)
**Complexity:** Low

## Phase P1 — Sensör Yolu Tamamlama
**Goal:** FDC2214 bring-up + sıcaklık alt sisteminin donanım gerçeğine göre tamamlanması (1N4148 kompanzasyon diyotları + TMP108 ortam izleme).
**Scope:** FDC2214 güç/saat sırası (CLK_EN, SD) + hata pinleri; TMP108 ortam sıcaklığı sürücüsü + **T_HIGH=60 °C alert → FLT_TEMP# (PB5) kesmesi**; 1N4148 diyot modeli teyidi (datasheet parametreleri) + PC1 ikinci diyot kanalı; ADC taramasına PC1 ekleme (.ioc).
**Non-goals:** Kompanzasyon matematiği değişikliği (P2'de).
**Dependencies:** P0
**Likely files touched:** `fdc2214.c/h`, yeni `tmp108.c/h`, `temp_diode.c/h` (genişletme), `pressure_app.c`, `bsp_pins.h`, `.ioc` (CubeMX manuel)
**Manual steps required:** YES — CubeMX'te ADC IN2 ekleme + regenerate; diyot bias devresi teyidi; FDC ADDR pini teyidi
**Target validation level:** 2 (donanım yoksa) / 4 (kart varsa)
**Risks:** CLK_EN sürülmeden sensör test edilemez; I2C adres belirsizliği (0x2A/0x2B)
**Done criteria:**
- [ ] FDC2214 init: SD=LOW, CLK_EN=HIGH, ERRB/INT_B izleme
- [ ] TMP108 ortam sıcaklığı okunuyor; 60 °C alert FLT_TEMP# kesmesi işleniyor ve alarm davranışı tanımlı
- [ ] 1N4148 çift kanal (PC0/PC1) okuma + çapraz makullük
- [ ] Donanımda DEVICE_ID (0x3055) okundu (kart varsa)
**Complexity:** Medium

## Phase P2 — Ölçüm Kalitesi: Kalibrasyon & Sıcaklık Kompanzasyonu
**Goal:** Gerçek sıcaklık kaynağıyla doğru, tekrarlanabilir basınç ölçümü.
**Scope:** Kompanzasyon modeli (zero/span ayrı katsayı, t_ref), kalibrasyon iş akışı sağlamlaştırma (yakalama doğrulama, mantıksız değer reddi), damping.
**Non-goals:** Çok noktalı (>2) kalibrasyon, polinom fit (ileride).
**Dependencies:** P1
**Likely files touched:** `pressure_app.c`, `cal_storage.c/h` (sürüm artışı), `state_machine.c`
**Manual steps required:** YES — basınç referansıyla kalibrasyon testi
**Target validation level:** 4 (donanım + referans basınç)
**Risks:** cal_params_t yapısı değişirse flash format sürümü artmalı (eski kayıt geçersiz)
**Done criteria:**
- [ ] Sıcaklık kompanzasyonu 1N4148 diyot kanal(lar)ından besleniyor
- [ ] Kalibrasyon yakalamada stabilite kontrolü (ör. ΔC varyansı eşiği)
- [ ] Referans basınçlarla 4/20 mA doğrulama
**Complexity:** Medium

## Phase P3 — Ekran & Menü State Machine İyileştirme
**Goal:** Sahada kullanılabilir, hataya dayanıklı UI.
**Scope:** LCD_PWR_ON sıralaması, donanımda görsel doğrulama; menü timeout→NORMAL, birimler, hata/alarm ekranları, backlight ayar menüsü.
**Non-goals:** Grafik/ikon kütüphanesi, çoklu dil.
**Dependencies:** P0 (build), donanım testleri P1 sonrası
**Likely files touched:** `lcd400.c/h`, `state_machine.c/h`, `pressure_app.c`
**Manual steps required:** YES — donanımda görsel test
**Target validation level:** 4
**Risks:** ST7789V init donanımda farklı davranabilir (gamma/yön ayarları)
**Done criteria:**
- [ ] Ekran açılışta güvenilir başlıyor (güç sırası + reset zamanlaması)
- [ ] 60 s menü timeout; fault durumunda alarm ekranı
- [ ] 3 butonla tam kalibrasyon akışı donanımda yapıldı
**Complexity:** Medium

## Phase P4 — Çıkış Döngüsü Bütünlüğü (4-20 mA)
**Goal:** Komut ↔ ölçülen akım tutarlılığı ve NAMUR uyumlu alarm davranışı.
**Scope:** INA190 geri besleme makullük kontrolü (komut vs ölçüm sapma alarmı), NAMUR NE43 seviyeleri, fault kurtarma (latch/auto-retry politikası).
**Non-goals:** HART (ayrı konfig).
**Dependencies:** P1 (ADC düzeni)
**Likely files touched:** `xtr111_loop.c/h`, `pressure_app.c`
**Manual steps required:** YES — multimetre ile akım doğrulama (250 Ω + 24 V)
**Target validation level:** 4
**Risks:** XTR111 transfer fonksiyonu (R_SET) şemadan teyit edilmeli
**Done criteria:**
- [ ] |I_komut − I_ölçülen| eşiği aşınca tanı bayrağı
- [ ] 4/12/20 mA noktaları multimetreyle ±0.05 mA içinde
**Complexity:** Medium

## Phase P5 — Bluetooth Konfigürasyon (DL-CC2340-B)
**Goal:** Telefon/araçtan BLE ile canlı ölçüm okuma + kalibrasyon parametresi yazma.
**Scope:** USART3 IT/DMA ring-buffer sürücüsü, BLE güç/reset/mod pin yönetimi, AT init dizisi, CRC'li çerçeve protokolü (ölçüm oku, parametre oku/yaz, kaydet, cihaz bilgisi), BLE_EVENT işleme.
**Non-goals:** OTA firmware güncelleme, bonding/şifreleme politikası (v2).
**Dependencies:** P0; tam test için P1+P2
**Likely files touched:** yeni `ble_uart.c/h`, yeni `ble_proto.c/h`, `pressure_app.c`, `main.c` (USER CODE)
**Manual steps required:** YES — modül datasheet PDF indirme; telefonla bağlantı testi (nRF Connect vb.)
**Target validation level:** 5 (telefon ↔ cihaz)
**Risks:** Modül varsayılan baud/AT formatı belirsiz; BLE_MODE pin işlevi teyitsiz
**Done criteria:**
- [ ] AT init + advertise başlatma; bağlantı/kopma BLE_EVENT ile izleniyor
- [ ] Telefondan canlı basınç/sıcaklık okunuyor; cal parametresi yazılıp flash'a kaydediliyor
- [ ] UART hata sayaçları (frame/CRC) tutuluyor
**Complexity:** High

## Phase P6 — Watchdog & Tanılar
**Goal:** TPS3851 pencere uyumu + temel FMEDA tanıları.
**Scope:** TPS3851H30 zamanlama teyidi (web) ve kick stratejisi; flash erase sırasında watchdog yönetimi; ADC kanal makullük (range) kontrolleri; GPIO read-back (LOOP_EN, BLE_PWR_ON, LCD_PWR_ON); I2C hata sayaçları + bus kurtarma.
**Non-goals:** Tam EN 61508 tanı seti (RAM March testi, saat çapraz izleme) — ayrı görev.
**Dependencies:** P1, P4
**Likely files touched:** `pressure_app.c`, yeni `diag.c/h`, `cal_storage.c`
**Manual steps required:** YES — watchdog reset testi (kick durdurarak)
**Target validation level:** 4
**Done criteria:**
- [ ] Kick zamanlaması TPS3851H30 penceresine kanıtlanabilir uyumlu
- [ ] Kick kesilince donanım reset gözlendi
- [ ] ADC range-check fault'ları alarm akımına yansıyor
**Complexity:** Medium

## Phase P7 — Donanım Doğrulama & Sürüm
**Goal:** INTEGRATION.md prosedürünün tamamı + uzun süreli test geçilmiş aday sürüm.
**Scope:** Tam bring-up checklist, referans basınçla kalibrasyon, 24 saat soak, sürüm etiketi.
**Dependencies:** P1–P6
**Manual steps required:** YES — tamamı donanım üzerinde
**Target validation level:** 5–6
**Done criteria:**
- [ ] INTEGRATION.md §7 tüm adımlar geçti
- [ ] 24 h soak: reset yok, sürüklenme kabul sınırında
- [ ] Release build + sürüm notu + git tag
**Complexity:** Medium

---

## Bağımlılık Grafiği

```
P0 ──► P1 ──► P2 ──► P7
 │      ├──► P4 ──► P6 ──► P7
 │      └──► P3 ──► P7
 └──► P5 (tam testi P1+P2 sonrası) ──► P7
```

## Kapsam Dışı (Excluded)

| Öğe | Neden |
|---|---|
| 4-20_HART / LP_HART / LP_PA / MODBUS konfigleri | İlk hedef 4-20; pin haritası çok-konfig desteğini koruyor |
| DAC8742 / DAC161S997 sürücüleri | HART/LP konfiglerine ait |
| EA_DOGS104N LCD sürücüsü | LP konfiglerine ait |
| Tam EN 61508 tanı seti (RAM testi, saat çapraz izleme, RDP kilitleme) | Ayrı sertifikasyon görevi; PIN_MAPPING "Standards Ref" referans |
| OTA / BLE bonding | BLE v2 |
