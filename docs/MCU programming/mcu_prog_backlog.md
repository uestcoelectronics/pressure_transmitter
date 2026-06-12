# mcu_prog — Backlog

> **DONDURULMUŞ / YASAK DOSYALAR:** `Firmware/Drivers/**`, `Firmware/cmake/**`, `startup_stm32u385xx.s`, `*.ld`, `Core/**` USER CODE blokları dışı, `Firmware/build/**`. `.ioc` değişikliği = CubeMX manuel adımı (elle düzenleme yok).

---

## CARD-0.1 — Build Baseline Düzeltmesi (flash DOUBLEWORD)

**Purpose:** Firmware şu an derlenmiyor; her şeyin ön koşulu.
**Scope:** `cal_storage.c` flash yazımını STM32U3 HAL'in desteklediği `FLASH_TYPEPROGRAM_DOUBLEWORD` (8 bayt) ile yeniden yaz (veya `_BURST` değerlendir). Buffer hizalamasını 8 bayta uyarla. Build uyarılarını listele.
**Non-goals:** Kalibrasyon formatı değişikliği.
**Dependencies:** —
**Inspect before coding:** `App/Src/cal_storage.c`, `Drivers/STM32U3xx_HAL_Driver/Inc/stm32u3xx_hal_flash.h` (satır 260-271: DOUBLEWORD/BURST tanımları), `App/Inc/cal_storage.h`
**Files allowed to edit:** `App/Src/cal_storage.c`
**Files forbidden to edit:** Drivers/**, Core/**
**Expected behavior:** `cmake --build build/Debug` sıfır hata; cal_save flash'a 8-bayt adımlarla yazar, CRC doğrulaması değişmez.
**Tests / validation:** `cd Firmware && cmake --build build/Debug` → elf üretimi; map dosyasında cal bölgesi kontrolü.
**Manual validation:** (kart varsa) kalibrasyon kaydet → güç çevir → değerler korunmuş.
**Rollback plan:** git yoksa: düzenleme öncesi `cal_storage.c.bak` kopyası; git varsa `git checkout -- App/Src/cal_storage.c`.
**Diff budget:** 1 production dosya, 0 yeni.
**Done criteria:**
- [ ] Temiz build (0 error)
- [ ] Uyarı listesi changelog'a işlendi

## CARD-0.2 — bsp_pins.h ↔ PIN_MAPPING Mutabakatı

**Purpose:** Firmware'in pin gerçeğiyle eşleşmesi; eksik pinlerin tanımlanması.
**Scope:** `bsp_pins.h`'a ekle/teyit et: LED_CTRL=PB10, LCD_PWR_ON=PA10, TFT_TE=PC9, CLK_EN=PD2, FDC_SD=PB6, FDC_INT_B=PB7, FDC_ERRB=PA1, FDC_CD_IRQ=PA0, TMP108_ALERT(FLT_TEMP#)=PB5. `.ioc`'taki GPIO tanımlarıyla çapraz kontrol; eksikler MANUAL adım olarak CubeMX listesine.
**Non-goals:** Pinleri sürmek (CARD-1.x).
**Dependencies:** CARD-0.1
**Inspect before coding:** `App/Inc/bsp_pins.h`, `Core/Src/gpio.c`, `Firmware/PressureTransmitter.ioc`, `PIN_MAPPING.xlsx` (MCU Pin Map)
**Files allowed to edit:** `App/Inc/bsp_pins.h`
**Files forbidden to edit:** `gpio.c` (CubeMX üretimi — değişiklik .ioc üzerinden)
**Expected behavior:** Derleme bozulmaz; tüm pin makroları FMEDA haritasıyla bire bir.
**Tests / validation:** Build + makro/pin tablosunun PIN_MAPPING ile diff'i (doc olarak changelog'a).
**Rollback plan:** Yedek kopya / git checkout.
**Diff budget:** 1 dosya, 0 yeni.
**Done criteria:**
- [ ] 9 pin tanımlı ve doğru
- [ ] .ioc'ta eksik GPIO'lar MANUAL adım olarak kayıtlı

## CARD-0.3 — Git İnit + .gitignore (KULLANICI ONAYI GEREKLİ)

**Purpose:** Rollback güvenliği — şu an hiç versiyon kontrolü yok.
**Scope:** `git init`, `.gitignore` (build/, ~$*, *.bak), ilk commit.
**Dependencies:** — (her karttan önce yapılması ideal)
**Inspect before coding:** repo kökü
**Files allowed to edit:** `.gitignore` (yeni)
**Expected behavior:** Tüm kaynak ilk commit'te; build çıktıları hariç.
**Tests / validation:** `git status` temiz.
**Rollback plan:** `.git` klasörünü sil (geri alınabilir).
**Diff budget:** 0 production, 1 yeni destek dosyası.
**Done criteria:**
- [ ] Kullanıcı onayladı
- [ ] İlk commit atıldı

## CARD-1.1 — FDC2214 Güç/Saat/Hata Pinleri Bring-up

**Purpose:** CLK_EN sürülmeden FDC2214 saatsiz → sensör çalışmaz.
**Scope:** Init sırası: FDC_SD=LOW (uyandır) → CLK_EN=HIGH (X403 40 MHz) → bekleme → fdc2214_init(). ERRB (PA1) polling/EXTI, INT_B (PB7) veri-hazır kullanımı (opsiyonel polling kalabilir). I2C başarısızlığında 0x2B adres denemesi (ADDR belirsizliği için geçici tanı logu).
**Non-goals:** Ölçüm matematiği.
**Dependencies:** CARD-0.2
**Inspect before coding:** `App/Src/fdc2214.c`, `App/Inc/fdc2214.h`, `App/Src/pressure_app.c` (init sırası), `Core/Src/gpio.c` (PD2/PB6 çıkış konfigi var mı)
**Files allowed to edit:** `fdc2214.c/h`, `pressure_app.c`, `bsp_pins.h`
**Files forbidden to edit:** Core/** (GPIO eksikse .ioc → MANUAL)
**Expected behavior:** Açılışta sensör saatlenir, DEVICE_ID 0x3055 okunur; ERRB aktifse hata bayrağı + alarm akımı.
**Tests / validation:** Build; donanımda DEVICE_ID logu (kart varsa).
**Manual validation:** I2C hattında haberleşme (mantık analizörü/scope opsiyonel).
**Rollback plan:** Yedek/git checkout.
**Diff budget:** 3 dosya, 0 yeni.
**Done criteria:**
- [ ] SD/CLK_EN sırası kodda
- [ ] ERRB işleniyor
- [ ] Donanımda ID okundu (yoksa MANUAL bekliyor)

## CARD-1.2 — TMP108 Ortam İzleme + 60 °C Alert (FLT_TEMP#)

**Purpose:** TMP108 = yalnız ORTAM sıcaklığı monitörü (kullanıcı teyitli rol); 60 °C üstünde donanım kesmesiyle alarm.
**Scope:** Yeni `tmp108.c/h`: init (sürekli dönüşüm, 12-bit 0.0625 °C/LSB), ortam sıcaklığı okuma; **T_HIGH = 60 °C alert eşiği konfigürasyonu** (histerezisle, ör. T_LOW=55 °C geri dönüş), ALERT pin polaritesi aktif-LOW → **FLT_TEMP# (PB5) EXTI falling kesmesi**; kesmede ortam-aşırı-sıcaklık tanı bayrağı + ekran/alarm davranışı (politika: ölçüm devam eder, uyarı gösterilir — kullanıcıyla netleştirilebilir). I2C adresi şemadan teyit (ADD0 — MANUAL-2).
**Non-goals:** Basınç kompanzasyonuna katkı — TMP108 kompanzasyonda KULLANILMAZ (kompanzasyon = 1N4148 diyotlar, CARD-1.3).
**Dependencies:** CARD-0.2
**Inspect before coding:** `DATASHEETS\TMP108AIYFFT.pdf` (register map: config THIGH/TLOW, alert modu), `App/Src/fdc2214.c` (I2C helper deseni), `Core/Src/i2c.c`, `Core/Src/gpio.c` (PB5 EXTI var mı — yoksa .ioc MANUAL)
**Files allowed to edit:** yeni `App/Src/tmp108.c`, yeni `App/Inc/tmp108.h`, `Firmware/CMakeLists.txt` (target_sources), `pressure_app.c`, `main.c` (USER CODE EXTI callback)
**Expected behavior:** 1 Hz ortam sıcaklığı okuma; 60 °C aşımında FLT_TEMP# kesmesi → alarm bayrağı; histerezis altına inince bayrak temizlenir; I2C hatasında sayaç + son geçerli değer.
**Tests / validation:** Build; donanımda oda sıcaklığı makullüğü; alert testi (eşiği geçici 30 °C yapıp ısıtarak — MANUAL).
**Rollback plan:** Yeni dosyaları sil + CMakeLists geri al.
**Diff budget:** 3 değişen, 2 yeni.
**Done criteria:**
- [ ] TMP108 ortam okuma çalışıyor (veya MANUAL bekliyor)
- [ ] T_HIGH=60 °C alert konfigi yazılıyor ve doğrulanıyor (register read-back)
- [ ] FLT_TEMP# kesmesi alarm bayrağına bağlı

## CARD-1.3 — 1N4148 Diyot Modeli Teyidi + İkinci Kanal (PC0/PC1) + ADC Düzeni

**Purpose:** Kompanzasyon sıcaklığı = sensör içi **1N4148 diyotlar** (kullanıcı teyitli). Mevcut diyot modeli korunur; parametreler datasheet'e göre teyit edilir, PC1 ikinci kanal eklenir.
**Scope:** `temp_diode.c/h` genişletme: çift kanal (PC0=TMP_ADC1, PC1=TMP_ADC2), 1N4148 parametre teyidi (`DATASHEETS\__TI_DATASHEETS\1N914-D.PDF` — 1N4x48'i kapsar; V_f25 varsayılanı bias akımına göre, TC ≈ −2 mV/°C), çift kanal çapraz makullük |T1−T2| eşiği → tanı bayrağı, kanal ortalaması/seçimi. ADC taramasına PC1 (IN2) ekleme: **.ioc değişikliği → CubeMX regenerate (MANUAL-3)**. ADC rank sabitleri güncelle.
**Non-goals:** TMP108 entegrasyonu (CARD-1.2); kompanzasyon matematiği (CARD-2.1).
**Dependencies:** CARD-0.2; MANUAL-2 (diyot bias direnci/akımı → V_f25 varsayılanı), MANUAL-3 (.ioc)
**Inspect before coding:** `App/Src/temp_diode.c`, `App/Inc/bsp_pins.h` (ADC rank), `Core/Src/adc.c` (kanal listesi), `App/Src/pressure_app.c` (ADC callback)
**Files allowed to edit:** `temp_diode.c/h`, `bsp_pins.h`, `pressure_app.c`, `state_machine.c` (Vf25/TC menü öğeleri kalır — 1N4148 için kalibre edilebilir)
**Files forbidden to edit:** `adc.c` (CubeMX — .ioc üzerinden MANUAL)
**Expected behavior:** İki diyot kanalından sıcaklık; tutarsızlıkta tanı bayrağı; menüden Vf25/TC ayarlanabilir (mevcut davranış korunur).
**Tests / validation:** Build; donanımda oda sıcaklığı makullüğü (her iki kanal).
**Rollback plan:** Yedek/git.
**Diff budget:** 4 değişen, 0 yeni — bütçe aşımı (+1) onaylı varsayılan.
**Done criteria:**
- [ ] Çift kanal okuma + çapraz kontrol kodda
- [ ] 1N4148 parametreleri datasheet referansıyla yorumda belgeli
- [ ] CubeMX ADC değişikliği MANUAL olarak yapıldı/işaretlendi

## CARD-1.4 — Sıcaklık Rollerinin Entegrasyonu (Diyot=Kompanzasyon, TMP108=Ortam)

**Purpose:** İki sıcaklık alt sisteminin rollerini net API'lerle ayırmak (kullanıcı teyitli mimari).
**Scope:** `pressure_app.c`'de iki ayrı kaynak: `temp_diode_get_celsius()` → **yalnız kompanzasyon**; `tmp108_get_ambient_c()` → **yalnız ortam izleme/ekran/alarm**. Diyot kanal arızasında (range dışı / kanallar tutarsız) kompanzasyon politikası: son geçerli sıcaklıkla devam + tanı bayrağı (TMP108'e failover YOK — farklı fiziksel konum, yanlış kompanzasyon üretir). Ekranda ortam sıcaklığı sayfası (opsiyonel CARD-3.2 ile).
**Dependencies:** CARD-1.2, CARD-1.3
**Inspect before coding:** `pressure_app.c` (dC_to_pressure, temp kullanımı)
**Files allowed to edit:** `pressure_app.c`, `tmp108.c/h`, `temp_diode.c/h`
**Expected behavior:** Kompanzasyon yalnız diyottan; ortam alarmı yalnız TMP108'den; roller karışmaz.
**Tests / validation:** Build; donanımda diyot kanalını koparınca son-geçerli-değer + tanı bayrağı.
**Rollback plan:** git checkout.
**Diff budget:** 3 dosya, 0 yeni.
**Done criteria:**
- [ ] İki ayrı sıcaklık API'si, roller belgeli
- [ ] Diyot arıza politikası çalışıyor

## CARD-2.1 — Sıcaklık Kompanzasyon Modeli v2

**Purpose:** Tek k_T katsayısı yetersiz; zero ve span ayrı sıcaklık etkisi görür.
**Scope:** `cal_params_t`'ye `k_t_zero`, `k_t_span` (mevcut k_t → k_t_zero geriye uyum), **`vf25_mv` + `tc_mv_c` alanları (CARD-1.3 notu: şu an menüden ayarlanan Vf25/TC reboot'ta kayboluyor — v2'de flash'a girecek, boot'ta temp_diode_set_calibration ile yüklenecek)**, flash format **version=2** + v1'den migrasyon; menüye yeni öğeler.
**Dependencies:** CARD-1.4
**Inspect before coding:** `cal_storage.c/h`, `pressure_app.c` (dC_to_pressure), `state_machine.c`
**Files allowed to edit:** `cal_storage.c/h`, `pressure_app.c`, `state_machine.c`
**Expected behavior:** P = f(ΔC, T) zero/span ayrı düzeltmeli; eski kayıt sorunsuz migrate.
**Tests / validation:** Build; iki sıcaklıkta kalibrasyon doğrulaması (MANUAL).
**Rollback plan:** git checkout; flash format v1 okuma korunur.
**Diff budget:** 4 dosya (bütçe +1 onaylı varsayılan), 0 yeni.
**Done criteria:**
- [ ] v2 format + migrasyon
- [ ] İki sıcaklık noktasında sapma kabul sınırında (MANUAL)

## CARD-2.2 — Kalibrasyon İş Akışı Sağlamlaştırma

**Purpose:** Hatalı yakalamaların (gürültülü ΔC, ters span) önlenmesi.
**Scope:** CAL_LIVE'da stabilite göstergesi (son N örnek varyansı), stabil değilken yakalama reddi; zero≈span reddi; kaydetmeden çıkışta uyarı ekranı.
**Dependencies:** CARD-2.1
**Inspect before coding:** `state_machine.c`, `pressure_app.c` (sm_update_live)
**Files allowed to edit:** `state_machine.c/h`, `pressure_app.c`
**Expected behavior:** Kararsız sinyalde "WAIT" gösterir; geçersiz kalibrasyon kaydedilemez.
**Tests / validation:** Build; donanımda senaryo testi.
**Rollback plan:** git checkout.
**Diff budget:** 3 dosya, 0 yeni.
**Done criteria:**
- [ ] Stabilite eşiği çalışıyor
- [ ] Geçersiz kombinasyonlar reddediliyor

## CARD-3.1 — LCD Güç Sıralaması + Donanım Doğrulama

**Purpose:** LCD_PWR_ON (PA10) firmware'de sürülmüyor olabilir; ekran açılış güvenilirliği.
**Scope:** lcd_init öncesi LCD_PWR_ON=HIGH + güç oturma beklemesi + reset zamanlaması (datasheet'e göre); donanımda görsel test; gerekirse ST7789V init ince ayar (MADCTL/gamma).
**Dependencies:** CARD-0.2
**Inspect before coding:** `lcd400.c` (init dizisi), `DATASHEETS\DS154S10Z0TG01.pdf` (s.6-12: timing), `gpio.c` (PA10 konfigi)
**Files allowed to edit:** `lcd400.c/h`, `bsp_pins.h`
**Expected behavior:** Her soğuk açılışta ekran ilk denemede çalışır.
**Tests / validation:** Build; 10× güç çevrim testi (MANUAL).
**Rollback plan:** git checkout.
**Diff budget:** 2 dosya, 0 yeni.
**Done criteria:**
- [ ] Güç sırası kodda
- [ ] 10/10 açılış başarılı (MANUAL)

## CARD-3.2 — Menü State Machine İyileştirmeleri

**Purpose:** "Menu state machine iyi ayarlanmalı" gereksinimi.
**Scope:** 60 s eylemsizlikte menüden NORMAL'e dönüş (kaydetmeden, uyarıyla); NORMAL ekranda birimler (bar / °C / mA) ve fault göstergesi; alarm ekranı (fault nedeni); backlight % menü öğesi (cal_params'a eklenebilir → format v2 ile birlikte); SET kısa NORMAL'de ekran sayfası değiştirme (P / T / mA / ΔC sayfaları).
**Dependencies:** CARD-2.1 (format v2 senkronu)
**Inspect before coding:** `state_machine.c/h`, `pressure_app.c` (render fonksiyonları)
**Files allowed to edit:** `state_machine.c/h`, `pressure_app.c`, `cal_storage.h` (backlight alanı v2 içinde)
**Expected behavior:** Timeout, sayfalar, alarm ekranı, backlight ayarı çalışır.
**Tests / validation:** Build; donanımda UI senaryoları.
**Rollback plan:** git checkout.
**Diff budget:** 3 dosya, 0 yeni.
**Done criteria:**
- [ ] Timeout + sayfalar + alarm ekranı
- [ ] Donanımda 3 butonla tam akış (MANUAL)

## CARD-4.1 — Loop Geri Besleme Makullüğü + NAMUR Alarmları

**Purpose:** Komut edilen ile ölçülen akım arasındaki sapmanın tanısı; standart alarm seviyeleri.
**Scope:** |I_cmd − I_meas| > eşik (ör. 0.3 mA, 2 s) → tanı bayrağı + LED hızlı yanıp sönme; NAMUR NE43 sabitleri (3.6 düşük alarm / 3.8 alt satürasyon / 20.5 üst satürasyon); fault politikası: latch + SET uzun ile clear. XTR111 R_SET değeri şemadan teyit (MANUAL).
**Dependencies:** CARD-1.1 (ADC düzeni stabil)
**Inspect before coding:** `xtr111_loop.c/h`, `pressure_app.c`
**Files allowed to edit:** `xtr111_loop.c/h`, `pressure_app.c`
**Expected behavior:** Açık devre/sapma tespit edilir; alarm akımları standart.
**Tests / validation:** Build; multimetre ile 4/12/20 mA (MANUAL).
**Rollback plan:** git checkout.
**Diff budget:** 3 dosya, 0 yeni.
**Done criteria:**
- [ ] Sapma tanısı çalışıyor
- [ ] 3 noktada ±0.05 mA (MANUAL)

## CARD-5.1 — BLE UART Taşıma Katmanı

**Purpose:** DL-CC2340-B ile güvenilir seri haberleşme altyapısı.
**Scope:** Yeni `ble_uart.c/h`: USART3 IT tabanlı RX ring buffer + TX kuyruk; BLE güç sırası (BLE_PWR_ON=PC2, BLE_RESET=PA15 OD, BLE_MODE=PB12); AT init dizisi (cihaz adı "PT910-xxxx", advertise başlat); BLE_EVENT (PB0) bağlantı durumu. Önce modül datasheet'i indirilecek (MANUAL-5): pin işlevleri + varsayılan baud teyidi.
**Non-goals:** Uygulama protokolü (CARD-5.2).
**Dependencies:** CARD-0.2; MANUAL-5
**Inspect before coding:** `Core/Src/usart.c`, `Core/Inc/usart.h`, `stm32u3xx_it.c` (USART3 IRQ var mı — yoksa .ioc MANUAL), DL-CC2340-B datasheet
**Files allowed to edit:** yeni `ble_uart.c/h`, `CMakeLists.txt`, `pressure_app.c`, `main.c` (USER CODE)
**Expected behavior:** Modül açılır, AT yanıt verir, advertise başlar; telefonda cihaz görünür.
**Tests / validation:** Build; nRF Connect ile cihaz görünürlüğü (MANUAL).
**Rollback plan:** Yeni dosyaları sil.
**Diff budget:** 3 değişen, 2 yeni.
**Done criteria:**
- [ ] AT haberleşmesi çalışıyor
- [ ] Telefonda advertise görünüyor (MANUAL)

## CARD-5.2 — BLE Konfigürasyon Protokolü

**Purpose:** BLE üzerinden ölçüm okuma + kalibrasyon yazma (gereksinim).
**Scope:** Yeni `ble_proto.c/h`: çerçeve [SOF|len|cmd|payload|CRC16]; komutlar: GET_MEAS (P, T, mA, durum), GET_PARAM/SET_PARAM (cal alanları), SAVE (flash), INFO (sürüm/seri), hata yanıtları. Yazma koruması: SET_PARAM yalnızca menüden "BLE unlock" veya sabit PIN ile (basit v1 güvenliği).
**Dependencies:** CARD-5.1
**Inspect before coding:** `ble_uart.h`, `cal_storage.h`, `pressure_app.c`
**Files allowed to edit:** yeni `ble_proto.c/h`, `pressure_app.c`, `CMakeLists.txt`
**Expected behavior:** Telefon uygulaması/test aracıyla parametre okunur-yazılır, SAVE flash'a işler.
**Tests / validation:** Build; nRF Connect ile hex çerçeve testi (MANUAL).
**Rollback plan:** Yeni dosyaları sil.
**Diff budget:** 2 değişen, 2 yeni.
**Done criteria:**
- [ ] GET_MEAS/SET_PARAM/SAVE uçtan uca (MANUAL)
- [ ] CRC hatalı çerçeve reddi

## CARD-6.1 — TPS3851 Pencere Zamanlaması Teyidi + Watchdog Stratejisi

**Purpose:** Windowed watchdog'a erken kick = reset; zamanlama kanıtlanmalı.
**Scope:** Web'den TPS3851H30EDRBT datasheet (window oranları, WD timeout sürümü); 100 ms kick'in pencereye uyumunun analizi; flash erase (cal_save) öncesi/sonrası kick stratejisi; kick'i koşullu hale getirme (ana döngü sağlık kontrolü geçerse — A.16 program akışı izleme temeli).
**Dependencies:** CARD-0.1
**Inspect before coding:** `pressure_app.c` (watchdog bölümü), `cal_storage.c` (erase süresi), `Core/Src/iwdg.c`
**Files allowed to edit:** `pressure_app.c`, `cal_storage.c`
**Expected behavior:** Kick zamanlaması belgelenmiş pencere içinde; cal_save watchdog reset'i tetiklemez.
**Tests / validation:** Build; kick durdurma testi → reset gözlemi (MANUAL).
**Rollback plan:** git checkout.
**Diff budget:** 2 dosya, 0 yeni.
**Done criteria:**
- [ ] Zamanlama analizi changelog'da
- [ ] Reset testi geçti (MANUAL)

## CARD-6.2 — Temel Tanılar (diag modülü)

**Purpose:** FMEDA'da referans verilen temel tanıların (A.13, A.7) basit uygulaması.
**Scope:** Yeni `diag.c/h`: ADC kanal range-check (TMP, VCC_FB, I_FB), GPIO read-back (LOOP_EN, LCD_PWR_ON, BLE_PWR_ON, CLK_EN), I2C hata sayaçları + bus recovery (9 clock), tanı durumu → alarm akımı/ekran.
**Dependencies:** CARD-1.4, CARD-4.1
**Inspect before coding:** `pressure_app.c`, `bsp_pins.h`
**Files allowed to edit:** yeni `diag.c/h`, `pressure_app.c`, `CMakeLists.txt`
**Expected behavior:** Aralık dışı ADC/stuck GPIO → tanı bayrağı + güvenli davranış.
**Tests / validation:** Build; sahte hata enjeksiyonu (MANUAL).
**Rollback plan:** Yeni dosyaları sil.
**Diff budget:** 2 değişen, 2 yeni.
**Done criteria:**
- [ ] Range-check + read-back çalışıyor
- [ ] Tanı bayrağı alarm yoluna bağlı

## CARD-7.1 — Donanım Bring-up Checklist (MANUEL AĞIRLIKLI)

**Purpose:** INTEGRATION.md §7'nin tamamının ilk kez uçtan uca koşulması.
**Scope:** ST-Link flash, DAC/I_OUT doğrulama, buton testi, kalibrasyon, loop çıkışı, damping. Sonuçlar manual_steps + changelog'a.
**Dependencies:** P1–P4 kartları
**Files allowed to edit:** — (yalnızca dokümantasyon)
**Tests / validation:** Seviye 5 donanım testi.
**Done criteria:**
- [ ] Tüm adımlar geçti, sonuç değerleri kayıtlı

## CARD-7.2 — Kalibrasyonlu Soak + Release Adayı

**Purpose:** Sürüm hazırlığı.
**Scope:** Referans basınçla kalibrasyon, 24 h soak (reset/drift kaydı), Release build, sürüm notu, git tag.
**Dependencies:** CARD-7.1, P5, P6
**Done criteria:**
- [ ] 24 h kesintisiz, reset yok
- [ ] Release elf + tag

---

## Bu Backlog'un Dışında

| Öğe | Neden |
|---|---|
| HART/MODBUS/LP konfigleri, DAC8742/DAC161S997, EA_DOGS104N | Farklı konfigürasyonlar — ayrı görev |
| Tam EN 61508 tanı seti (RAM March, saat çapraz, RDP) | Sertifikasyon kapsamı — ayrı görev |
| BLE OTA, bonding/şifreleme | v2 |
| Unit test altyapısı (host-side) | İstenirse ayrı kart açılır — önerilir |
