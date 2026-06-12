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

## TASK PACKET CARD-1.2 — 2026-06-12

**Goal:** TMP108 ortam sıcaklığı izleme + T_HIGH=60 °C alert → FLT_TEMP# (PB5) işleme.
**Non-goals:** Kompanzasyona katkı YOK (diyotların işi); TMP108 failover kaynağı değil.
**Current state:** TMP108 sürücüsü yok; PB5 EXTI falling .ioc'ta tanımlı ve EXTI5 IRQ açık; main.c callback'inde FLT_TEMP işlenmiyor.
**Datasheet teyitleri (TMP108AIYFFT.pdf):** 12-bit 0.0625 °C/LSB sol-hizalı; config 0x2230 = continuous + 1 Hz + comparator + POL aktif-LOW + HYS=4 °C (maks — 60'ta kalkar ~56'da düşer); THIGH=60 °C → 0x3C00; TLOW=-50 °C → 0xCE00 (alt limit fiilen devre dışı); ALERT dahili ~100k pull-up; adres 0x48/0x49/0x4A/0x4B (A0 pini).
**Exact files inspected:** TMP108AIYFFT.pdf (register map), gpio.c (PB5 EXTI ✓), main.c USER CODE 4, pressure_app.c (tam), fdc2214.c (I2C deseni).
**Files allowed to edit:** yeni App/Src/tmp108.c + App/Inc/tmp108.h; Firmware/CMakeLists.txt; App/Src/pressure_app.c; Core/Src/main.c (yalnız USER CODE blokları)
**Files forbidden:** Drivers/**, Core/** USER CODE dışı, .ioc
**Expected behavior after:**
- Init: adres tarama (IsDeviceReady 0x48→0x4B), config+THIGH/TLOW yaz + read-back doğrula; başarısızlık ölümcül değil (ortam izleme yokken ölçüm sürer, tanı bayrağı)
- 1 Hz ortam okuma; FLT_TEMP# düşmesinde EXTI → anında overtemp bayrağı; comparator histeresisle pin kalkınca poll'da temizlenir
- Ekran satır 3 önceliği: *FAULT* > SENSOR ERR > AMB HOT > OK; alarmda ölçüm DEVAM eder (D8)
**Validation commands:** cmake --build build/Debug
**Manual validation scenario:** Donanımda: oda sıcaklığı makullüğü; THIGH'ı geçici 30 °C yazıp ısıtınca FLT_TEMP# düşmesi + AMB HOT; soğuyunca otomatik temizlenme.
**Rollback plan:** git checkout -- <dosyalar>; yeni dosyaları sil.
**Diff budget:** 3 değişen (CMakeLists, pressure_app.c, main.c USER CODE), 2 yeni.
**Done criteria:** temiz build; config read-back doğrulaması kodda; EXTI + poll yolu kodda; donanım MANUAL-4.
**Stop conditions:** main.c USER CODE dışına dokunmak gerekirse dur.

## TASK PACKET CARD-1.3 — 2026-06-12

**Goal:** 1N4148 kompanzasyon diyotları çift kanal (PC0=TMP_ADC1, PC1=TMP_ADC2): ikinci kanal okuma, geçerlilik + çapraz makullük, tutarlı çıkış.
**Non-goals:** Kompanzasyon matematiği (CARD-2.1); TMP108 (ayrı rol); Vf25/TC flash persistansı (CARD-2.1 format v2 notu).
**Current state:** temp_diode.c tek kanal (PC0); ADC 4 kanal hazır (MANUAL-3); ADC_RANK_TDIODE2 tanımlı ama kullanılmıyor; state_machine'de Vf25/TC edit'i birbirini varsayılana eziyor (bug).
**Exact files inspected:** temp_diode.c (tam), temp_diode.h (tam), state_machine.c (menü Vf25/TC bölümleri), pressure_app.c (ADC tüketimi — bu oturumda tam).
**Files allowed to edit:** App/Src/temp_diode.c, App/Inc/temp_diode.h, App/Src/pressure_app.c, App/Src/state_machine.c (4 değişen — kart önceden +1 onaylı)
**Files forbidden:** Core/**, Drivers/**, .ioc, bsp_pins.h (değişiklik gerekmez)
**Expected behavior after:**
- Her iki diyot kanalı okunur; kanal geçerlilik aralığı V_f 200–1000 mV (açık/kısa devre tespiti)
- |T1−T2| ≤ 5 °C → çıkış ortalama; aşım → tutarsızlık bayrağı + çıkış son tutarlı değerde tutulur
- Tek kanal geçersizse diğeriyle devam + bayrak; ikisi de geçersizse son değer + bayrak
- 1N4148 parametreleri (V_f25≈600 mV @ ~100 µA, TC≈−2 mV/°C; onsemi 1N914-D.PDF 1N4x48 dahil) yorumda belgeli
- Menü bug fix: Vf25 düzenlemek TC'yi (ve tersi) artık ezmiyor; edit mevcut değerden başlıyor
**Validation commands:** cmake --build build/Debug
**Manual validation scenario:** Donanımda iki kanal oda sıcaklığında ±5 °C içinde; bir diyot bağlantısı koparılınca bayrak + tek kanalla devam.
**Rollback plan:** git checkout -- <4 dosya>
**Diff budget:** 4 değişen (onaylı), 0 yeni.
**Done criteria:** temiz build; çift kanal + makullük + bayrak kodda; menü bug fix; donanım MANUAL-4.
**Stop conditions:** bsp_pins/Core değişikliği gerekirse dur.

## TASK PACKET CARD-1.4 — 2026-06-12

**Goal:** Sıcaklık rollerinin entegrasyonu: diyot tutarsızlık bayrağı → ekran tanı yolu; rol ayrımının pressure_app'te belgelenmesi.
**Non-goals:** Kompanzasyon matematiği (CARD-2.1); TMP108→diyot failover (bilinçli YOK — farklı fiziksel konum).
**Current state:** temp_diode arbitrasyonu + is_consistent() hazır (CARD-1.3); bayrak hiçbir yerde gösterilmiyor. TMP108 AMB HOT ekranda (CARD-1.2).
**Politika kararı (D2 uzantısı):** Diyot tutarsızlığı ölçümü DURDURMAZ ve alarm akımı tetiklemez — kompanzasyon son tutarlı/tek kanal değerle sürer, ekranda "TDIODE ERR" uyarısı (degraded-but-operational). Alarm-low yalnız basınç sensörü (FDC) hatasında.
**Exact files inspected:** pressure_app.c (bu oturumda tam + CARD-1.3 diff'i), temp_diode.h (bu oturumda yazıldı).
**Files allowed to edit:** App/Src/pressure_app.c
**Expected behavior after:** Ekran satır 3 önceliği: *FAULT* > SENSOR ERR > TDIODE ERR > AMB HOT > OK; dosya başında sıcaklık mimarisi rol bloğu.
**Validation commands:** cmake --build build/Debug
**Manual validation scenario:** Donanımda bir diyot kanalı koparılınca "TDIODE ERR" + ölçüm devam; geri takılınca kaybolur.
**Rollback plan:** git checkout -- Firmware/App/Src/pressure_app.c
**Diff budget:** 1 değişen, 0 yeni.
**Done criteria:** temiz build; öncelik zinciri + rol dokümantasyonu kodda.
**Stop conditions:** —

## TASK PACKET CARD-2.1 — 2026-06-12

**Goal:** Kompanzasyon modeli v2 (zero+span katsayıları) + flash format v2 (vf25/tc persistansı) + v1→v2 migrasyon.
**Non-goals:** Kalibrasyon iş akışı sağlamlaştırma (CARD-2.2); >2 nokta kalibrasyon.
**Current state:** v1: tek k_t; vf25/tc yalnız RAM (reboot'ta kayıp); menü 10 öğe; dC_to_pressure tek katsayılı.
**Model v2:** ΔT = T − T_ref; P = P_raw − (k_t_zero + k_t_span·frac)·ΔT  (frac = ham okuma oranı 0..1 → zero kayması her yerde, span kayması skala ile orantılı).
**Exact files inspected:** cal_storage.h (tam), cal_storage.c (tam), state_machine.c (tam), pressure_app.c (bu oturumda tam).
**Files allowed to edit:** cal_storage.h/c, state_machine.c, pressure_app.c (4 — kartta önceden onaylı). state_machine.h'a DOKUNULMAYACAK (menü sayısı pressure_app'te "/11" senkron yorumuyla).
**Migrasyon:** cal_init: version==2 → yükle; version==1 + v1-boyutlu CRC doğru → alanları kopyala, yeniler default (k_t_span=0, vf25=600, tc=-2); aksi → defaults. v1 layout cal_storage.c içinde legacy struct olarak korunur.
**Senkron kuralı:** vf25/tc'nin tek kaynağı cal_params; temp_diode_set_calibration boot'ta, menü commit'inde ve Exit(no save) reload'unda çağrılır.
**Validation commands:** cmake --build build/Debug
**Manual validation scenario:** Donanımda: v1 kayıtlı cihazda boot → değerler korunmuş + yeni alanlar default; Vf25 değiştir + Save & Exit + reboot → kalıcı.
**Rollback plan:** git checkout -- <4 dosya>. Flash'taki eski v1 kaydı migrasyonla okunabilir kalır (geri dönüş güvenli).
**Diff budget:** 4 değişen (onaylı), 0 yeni.
**Done criteria:** temiz build; v2 struct + migrasyon + model + menü öğeleri kodda; donanım MANUAL-4.
**Stop conditions:** state_machine.h veya başka dosya gerekirse dur.
