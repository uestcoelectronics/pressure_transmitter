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

## TASK PACKET CARD-2.2 — 2026-06-12

**Goal:** Kalibrasyon iş akışı sağlamlaştırma: stabilite eşiği, geçersiz yakalama/kayıt reddi, kaydetmeden çıkış onayı.
**Non-goals:** Kompanzasyon matematiği; çok noktalı kalibrasyon; BLE.
**Current state:** CAL_LIVE her SET-long'da koşulsuz yakalıyor; Save & Exit doğrulamasız; Exit (no save) onayız anında iptal ediyor.
**Tasarım:**
- Stabilite: CAL_LIVE'da son 8 örnek (~0.8 s) ΔC peak-to-peak ≤ CAL_STAB_P2P_MAX (2000 count, donanımda ayarlanır) → STABLE; değilse yakalama reddi
- Span yakalama: |ΔC − cap_at_zero| ≥ CAL_MIN_SPAN_COUNTS (10000) değilse red ("SPAN TOO CLOSE")
- Save & Exit: |cap_at_span−cap_at_zero| < CAL_MIN_SPAN veya p_max ≤ p_min → kaydetme, "CAL INVALID" mesajı, menüde kal
- Exit (no save): değişiklik varsa (dirty) ilk SET "Discard? SET again" → ikinci SET iptal eder; dirty değilse direkt çıkış
- Transient bilgi mesajı API'si: sm_get_info_msg() (menü satır 2'de gösterilir, navigasyonda silinir); sm_cal_live_stable() (CAL_LIVE satır 3: "STABLE SET long" / "WAIT unstable")
**Exact files inspected:** state_machine.h (tam), state_machine.c (tam, bu oturum), pressure_app.c render fonksiyonları (bu oturum).
**Files allowed to edit:** App/Inc/state_machine.h, App/Src/state_machine.c, App/Src/pressure_app.c (3 değişen — bütçe içinde)
**Validation commands:** cmake --build build/Debug
**Manual validation scenario:** Donanımda: basınç değişirken yakalama denenince red; stabilken yakalama; zero=span ile Save reddi; değişiklik sonrası Exit çift onay.
**Rollback plan:** git checkout -- <3 dosya>
**Diff budget:** 3 değişen, 0 yeni.
**Done criteria:** temiz build; stabilite+red+onay akışları kodda; donanım MANUAL-4.
**Stop conditions:** başka dosya gerekirse dur.

## TASK PACKET CARD-4.1 — 2026-06-13

**Goal:** Loop geri besleme makullüğü (komut↔ölçüm sapma tanısı) + NAMUR NE43 alarm/satürasyon seviyeleri + fault kurtarma politikası.
**Non-goals:** HART; DAC kalibrasyonu; INA190 kazanç ayarı.
**Current state / bulunan buglar:**
1. ma_to_dac_code 3.8 mA alt clamp → loop_set_current_ma(3.6) fiilen 3.8 üretiyor: ALARM-LOW HİÇ ÇIKMIYOR
2. xtr111_loop.h transfer yorumu (R_SET=15Ω) ↔ .c (R_SET=1.2kΩ, 0.12 V/mA) çelişkili — .c otorite, şema teyidi MANUAL-2'de
3. Sapma tanısı yok; XTR111 FLT fault'u sonsuza dek latched (kurtarma yok)
**Tasarım:**
- NAMUR NE43 sabitleri (header): ALARM_LOW=3.6, SAT_LOW=3.8, SAT_HIGH=20.5, ALARM_HIGH=21.0; komut clamp penceresi 3.6..21.0
- pressure_to_ma: aralık dışı ölçüm → 3.8 / 20.5 satürasyonu (4/20 değil)
- Sapma monitörü loop_service(now): enabled+!fault iken |cmd−meas| > 0.3 mA kesintisiz 2 s → deviation bayrağı; eşik altı kesintisiz 2 s → otomatik temizlenir. LED hızlı blink + ekran "LOOP DEV"
- Fault kurtarma: FLT latch 5 s sonra otomatik retry; retry öncesi FLT pini hâlâ LOW ise erteleme (edge kaçırma önlemi — EXTI yalnız kenar görür)
**Exact files inspected:** xtr111_loop.c (tam), xtr111_loop.h (tam), pressure_app.c (bu oturumlarda tam).
**Files allowed to edit:** App/Src/xtr111_loop.c, App/Inc/xtr111_loop.h, App/Src/pressure_app.c (3 — bütçe içinde)
**Validation commands:** cmake --build build/Debug
**Manual validation scenario:** Donanımda multimetre: 4/12/20 mA ±0.05; sensör hatasında gerçek 3.6 mA; loop teli koparılınca FLT→safe state→5 s sonra otomatik retry; yük direnci değiştirilerek sapma tanısı.
**Rollback plan:** git checkout -- <3 dosya>
**Diff budget:** 3 değişen, 0 yeni.
**Done criteria:** temiz build; NAMUR seviyeleri + sapma monitörü + retry kodda; donanım MANUAL-4.
**Stop conditions:** başka dosya gerekirse dur.

## TASK PACKET CARD-3.1 — 2026-06-13

**Goal:** LCD güç sırası (LCD_PWR_ON/PA10) + datasheet reset zamanlaması + üretici ST7789V init dizisinin (güç kontrol + gamma) eklenmesi.
**Non-goals:** Menü/UI mantığı (CARD-3.2); font/render değişikliği.
**Current state / bulgular:** lcd_init PA10'u sürüyor ✓ ama güç oturma süresi sabit-yorumsuz (20 ms). Reset 20/20/150 ms. **Init dizisi eksik:** yalnız SWRESET/SLPOUT/MADCTL/COLMOD/INVON/DISPON var; üretici dizisindeki 0xB2 (porch), 0xB7 (gate), 0xBB (VCOM), 0xC2/C3/C4/C6 (VRH/VDV/FRCTRL), 0xD0 (power ctrl), 0xE0/E1 (gamma ±) YOK → default güç/gamma ile sönük/yanlış kontrast riski.
**Datasheet (DS154S10Z0TG01.pdf) üretici referans dizisi:** RES low 100ms→high 100ms; 0x11+120ms; 0x36=00; 0x3A=05; 0xB2 0C 0C 00 33 33; 0xB7 35; 0xBB 32; 0xC2 01; 0xC3 15; 0xC4 20; 0xC6 0F; 0xD0 A4 A1; 0xE0/E1 14'er bayt gamma; 0x21 INVON; 0x29 DISPON. BLK aktif-HIGH dijital backlight (PWM ile dimming geçerli).
**Tasarım:** Adlandırılmış zamanlama sabitleri; PA10 power settle 50 ms; HW reset (low 10 ms, high→120 ms bekleme, SLPOUT öncesi); SWRESET kaldırılıyor (HW reset yeterli, üretici de kullanmıyor); tam register dizisi cmd+data helper'la; ekran temizliği DISPON öncesi (garbage flash önlemi — backlight DISPON sonrası açılıyor, mevcut iyi UX korunuyor).
**Exact files inspected:** lcd400.c (tam), lcd400.h (tam), datasheet init dizisi (çıkarıldı), gpio.c (PA10/PC8 konfig — önceki kart).
**Files allowed to edit:** App/Src/lcd400.c (1 dosya)
**Files forbidden:** lcd400.h (imza değişmiyor), Core/**, .ioc
**Validation commands:** cmake --build build/Debug
**Manual validation scenario:** Donanımda 10× soğuk açılış → ekran her seferinde ilk denemede içerik gösteriyor; kontrast/renk düzgün (gamma sonrası); backlight açılışta garbage göstermiyor.
**Rollback plan:** git checkout -- Firmware/App/Src/lcd400.c
**Diff budget:** 1 değişen, 0 yeni.
**Done criteria:** temiz build; güç sırası + reset zamanlaması sabitleri + tam üretici init dizisi kodda; donanım MANUAL-4.
**Stop conditions:** lcd400.h veya başka dosya gerekirse dur.

## TASK PACKET CARD-3.2 — 2026-06-13

**Goal:** Menü/UI iyileştirmeleri: 60 s eylemsizlik timeout→NORMAL (discard); NORMAL ekranda sayfa geçişi (MAIN/SENSOR/LOOP — P/T/mA/ΔC); fault/alarm ekranı; backlight % menü öğesi.
**Non-goals:** Storage formatı değişikliği (backlight runtime-only bu kartta); BLE; render motoru.
**Current state:** sm event-driven, zaman bilgisi yok; NORMAL'de UP/DN/SET-short işlevsiz; render_normal tek sayfa; menü 11 öğe; backlight lcd_init'te sabit %60.
**Tasarım kararları:**
- Timeout: sm_tick(now) 5 ms tikte; her event s_last_activity'yi damgalar; non-NORMAL'de 60 s eylemsizlik → cal_init+temp sync (discard) → NORMAL
- Sayfalar: s_normal_page 0..2 (MAIN/SENSOR/LOOP); NORMAL'de UP/DN döndürür; SET_LONG menüye girer (korunur)
- Fault ekranı: loop_is_in_fault() → render_fault() (sayfadan bağımsız; CARD-4.1 5 s auto-retry ile kendiliğinden temizlenir)
- Backlight: MI_BACKLIGHT menü öğesi (12 öğe); edit step 5; commit canlı lcd_set_backlight; **kalıcı DEĞİL** (boot %60) — s_dirty tetiklemez (cal değişikliği değil)
**Exact files inspected:** state_machine.c (tam), state_machine.h (tam), pressure_app.c (render+loop, tam).
**Files allowed to edit:** App/Inc/state_machine.h, App/Src/state_machine.c, App/Src/pressure_app.c (3 — bütçe içinde)
**Validation commands:** cmake --build build/Debug
**Manual validation scenario:** Donanımda: menüde 60 s bekle → NORMAL'e dönüş; NORMAL UP/DN ile 3 sayfa; loop fault → alarm ekranı; backlight menüsü → anlık parlaklık değişimi.
**Rollback plan:** git checkout -- <3 dosya>
**Diff budget:** 3 değişen, 0 yeni.
**Done criteria:** temiz build; timeout+sayfalar+fault ekranı+backlight kodda; donanım MANUAL-4.
**Stop conditions:** storage/başka dosya gerekirse dur (backlight persistansı ayrı karta).

## TASK PACKET CARD-6.1 — 2026-06-13

**Goal:** TPS3851 windowed watchdog kick stratejisi: deterministik düşen-kenar besleme, güvenlik görevi canlılığına koşullu (A.16), flash erase sırasında starvation önlemi; datasheet zamanlama analizi.
**Non-goals:** Tam FMEDA tanı seti (CARD-6.2); CWD pencere mutlak değeri (şema — MANUAL-2).
**Web bulgusu (TPS3851 SBVS300B):** WDI düşen-kenar, pencereli (erken kick=fault). tWD CWD'ye bağlı (std 0.7ms-3.23s, ext 62ms-77s). KICK_PERIOD pencere içinde olmalı (tWD_MIN<P<tWD_MAX).
**Current state / bulgular:** WDT slice 100 ms'de HAL_GPIO_TogglePin → düşen kenar fiilen her **200 ms** (toggle 2× periyot); koşulsuz (loop canlılığına bağlı değil — superloop hang'de durur ama sub-task hang'i yakalamaz); cal_save erase sırasında 100 ms slice çalışmaz.
**Tasarım:**
- wdt_feed_raw(): net düşen kenar (HIGH→kısa LOW darbe→HIGH) + IWDG refresh; KOŞULSUZ (cal_save gibi kontrollü uzun işlemlerden de çağrılır)
- Ana döngü 100 ms WDT slice: yalnız güvenlik görevi (sensör+loop tiki) son WDT_HEALTH_TIMEOUT_MS=400 ms içinde çalıştıysa besle → A.16 program-akış izleme; aksi halde besleme dur → harici WDT+IWDG reset → güvenli durum
- s_loop_token = now sensör 100 ms tikinin başında (periyodik scheduler kanıtı)
- cal_save: erase öncesi+sonrası ve program döngüsünde wdt_feed_raw() (extern) — uzun flash op dog'u aç bırakamaz
- KICK_PERIOD/HEALTH tek sabit, pencere donanım teyidine kadar 100 ms (extended timing varsayımı)
**Exact files inspected:** pressure_app.c (WDT+sensör slice, tam), cal_storage.c (erase/program, tam), iwdg.c (Window=Reload=4095 → IWDG penceresiz, ~256 ms), datasheet (indirildi).
**Files allowed to edit:** App/Src/pressure_app.c, App/Src/cal_storage.c (2 — bütçe içinde)
**Validation commands:** cmake --build build/Debug
**Manual validation scenario:** Donanımda: normal çalışmada reset yok; kick durdurarak (kod geçici) reset gözlemi; cal_save sırasında reset yok; CWD penceresine göre KICK_PERIOD doğrula.
**Rollback plan:** git checkout -- <2 dosya>
**Diff budget:** 2 değişen, 0 yeni.
**Done criteria:** temiz build; deterministik kenar + health gating + flash-safe feed kodda; MANUAL-2 CWD maddesi; donanım MANUAL-4.
**Stop conditions:** ek dosya gerekirse dur.

## TASK PACKET CARD-6.2 — 2026-06-13

**Goal:** Temel tanılar (diag modülü): ADC kanal rail-stuck (A.13), output GPIO read-back (A.7), I2C bus recovery (9-clock); tanı bayrağı → ekran + güvenlik yolu.
**Non-goals:** Tam EN 61508 seti (RAM March, saat çapraz); divider mutlak gerilim ölçümü (ratio şemadan — tunable bırakıldı); driver dosyalarına dokunma.
**Tasarım kararları:**
- ADC tanısı **divider-bağımsız**: VCC_FB ve I_loop ham ADC'nin 0 veya full-scale'e takılması (kopuk kanal/mux arızası). Mutlak gerilim eşiği YOK (divider ratio bilinmiyor — MANUAL).
- GPIO read-back: ODR (komut) vs IDR (gerçek) karşılaştırma; izlenen output'lar LOOP_EN(PB2,kritik), LCD_PWR(PA10), BLE_PWR(PC2), CLK_EN(PD2); 2-örnek kalıcılık (transient elenir)
- LOOP_EN read-back uyuşmazlığı = güvenlik-kritik → diag_critical() → pressure_app loop_set_safe_state(); diğer bayraklar advisory ("DIAG CHK" ekran)
- I2C bus recovery: diag fiziksel kurtarma sağlar (PB8/PB9 GPIO-OD, 9 clock, STOP, AF4 geri, MX_I2C1_Init); pressure_app orkestre eder: fdc VE tmp108 birlikte N tik sağlıksızsa kurtar + cihazları yeniden init (cooldown'lu)
- diag driver'lara DOKUNMAZ — sağlığı fdc2214_has_error()/tmp108_is_ok() ile gözler (katman temiz)
**Exact files inspected:** pressure_app.c (tam), bsp_pins.h (pin makroları + ADC rank), fdc2214.h/tmp108.h (sağlık API), i2c.h (MX_I2C1_Init), gpio.c (PB8/PB9 AF4).
**Files allowed to edit:** yeni App/Src/diag.c + App/Inc/diag.h; App/Src/pressure_app.c; Firmware/CMakeLists.txt (2 değişen + 2 yeni — bütçe içinde)
**Files forbidden:** fdc2214.c/tmp108.c (driver), Core/**, .ioc
**Validation commands:** cmake --build build/Debug
**Manual validation scenario:** Donanımda: VCC_FB kanalını kısa/aç → DIAG bayrağı; LOOP_EN'i zorla → safe state; I2C SDA'yı GND'ye çek → bus recovery + cihaz reinit.
**Rollback plan:** git checkout -- pressure_app.c CMakeLists.txt; yeni dosyaları sil.
**Diff budget:** 2 değişen, 2 yeni.
**Done criteria:** temiz build; range-check + read-back + bus recovery kodda; tanı bayrağı alarm/ekran yoluna bağlı; donanım MANUAL-4.
**Stop conditions:** driver/Core değişikliği gerekirse dur.

## TASK PACKET CARD-5.1 — 2026-06-13

**Goal:** DL-CC2340-B BLE taşıma katmanı: USART3 IT RX ring buffer, TX, güç/reset/mode pin yönetimi + boot sırası, AUX (data-ready) erişimi.
**Non-goals:** AT komut protokolü / advertise / GET_MEAS / SET_PARAM (CARD-5.2); bonding/şifreleme.
**Datasheet (DL-CC2340-B V1.0, C19273634.pdf) teyitleri:**
- **Varsayılan baud 115200 8N1** (firmware USART3 ile UYUMLU); AT formatı `AT+<cmd> p1,p2` → `OK\r\n`/`ERROR:<n>\r\n`; async URC: `+CONNOK:Handle,Addr,Role,Num`, `+DISCONN:...`, `+SCANRET:...`
- Pinler: DIO20=UART-TX (→MCU RX=PC10), DIO22=UART-RX (←MCU TX=PC11), **DIO24=MODE** (sleep ctrl, default HIGH=wake → BLE_MODE/PB12), **DIO21=AUX** (data-ready/busy → BLE_EVENT/PB0), **RESET** aktif-LOW (→BLE_RESET/PA15 OD), VCC (→BLE_PWR_ON/PC2 enable)
- Reset duration <100 ms; AT↔transparent geçiş <2 ms
**Bulgu/karar:** USART3 NVIC CubeMX'te ENABLE DEĞİL (it.c'de handler yok). **.ioc değiştirmeden:** NVIC'i ble_uart_init'te HAL_NVIC ile enable + `USART3_IRQHandler`'ı ble_uart.c'de tanımla (startup zayıf sembol override). RX: HAL_UART_Receive_IT 1-bayt + RxCpltCallback ring'e push + re-arm. TX: bloklamalı (config trafiği düşük hız).
**Exact files inspected:** usart.c/h (115200 ✓), stm32u3xx_it.c (USART3 handler YOK), bsp_pins.h (BLE pin makroları — CARD-0.2'de eklendi), pressure_app.c (init/loop).
**Files allowed to edit:** yeni App/Src/ble_uart.c + App/Inc/ble_uart.h; App/Src/pressure_app.c; Firmware/CMakeLists.txt (2 değişen + 2 yeni)
**Files forbidden:** Core/**, .ioc (NVIC app'ten enable edilecek)
**Validation commands:** cmake --build build/Debug
**Manual validation scenario:** Donanımda nRF Connect ile modül görünürlüğü (advertise CARD-5.2'de); şimdilik UART loopback / AT "AT"→"OK" yankısı.
**Rollback plan:** git checkout -- pressure_app.c CMakeLists.txt; yeni dosyaları sil.
**Diff budget:** 2 değişen, 2 yeni.
**Done criteria:** temiz build; ring buffer + IT RX + pin/güç sırası + AUX erişimi + USART3 IRQ app'ten kodda; donanım MANUAL-4/5.
**Stop conditions:** USART3 IRQ app'ten enable edilemezse (.ioc gerekirse) dur ve MANUAL aç.

## TASK PACKET CARD-5.2 — 2026-06-13

**Goal:** BLE konfigürasyon protokolü: non-bloklayan AT init (advertise), transparent kanalda CRC16'lı çerçeve protokolü (GET_MEAS/GET_PARAM/SET_PARAM/SAVE/INFO/UNLOCK).
**Non-goals:** OTA, bonding/şifreleme; central/scan rolü.
**Datasheet AT formatları (teyitli):** AT+ADVNAME=<local>,<manuf>; AT+ADVSTA=<0|1>; AT+ENTM (gattRole 0=peripheral server); AT+RESET→+PWRUP; AT+SAVE. URC'ler (+CONNOK/+DISCONN) AT modunda; transparent modda veri akışı + AUX(DIO21) data-ready.
**Mimari karar:** İki faz — (1) AT init: line-based, her komuta "OK" bekle (timeout/retry, başarısızsa degraded→RUN); (2) RUN: pure transparent, byte-feed çerçeve parser. Transparent modda URC beklenmiyor; bağlantı durumu implicit (gelen geçerli çerçeve = aktivite). Non-bloklayan: ble_proto_service(now,p,t,ma,status) ana döngüden çağrılır, 4-20 loop durmaz.
**Çerçeve:** SOF=0xAA | LEN | CMD | PAYLOAD[LEN] | CRC16-CCITT(hi,lo) over [LEN,CMD,PAYLOAD]. Yanıt: SOF,LEN,CMD echo, payload[0]=status, veri. float=4B LE.
**Komutlar:** 0x01 GET_MEAS(P,T,mA,stat); 0x02 GET_PARAM<pid>; 0x03 SET_PARAM<pid><f32> (unlock'lu); 0x04 SAVE (unlock'lu); 0x05 INFO; 0x06 UNLOCK<u32 PIN>. PID: p_min/p_max/damping/kt_zero/kt_span/vf25/tc/cap_zero/cap_span. Yazma koruması: sabit PIN (D4/Q5), SET/SAVE kilitliyse ERR.
**Exact files inspected:** ble_uart.h (API), cal_storage.h (param alanları+cal_save), temp_diode.h (vf25/tc sync), pressure_app.c (loop+status), datasheet AT bölümü.
**Files allowed to edit:** yeni App/Src/ble_proto.c + App/Inc/ble_proto.h; App/Src/pressure_app.c; Firmware/CMakeLists.txt (1 değişen prod + 2 yeni)
**Files forbidden:** Core/**, .ioc, ble_uart.c (taşıma katmanı sabit)
**Validation commands:** cmake --build build/Debug
**Manual validation scenario:** Donanım: telefon (nRF Connect) ile "PT910" advertise gör; transparent karakteristiğe GET_MEAS çerçevesi yaz → P/T/mA yanıtı; UNLOCK+SET_PARAM+SAVE → reboot sonrası kalıcı.
**Rollback plan:** git checkout -- pressure_app.c CMakeLists.txt; yeni dosyaları sil.
**Diff budget:** 1 değişen prod, 2 yeni.
**Done criteria:** temiz build; AT init SM + frame parser + komutlar + CRC + unlock kodda; donanım MANUAL-5.
**Stop conditions:** ble_uart API yetmezse dur.
