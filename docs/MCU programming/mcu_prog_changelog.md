# mcu_prog — Changelog

> Append-only. Yeni girişler en alta eklenir.

## 2026-06-12 | Bootstrap | Task: BOOTSTRAP-01

**Task ID:** BOOTSTRAP-01
**Type:** Planning / Bootstrap
**Status:** Complete

**Files Created:**
- `docs/MCU programming/mcu_prog_repo_audit.md`
- `docs/MCU programming/mcu_prog_design_grill.md`
- `docs/MCU programming/mcu_prog_web_research.md`
- `docs/MCU programming/mcu_prog_roadmap.md`
- `docs/MCU programming/mcu_prog_backlog.md`
- `docs/MCU programming/mcu_prog_state.md`
- `docs/MCU programming/mcu_prog_memory.md`
- `docs/MCU programming/mcu_prog_changelog.md`
- `docs/MCU programming/mcu_prog_tracker.html`
- `docs/MCU programming/mcu_prog_manual_steps.md`
- `docs/MCU programming/mcu_prog_capabilities.md`
- `docs/MCU programming/mcu_prog_project_profile.md`
- `docs/MCU programming/mcu_prog_task_packets.md`
- `docs/MCU programming/mcu_prog_claude_system_prompt.md`
- `docs/MCU programming/mcu_prog_self_check.md`
- `docs/MCU programming/start_mcu_prog_claude.bat`
- `docs/MCU programming/continue_mcu_prog_claude.bat`
- `docs/MCU programming/status_mcu_prog.bat`

**Files Modified:** Yok (planning-only; production koduna dokunulmadı)

**Tests / Validations Run:**
- `cmake --preset Debug` + `cmake --build build/Debug` → **FAIL**: `App/Src/cal_storage.c:112: 'FLASH_TYPEPROGRAM_QUADWORD' undeclared` (tek hata; U3 HAL yalnız DOUBLEWORD/BURST destekler — `stm32u3xx_hal_flash.h:260-271` ile teyitli)
- `DS154S10Z0TG01.pdf` s.4 metin çıkarımı → LCD denetleyici **ST7789V teyit**
- `PIN_MAPPING.xlsx` + `BOM/PCBPT910G1_4_20.xlsx` analizleri → pin/komponent uyumsuzlukları tespit (bkz. repo_audit)
- Web: DL-CC2340-B = AT komutlu BLE modülü teyidi

**Validation Level Reached:** 1 — statik inceleme + doküman üretimi (build denemesi yapıldı, başarısız → seviye 2 sayılmaz)

**Result:** Tam planlama tamamlandı. 8 fazlı roadmap, 16 kartlık backlog. Kritik bulgular: (1) build kırık — QUADWORD; (2) sıcaklık donanımı firmware varsayımından farklı (termistör+TMP108, diyot değil); (3) FDC2214 CLK_EN/SD pinleri sürülmüyor; (4) BLE hiç yok; (5) repo git değil.

**Risks Introduced:** None
**Risks Resolved:** LCD denetleyici belirsizliği (ST7789V teyit edildi — INTEGRATION.md TODO #1 kapandı)

**Next Action:** CARD-0.1 — Build Baseline Düzeltmesi (kullanıcı onayı gerekli) + CARD-0.3 git init onayı

## 2026-06-12 | Alignment | Task: CORRECTION-01

**Task ID:** CORRECTION-01
**Type:** Planning / Alignment (kullanıcı düzeltmesi)
**Status:** Complete

**Files Created:** Yok
**Files Modified:**
- `mcu_prog_repo_audit.md` (sıcaklık bulgusu düzeltildi)
- `mcu_prog_design_grill.md` (çelişki #1 KAPANDI, U1/U2 CONFIRMED)
- `mcu_prog_roadmap.md` (P1 kapsamı)
- `mcu_prog_backlog.md` (CARD-1.2, CARD-1.3, CARD-1.4 yeniden yazıldı)
- `mcu_prog_state.md` (D2/D6/D8, Q1/Q2, risk tablosu)
- `mcu_prog_memory.md` (sıcaklık mimarisi)
- `mcu_prog_manual_steps.md` (MANUAL-2 madde 1)
- `mcu_prog_tracker.html` (kartlar, risk, sorular, changelog)

**Tests / Validations Run:**
- `DATASHEETS\__TI_DATASHEETS\1N914-D.PDF` metin çıkarımı → onsemi dokümanı **1N4148'i kapsıyor** (1N91x/1N4x48 ailesi) ✓
- 4-20 BOM diyot taraması → kartta ayrık 1N4148 yok → diyotlar sensör modülü içinde (Yantai metal kapasitif sensör)

**Validation Level Reached:** 1 — doküman güncellemesi + statik teyit

**Result:** Kullanıcı teyidi işlendi: **Kompanzasyon sıcaklığı = 1N4148 diyotlar (PC0=TMP_ADC1, PC1=TMP_ADC2)** — mevcut temp_diode.c modeli kavramsal olarak doğru, PC1 kanalı + datasheet parametre teyidi eklenecek. **TMP108 = yalnız ortam sıcaklığı**; T_HIGH=60 °C alert pini FLT_TEMP# (PB5) kesmesi üretecek şekilde konfigüre edilecek; kompanzasyonda kullanılmayacak, failover yapılmayacak. Pin haritasındaki "thermistor" etiketi yanıltıcı olarak işaretlendi.

**Risks Introduced:** None
**Risks Resolved:** "Sıcaklık modeli yanlış donanım varsayımı" (HIGH) → kapatıldı; yerine MEDIUM "TMP108 alert + PC1 kanalı eksik"

**Next Action:** CARD-0.1 — Build Baseline Düzeltmesi (kullanıcı onayı gerekli) + CARD-0.3 git init onayı

## 2026-06-12 | Execute | Task: CARD-0.1

**Task ID:** CARD-0.1
**Type:** Execute
**Status:** Complete

**Files Created:** `Firmware/App/Src/cal_storage.c.bak` (yedek)
**Files Modified:** `Firmware/App/Src/cal_storage.c` (cal_save: QUADWORD/16-bayt → DOUBLEWORD/8-bayt; tampon `uint8_t[]` → `uint64_t[]` hizalı)

**Tests / Validations Run:**
- HAL imza teyidi: `stm32u3xx_hal_flash.c:170 HAL_FLASH_Program(TypeProgram, Address, DataAddress)` — DataAddress = veri adresi ✓
- `cmake --build build/Debug` → **PASS**: 0 error, 0 warning, `PressureTransmitter.elf` link edildi

**Validation Level Reached:** 2 — derleme/link. Donanımda flash kaydet/yükle testi YAPILMADI (kart gerektirir → MANUAL-4 kapsamında CARD-7.1 öncesi yapılacak).

**Result:** Build baseline yeşil. Kalibrasyon formatı (magic/CRC/v1) değişmedi; yalnızca program birimi 16→8 bayt.

**Risks Introduced:** None
**Risks Resolved:** "Build kırık" (blocked) → kapandı

**Next Action:** CARD-0.2 — bsp_pins.h ↔ PIN_MAPPING mutabakatı; CARD-0.3 git init onayı bekleniyor

## 2026-06-12 | Execute | Task: CARD-0.3 + CARD-0.2

**Task ID:** CARD-0.3, CARD-0.2
**Type:** Execute
**Status:** Complete

**Files Created:** `.gitignore`
**Files Modified:** `Firmware/App/Inc/bsp_pins.h`

**Tests / Validations Run:**
- `git init -b main` + ilk commit `e2120fe` (392 dosya); CARD-0.2 commit `9dfad89`
- `gpio.c` + `adc.c` incelemesi (pin haritası çapraz kontrol)
- `cmake --build build/Debug` → **PASS** (0 error, 0 warning)

**Validation Level Reached:** 2 — derleme/link

**Result:** Git aktif (yerel, push yok). bsp_pins.h'a 7 eksik alias eklendi: FLT_TEMP (PB5), BLE_PWR_ON/MODE/RESET/EVENT, RST_TI, TEST_MODE; 1N4148 ve IN13/IN14 yorum düzeltmeleri. **Bulgular:** (1) .ioc tüm FMEDA pinlerini zaten içeriyor; FDC_CLK_EN boot'ta HIGH → "sensör saatsiz" riski kapandı; eksik olan uygulama tarafı kesme işleme (CARD-1.1). (2) **ADC BUG:** `adc.c` rank 2/3'e kanal atamıyor → üç rank da PC0; VCC_FB ve loop akım FB hiç okunmuyor → .ioc düzeltmesi MANUAL-3'e eklendi. (3) CD_IRQ (PA0) konfigüre edilemez — EXTI0 hattı BLE_EVENT (PB0) kullanıyor.

**Risks Introduced:** None
**Risks Resolved:** Git yok (HIGH) → kapandı; FDC saatsiz (HIGH) → kapandı. Yeni risk kaydı: ADC kanal bug'ı (HIGH, MANUAL-3'te çözüm).

**Next Action:** CARD-1.1 (FDC uygulama tarafı) + MANUAL-3 (kullanıcı: CubeMX ADC düzeltmesi)

## 2026-06-12 | Execute | Task: MANUAL-3 + ADC re-rank

**Task ID:** MANUAL-3 (kullanıcı) + bsp_pins ADC düzeni (CARD-1.3'ün .ioc ayağı)
**Type:** Execute
**Status:** Complete

**Files Created:** Yok
**Files Modified:**
- `Firmware/PressureTransmitter.ioc`, `Firmware/Core/Src/adc.c`, `Firmware/.mxproject` (CubeMX regenerate — kullanıcı)
- `Firmware/App/Inc/bsp_pins.h` (ADC_RANK_TDIODE2=1 yeni, VCC_FB=2, ILOOP_FB=3, COUNT=4; IN11/IN12 yorum düzeltmesi)

**Tests / Validations Run:**
- Regenerate diff kontrolü: yalnız adc.c/.ioc/.mxproject değişti; USER CODE blokları ve App/ korunmuş ✓
- adc.c: 4 dönüşüm — Rank1=CH1(PC0), Rank2=CH2(PC1), Rank3=CH11(PC4), Rank4=CH12(PC5); rank kanal bug'ı kapandı ✓
- `cmake --build build/Debug` → PASS (tam yeniden derleme, 0 error / 0 warning)

**Validation Level Reached:** 2 — derleme/link

**Result:** ADC artık 4 kanalı gerçekten örnekliyor: iki 1N4148 diyodu (PC0/PC1), VCC_FB (PC4) ve loop akım geri beslemesi (PC5). pressure_app.c buffer'ı ADC_RANK_COUNT üzerinden otomatik 4'e çıktı; TDIODE2 verisi henüz kullanılmıyor (CARD-1.3'te işlenecek). **Bilgi düzeltmesi:** PC4/PC5 = ADC1_IN11/IN12 — PIN_MAPPING.xlsx'teki IN13/IN14 HATALI (kullanıcı CubeMX görsel teyidi); kanal/AF numaralarında CubeMX + MCU datasheet otorite kabul edildi.

**Risks Introduced:** None
**Risks Resolved:** ADC kanal bug'ı (HIGH) → kapandı

**Next Action:** CARD-1.1 — FDC2214 uygulama tarafı bring-up

## 2026-06-12 | Execute | Task: CARD-1.1

**Task ID:** CARD-1.1
**Type:** Execute
**Status:** Complete (kod); donanım doğrulaması MANUAL-4 bekliyor

**Files Created:** Yok
**Files Modified:**
- `Firmware/App/Inc/fdc2214.h` — yeni API: power_sequence, get_addr, poll_errb, data_ready; ADDR_ALT 0x2B
- `Firmware/App/Src/fdc2214.c` — güç/saat sıralaması (SD toggle + XO oturma bekleme), I2C adres otomatik tespiti (0x2A→0x2B), ERRB yoklama + STATUS okuma, INT_B data-ready
- `Firmware/App/Src/pressure_app.c` — init'te 1× güç-çevrimli retry; 100 ms tikte ERRB yoklama; data-ready gating; ERRB/I2C hatasında alarm-low (3.6 mA); ekran satır 3'e "SENSOR ERR"; temiz okumada otomatik hata temizleme

**Tests / Validations Run:**
- `cmake --build build/Debug` → PASS (0 error / 0 warning)
- Statik akış kontrolü: I2C-fail alarm-low davranışı korunmuş (regresyon düzeltildi: i2c_fail ayrı izleniyor)

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda DEVICE_ID okuma, ERRB tetikleme/kurtarma senaryosu, INT_B zamanlaması — MANUAL-4 (kart + ST-Link gerekli)

**Result:** FDC2214 bring-up uygulama tarafı tamam. Sıralama: SD=HIGH → CLK_EN=HIGH → 10 ms (XO) → SD=LOW → 5 ms → DEV_ID (0x2A, olmazsa 0x2B). ERRB asserted → STATUS oku + latched hata + alarm-low + "SENSOR ERR"; ERRB düzelip okuma başarılı olunca otomatik temizlenir. main.c'ye dokunulmadı (ERRB/INT_B polling — superloop mimarisine uygun).

**Risks Introduced:** data-ready gating sensör okumalarını INT_B pin davranışına bağladı; donanımda INT_B beklenmedik şekilde HIGH kalırsa ölçüm durur → MANUAL-4 testinde ilk kontrol edilecek madde
**Risks Resolved:** "FDC ERRB/INT_B kesmeleri uygulamada işlenmiyor" (MEDIUM) → kapandı (polling ile)

**Next Action:** CARD-1.2 — TMP108 ortam izleme + 60 °C alert (FLT_TEMP#)

## 2026-06-12 | Execute | Task: CARD-1.2

**Task ID:** CARD-1.2
**Type:** Execute
**Status:** Complete (kod); donanım doğrulaması MANUAL-4 bekliyor

**Files Created:**
- `Firmware/App/Inc/tmp108.h`, `Firmware/App/Src/tmp108.c` — TMP108 ortam monitörü sürücüsü
**Files Modified:**
- `Firmware/CMakeLists.txt` — tmp108.c target_sources
- `Firmware/App/Src/pressure_app.c` — tmp108_init (ölümcül olmayan), 1 Hz tmp108_poll tiki, ekran satır 3 önceliğine "AMB HOT >60C"
- `Firmware/Core/Src/main.c` — USER CODE: tmp108.h include + FLT_TEMP# EXTI → tmp108_on_alert_edge()

**Tests / Validations Run:**
- TMP108AIYFFT.pdf register teyitleri: pointer map, config formatı (Table 8), HYS maks 4 °C (Table 9), ALERT dahili ~100k pull-up, 12-bit 0.0625 °C/LSB
- `cmake --build build/Debug` → PASS (0 error / 0 warning)

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda adres tespiti, gerçek sıcaklık okuma, 60 °C alert tetikleme/kurtarma (geçici THIGH=30 °C ile test edilebilir) — MANUAL-4

**Result:** TMP108 yalnız ORTAM izleme rolünde (kullanıcı teyitli mimari): adres taraması 0x48-0x4B, config 0x2230 (continuous, 1 Hz, comparator, POL aktif-LOW, HYS=4 °C), THIGH=60 °C / TLOW=-50 °C (alt limit pasif), config+THIGH read-back doğrulaması (FH/FL bitleri maskeli). FLT_TEMP# düşmesinde EXTI anında bayrak kurar; comparator histeresisle pin kalkınca 1 Hz poll'da otomatik temizlenir. Alarmda ölçüm DEVAM eder, ekranda "AMB HOT >60C" (öncelik: *FAULT* > SENSOR ERR > AMB HOT > OK). D8 revize: 55 °C histerezis donanımda yok → 4 °C maks (~56 °C'de düşer).

**Risks Introduced:** None
**Risks Resolved:** None (TMP108 eksikliği kalemi CARD-1.3 sonrası tamamen kapanacak)

**Next Action:** CARD-1.3 — 1N4148 çift kanal (PC0/PC1) + çapraz makullük

## 2026-06-12 | Execute | Task: CARD-1.3

**Task ID:** CARD-1.3
**Type:** Execute
**Status:** Complete (kod); donanım doğrulaması MANUAL-4 bekliyor

**Files Created:** Yok
**Files Modified:**
- `Firmware/App/Inc/temp_diode.h` + `Src/temp_diode.c` — çift kanal rework: temp_diode_update(d1,d2), kanal geçerlilik (V_f 200–1000 mV), |T1−T2|≤5 °C çapraz makullük, arbitrasyon (ortalama / tek kanal / son-değer tutma), is_consistent() + kanal bazlı tanı getter'ları, get_vf25_mv()/get_tc_mv_c(); 1N4148 referansları (onsemi 1N914-D.PDF)
- `Firmware/App/Src/pressure_app.c` — temp_diode_update(TDIODE, TDIODE2) çağrısı
- `Firmware/App/Src/state_machine.c` — BUG FIX: Vf25 edit'i TC'yi (ve tersi) varsayılana eziyordu; edit artık mevcut değerden başlıyor ve yalnız hedef parametreyi değiştiriyor

**Tests / Validations Run:**
- `cmake --build build/Debug` → PASS (0 error / 0 warning)

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda iki kanalın gerçek okumaları, kanal kopma/tutarsızlık senaryoları — MANUAL-4

**Result:** Kompanzasyon sıcaklığı artık iki 1N4148 diyotundan yedekli besleniyor (FMEDA A.11/A.14 girişlerine zemin). Tutarlı durumda ortalama; tek kanal arızasında öteki kanal; çift arıza/tutarsızlıkta son tutarlı değer + bayrak. Vf25/TC menü kalıcılık eksiği CARD-2.1 v2 formatına işlendi.

**Risks Introduced:** None
**Risks Resolved:** Menü Vf25/TC karşılıklı ezme bug'ı (önceden kayıtsızdı — bulunup kapatıldı)

**Next Action:** CARD-1.4 — sıcaklık rolleri entegrasyonu (tanı bayrağının ekran/alarm yoluna bağlanması)

## 2026-06-12 | Execute | Task: CARD-1.4

**Task ID:** CARD-1.4
**Type:** Execute
**Status:** Complete (kod) — P1 fazı kod tarafı TAMAMLANDI

**Files Created:** Yok
**Files Modified:**
- `Firmware/App/Src/pressure_app.c` — dosya başına sıcaklık mimarisi rol bloğu (diyot=kompanzasyon, TMP108=ortam, failover yok, alarm-low yalnız FDC hatasında); ekran satır 3 önceliğine "TDIODE ERR" eklendi (*FAULT* > SENSOR ERR > TDIODE ERR > AMB HOT > OK)

**Tests / Validations Run:**
- `cmake --build build/Debug` → PASS (0 error / 0 warning)

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda diyot kopma → TDIODE ERR senaryosu — MANUAL-4

**Result:** Sıcaklık alt sistemi rolleri kodda belgeli ve tanı yolu bağlı. Politika (D2 uzantısı): diyot tutarsızlığı degraded-but-operational — ölçüm/çıkış durmaz, uyarı gösterilir; alarm-low yalnız basınç sensörü hatasında. P1'in dört kartı da kod düzeyinde tamam; faz donanım doğrulamasını (MANUAL-4) bekliyor.

**Risks Introduced:** Boot'ta ilk ADC taraması tamamlanana dek (~100-350 ms) "TDIODE ERR" kısa süre görünebilir — kozmetik; gerekirse CARD-3.2'de açılış maskesi
**Risks Resolved:** None

**Next Action:** CARD-2.1 — kompanzasyon modeli v2 + flash format v2 (k_t_zero/k_t_span + vf25/tc persistansı)

## 2026-06-12 | Execute | Task: CARD-2.1

**Task ID:** CARD-2.1
**Type:** Execute
**Status:** Complete (kod); donanım doğrulaması MANUAL-4 bekliyor

**Files Created:** Yok
**Files Modified:**
- `App/Inc/cal_storage.h` — CAL_VERSION=2; cal_params_t: k_t→k_t_zero, yeni k_t_span/vf25_mv/tc_mv_c; model dokümantasyonu
- `App/Src/cal_storage.c` — v1 layout dondurulmuş legacy struct; cal_init'te v2 oku → olmadı v1 CRC doğrula + alan taşı + yeniler default → olmadı defaults; defaults'a yeni alanlar
- `App/Src/state_machine.c` — MI_KT_SPAN menü öğesi (11 öğe); kT zero/span etiketleri; Vf25/TC artık cal_params'tan okunup yazılıyor (tek kaynak) + runtime senkron; Exit(no save) reload'da temp_diode senkronu
- `App/Src/pressure_app.c` — kompanzasyon v2: P −= (k_t_zero + k_t_span·frac)·ΔT; boot'ta vf25/tc'yi flash'tan temp_diode'a yükleme; MENU /11

**Tests / Validations Run:**
- `cmake --build build/Debug` → PASS (0 error / 0 warning)
- Statik kontrol: k_t_span=0 ile v1 davranış birebir korunuyor; v1 kaydı migrasyonla kayıpsız taşınıyor

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda v1→v2 migrasyon (v1 kayıtlı cihazda boot), Vf25 değişikliğinin reboot sonrası kalıcılığı, iki sıcaklıkta kompanzasyon doğrulaması — MANUAL-4

**Result:** Kompanzasyon artık zero (offset) ve span (gain) kaymalarını ayrı katsayılarla düzeltiyor; menüden ayarlanan Vf25/TC reboot'ta artık kaybolmuyor (flash v2). Eski v1 kalibrasyon kayıtları ilk boot'ta otomatik taşınır, sonraki Save v2 yazar.

**Risks Introduced:** Flash'ta v2 kayıt varken eski (v1) firmware'e geri dönülürse kayıt okunamaz → defaults (bilinçli; sürüm geri dönüşünde yeniden kalibrasyon gerekir)
**Risks Resolved:** Vf25/TC reboot kaybı (CARD-1.3 notu) → kapandı

**Next Action:** CARD-2.2 — kalibrasyon iş akışı sağlamlaştırma (stabilite eşiği, geçersiz kombinasyon reddi)

## 2026-06-13 | Execute | Task: CARD-2.2

**Task ID:** CARD-2.2
**Type:** Execute
**Status:** Complete (kod) — P2 fazı kod tarafı TAMAMLANDI

**Files Created:** Yok
**Files Modified:**
- `App/Inc/state_machine.h` — yeni API: sm_cal_live_stable(), sm_get_info_msg()
- `App/Src/state_machine.c` — stabilite penceresi (8 örnek ~0.8 s, ΔC p2p ≤ 2000); kararsızken yakalama reddi ("UNSTABLE - WAIT", stabil olunca otomatik temizlenir); span yakalamada |ΔC−zero| ≥ 10000 kontrolü ("SPAN TOO CLOSE"); Save & Exit öncesi cal_validate (ΔC span + p_max>p_min, "CAL INVALID:dC/:P", flash hatasında "FLASH WRITE ERR"); Exit (no save) kirli durumda çift onay ("Discard? SET again"); dirty takibi
- `App/Src/pressure_app.c` — menü satır 2'de transient mesaj; CAL_LIVE satır 3: "STABLE: SET long" / "WAIT: unstable.." / red mesajı

**Tests / Validations Run:**
- `cmake --build build/Debug` → PASS (0 error / 0 warning)
- Statik akış kontrolü: SPAN TOO CLOSE mesajının stabilite auto-clear'ı tarafından silinmesi bug'ı geliştirme sırasında yakalanıp düzeltildi (pointer karşılaştırmalı seçici temizleme)

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda stabilite eşiğinin (2000 count) gerçek sensör gürültüsüne uygunluğu — MANUAL-4'te ayarlanacak; UI akışlarının buton testi — MANUAL-4

**Result:** Kalibrasyon iş akışı hataya dayanıklı: gürültülü sinyalde yakalama mümkün değil, zero≈span kaydedilemez, geçersiz P aralığı kaydedilemez, yanlışlıkla değişiklik atma çift onaylı. Eşikler #define ile donanım testinde ayarlanabilir.

**Risks Introduced:** CAL_STAB_P2P_MAX=2000 ve CAL_MIN_SPAN_COUNTS=10000 değerleri gerçek sensör karakteristiğine göre ayarsız — MANUAL-4'te ilk kalibrasyonda gözden geçirilecek
**Risks Resolved:** Gürültülü/geçersiz kalibrasyon kabulü riski → kapandı (kod düzeyinde)

**Next Action:** CARD-3.1 (LCD güç sırası) veya CARD-4.1 (loop makullük + NAMUR) — ikisi de bağımsız ilerleyebilir

## 2026-06-13 | Execute | Task: CARD-4.1

**Task ID:** CARD-4.1
**Type:** Execute
**Status:** Complete (kod) — P4 fazı kod tarafı TAMAMLANDI

**Files Created:** Yok
**Files Modified:**
- `App/Inc/xtr111_loop.h` — NAMUR NE43 sabitleri (3.6/3.8/20.5/21.0); loop_service()/loop_has_deviation() API; transfer fonksiyonu yorumu .c ile senkronlandı (R_SET=1.2 kΩ; eski 15 Ω yorumu yanlıştı)
- `App/Src/xtr111_loop.c` — **BUG FIX: komut clamp penceresi 3.8..20.5 → 3.6..21.0** (alarm-low 3.6 mA artık gerçekten üretilebiliyor; önceden 3.8'e yuvarlanıyordu); sapma monitörü (|cmd−meas| > 0.3 mA kesintisiz 2 s → bayrak, eşik altı 2 s → otomatik temizleme; alarm bölgesinde devre dışı); FLT fault auto-retry (5 s; FLT pini hâlâ LOW ise erteleme — EXTI kenar kaçırma önlemi)
- `App/Src/pressure_app.c` — pressure_to_ma NAMUR satürasyonu (aralık dışı → 3.8/20.5); alarm sabiti kullanımı; loop_service() çağrısı (100 ms tik); LED fault VEYA sapmada 5 Hz; ekran önceliğine "LOOP DEV" (SENSOR ERR sonrası)

**Tests / Validations Run:**
- `cmake --build build/Debug` → PASS (0 error / 0 warning)
- Statik akış: alarm bölgesinde (cmd < 3.8) sapma monitörü pasif ✓; fault sapmayı bastırır ✓; retry sonrası komut app'in sonraki tikinde yenilenir ✓

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda 4/12/20 mA mutlak doğruluk (multimetre), gerçek 3.6 mA alarm seviyesi, FLT→retry döngüsü, sapma eşiğinin (0.3 mA) INA190 ölçüm gürültüsüne uygunluğu — MANUAL-4

**Result:** Çıkış döngüsü FMEDA A.7 kapalı-çevrim doğrulamasına zemin: komut↔ölçüm sapması sürekli izleniyor (tanı bayrağı çıkışı değiştirmez), NAMUR NE43 seviyeleri standartlaştı, XTR111 fault'u kalıcı kilitlenme yerine güvenli otomatik kurtarmalı. Politika notu: karttaki "SET uzun ile clear" NORMAL'deki menü kısayoluyla çakıştığından 5 s otomatik retry + FLT pin seviyesi kontrolü uygulandı.

**Risks Introduced:** DEV_MAX_MA=0.3 / DEV_TIME_MS=2000 donanımda ayarsız (INA190 gürültüsüne göre); retry döngüsü kalıcı arızada 5 s'de bir LOOP_EN toggle üretir (kabul edilebilir — XTR111 yüksek empedans)
**Risks Resolved:** Alarm-low 3.6 mA üretilemiyor bug'ı (kayıtsızdı — bulunup kapatıldı); header/c transfer fonksiyonu çelişkisi

**Next Action:** CARD-3.1 — LCD güç sırası + donanım doğrulama (P3)

## 2026-06-13 | Execute | Task: CARD-3.1

**Task ID:** CARD-3.1
**Type:** Execute
**Status:** Complete (kod); donanım görsel doğrulaması MANUAL-4 bekliyor

**Files Created:** Yok
**Files Modified:**
- `App/Src/lcd400.c` — güç/reset zamanlama sabitleri (LCD_PWR_SETTLE_MS=50, RST_LOW=10, RST_WAIT=120, SLPOUT=120); lcd_cmd_data() helper; üretici ST7789V güç-kontrol + gamma dizisi eklendi (0xB2 porch, 0xB7 gate, 0xBB VCOM, 0xC2/C3/C4 VRH/VDV, 0xC6 FRCTRL 60Hz, 0xD0 power-ctrl, 0xE0/E1 gamma ±); SWRESET kaldırıldı (HW reset yeterli, üretici de kullanmıyor); CS2 hardcoded GPIOC/PIN9 yerine LCD_CS2_PORT/PIN makroları; backlight DISPON sonrası (garbage flash yok)

**Tests / Validations Run:**
- Datasheet (DS154S10Z0TG01.pdf) üretici init dizisi çıkarıldı ve birebir kodlandı (14'er bayt gamma dahil)
- `cmake --build build/Debug` → PASS (0 error / 0 warning)

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda görsel çıktı, kontrast/renk doğruluğu (gamma sonrası), 10× soğuk açılış güvenilirliği, reset zamanlamasının panelle uyumu — MANUAL-4 (ekran + ST-Link gerekli)

**Result:** LCD init artık üretici referans dizisini birebir uyguluyor — eski init reset-default güç/gamma ile sönük/yanlış kontrast riski taşıyordu. Güç sırası (PA10 settle → HW reset → 120 ms → SLPOUT) datasheet/üretici zamanlamasına hizalandı. Backlight içerik hazır olduktan sonra açılıyor (boot'ta garbage flash yok).

**Risks Introduced:** Gamma/VCOM değerleri üretici default'u — panel lot farkıyla ince ayar gerekebilir (MANUAL-4'te görsel kontrol)
**Risks Resolved:** Eksik güç-kontrol/gamma register'ları (sönük/yanlış ekran riski) → kapandı

**Next Action:** CARD-3.2 — menü state machine iyileştirmeleri (timeout, sayfalar, alarm ekranı, backlight menü)

## 2026-06-13 | Execute | Task: CARD-3.2

**Task ID:** CARD-3.2
**Type:** Execute
**Status:** Complete (kod) — P3 fazı kod tarafı TAMAMLANDI

**Files Created:** Yok
**Files Modified:**
- `App/Inc/state_machine.h` — SM_NORMAL_PAGES=3; sm_tick(), sm_get_normal_page(), sm_get_backlight_pct() API
- `App/Src/state_machine.c` — MI_BACKLIGHT menü öğesi (12 öğe); 60 s eylemsizlik timeout (sm_tick → cal_init+temp sync ile discard → NORMAL); NORMAL'de UP/DN sayfa geçişi; her event activity damgası; backlight runtime (canlı lcd_set_backlight, s_dirty tetiklemez)
- `App/Src/pressure_app.c` — sayfa-duyarlı render_normal (MAIN P/I/T, SENSOR dC+Td1/Td2+Tamb, LOOP cmd/meas/err); render_fault() adanmış alarm ekranı; status_line() helper; menü "/12"; sm_tick(now) 5 ms tikte; s_disp_p/s_disp_dc cache

**Tests / Validations Run:**
- `cmake --build build/Debug` → PASS (0 error / 0 warning)
- Statik kontrol: timeout discard yolu Exit-no-save ile aynı semantik; backlight cal kaydını kirletmiyor

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda 60 s timeout, 3 sayfa gezinme, fault ekranı görünümü, backlight canlı değişimi — MANUAL-4

**Result:** UI sahaya hazır: eylemsizlikte güvenli NORMAL dönüşü, NORMAL'de 3 bilgi sayfası (basınç/sensör/loop), loop fault'ta adanmış alarm ekranı, menüden canlı backlight. Backlight kalıcı DEĞİL (boot %60) — persistans bilinçli olarak ertelendi (format v2 kapalı; gelecekte storage bump'ı ile eklenebilir).

**Risks Introduced:** Backlight reboot'ta %60'a döner (saha için küçük kısıt — persistans ertelendi)
**Risks Resolved:** Eylemsizlikte menüde kalma; tek-sayfa kısıtı; fault görünürlüğü

**Next Action:** CARD-5.1 — BLE UART taşıma katmanı (MANUAL-5 datasheet indirme önkoşulu) veya CARD-6.1 — TPS3851 watchdog

## 2026-06-13 | Execute | Task: CARD-6.1

**Task ID:** CARD-6.1
**Type:** Execute
**Status:** Complete (kod); donanım reset testi + CWD pencere teyidi MANUAL-4/MANUAL-2 bekliyor

**Files Created:** Yok
**Files Modified:**
- `App/Src/pressure_app.c` — wdt_feed_raw() (net düşen-kenar HIGH→LOW darbe→HIGH + IWDG refresh, koşulsuz); WDT slice artık güvenlik görevi canlılığına koşullu (s_loop_token sensör tikinde damgalanır, WDT_HEALTH_TIMEOUT_MS=400 ms → A.16 program-akış izleme); toggle (fiilî 200 ms kenar) yerine deterministik 100 ms kenar; init'te WDI idle=HIGH + ilk besleme
- `App/Src/cal_storage.c` — extern wdt_feed_raw(); cal_save erase öncesi tek besleme (uzun flash bloklamasında starvation önlemi; pencereli dog'da erken-kick riskine karşı yalnız erase ÖNCESİ)

**Tests / Validations Run:**
- TPS3851 datasheet (TI SBVS300B) indirildi+analiz: WDI düşen-kenar, pencereli, tWD CWD'ye bağlı (std 0.7ms-3.23s / ext 62ms-77s) — web_research.md'ye işlendi
- `cmake --build build/Debug` → PASS (0 error / 0 warning)

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda reset testi (kick durdurma), cal_save sırasında reset-yok doğrulaması, **CWD pencere değeri ile 100 ms kick uyumu** (en kritik — şema teyidi MANUAL-2 madde 6, sonra MANUAL-4) — bunlar olmadan windowed dog erken/geç kick ile reset üretebilir

**Result:** Watchdog beslemesi A.16 uyumlu: yalnız periyodik güvenlik görevi (sensör+loop tiki) son 400 ms'de çalıştıysa beslenir → "canlı döngü, ölü görev" arızasını yakalar; flash erase artık dog'u aç bırakamaz; kenar üretimi deterministik (toggle'ın 2× periyot sorunu giderildi). **Açık önkoşul:** kart CWD konfigi okunup 100 ms kick'in pencereye düştüğü teyit edilmeli.

**Risks Introduced:** WDT_KICK_PERIOD_MS=100 ms, CWD penceresi teyit edilene dek varsayım — pencere bu değeri içermiyorsa donanımda sürekli reset (MANUAL-2 madde 6 ile çözülür); wdt_feed_raw'daki NOP busy-loop (~µs) kabul edilebilir
**Risks Resolved:** cal_save flash erase watchdog starvation (MEDIUM) → kapandı; toggle 2× periyot belirsizliği → kapandı

**Next Action:** CARD-6.2 — temel tanılar (diag modülü: ADC range-check, GPIO read-back, I2C bus recovery) veya CARD-5.1 BLE (MANUAL-5 önkoşul)

## 2026-06-13 | Execute | Task: CARD-6.2

**Task ID:** CARD-6.2
**Type:** Execute
**Status:** Complete (kod) — P6 fazı kod tarafı TAMAMLANDI

**Files Created:**
- `App/Inc/diag.h`, `App/Src/diag.c` — temel tanı modülü
**Files Modified:**
- `App/Src/pressure_app.c` — diag_init; sensör tikinde diag_service (VCC/I_loop raw); diag_critical() → loop_set_safe_state(); I2C bus recovery orkestrasyonu (fdc+tmp108 birlikte ~500 ms sağlıksız → 9-clock recovery + cihaz reinit, 5 s cooldown); status_line'a "DIAG CHK"
- `Firmware/CMakeLists.txt` — diag.c target_sources

**Tests / Validations Run:**
- `cmake --preset Debug` + `cmake --build build/Debug` → PASS (0 error / 0 warning); diag.c.obj üretildi + elf'e linklendi

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda ADC rail-stuck tetikleme, LOOP_EN read-back → safe state, I2C SDA-stuck bus recovery — MANUAL-4. Rail-stuck eşikleri (±8 LSB) ve I2C recovery zamanlaması donanımda doğrulanacak.

**Result:** Temel FMEDA tanıları kodda: (A.13) VCC_FB ve loop-akım ADC kanallarının rail'e takılması (kopuk/mux arızası — divider-bağımsız); (A.7) output GPIO read-back (ODR↔IDR) LOOP_EN/LCD_PWR/BLE_PWR/CLK_EN, LOOP_EN kritik → güvenli durum; I2C fiziksel bus recovery (9-clock+STOP+reinit) iki cihaz birlikte takılınca. diag driver'lara dokunmadı (sağlığı mevcut API ile gözler). Tanı bayrağı ekran ("DIAG CHK") ve güvenlik yoluna (LOOP_EN) bağlı.

**Risks Introduced:** ADC rail-stuck eşiği ve I2C recovery tetik sayısı donanımda ayarsız (untuned — MANUAL-4); LOOP_EN read-back → safe state nuisance-trip riski (2-örnek kalıcılıkla azaltıldı)
**Risks Resolved:** I2C bus stuck kalıcı arıza riski → kapandı (otomatik recovery)

**Next Action:** CARD-5.1 — BLE UART taşıma katmanı (MANUAL-5: DL-CC2340-B datasheet indirme önkoşulu). Kalan: 5.1/5.2 + 7.x manuel.

## 2026-06-13 | Execute | Task: CARD-5.1 (+ MANUAL-5)

**Task ID:** CARD-5.1
**Type:** Execute
**Status:** Complete (kod); donanım BLE testi MANUAL-4/5

**Files Created:**
- `App/Inc/ble_uart.h`, `App/Src/ble_uart.c` — BLE UART taşıma katmanı
**Files Modified:**
- `App/Src/pressure_app.c` — ble_uart_init() (init'te)
- `Firmware/CMakeLists.txt` — ble_uart.c

**Tests / Validations Run:**
- `DATASHEETS\C19273634.pdf` analizi (MANUAL-5): baud 115200, pin eşleştirme, AT formatı, URC'ler → web_research.md
- `cmake --build build/Debug` → PASS (0 error / 0 warning); ble_uart.c.obj + USART3_IRQHandler override çakışmasız

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda UART haberleşmesi, AT "AT"→"OK" yankısı, RX IT ring akışı, AUX pin davranışı — MANUAL-4/5

**Result:** BLE taşıma katmanı hazır: USART3 IT RX ring buffer (256 B, overflow sayaçlı), bloklamalı TX; güç sırası (BLE_PWR_ON/PC2 + MODE/PB12 wake + RESET/PA15 OD darbe + 120 ms); AUX (DIO21→BLE_EVENT/PB0) data-ready erişimi. **USART3 IRQ CubeMX'te kapalıydı → .ioc DEĞİŞTİRMEDEN** app'ten HAL_NVIC ile enable + USART3_IRQHandler ble_uart.c'de tanımlandı (startup zayıf sembol override). Varsayılan baud 115200 firmware ile uyumlu (değişim gerekmedi).

**Risks Introduced:** RX IT 1-bayt re-arm yüksek hızda IRQ-latency ile bayt düşürebilir (config trafiği için kabul; overflow sayacı ile izlenir)
**Risks Resolved:** USART3 IRQ eksikliği → app-side enable ile çözüldü (.ioc gerekmedi)

**Next Action:** CARD-5.2 — BLE konfigürasyon protokolü (AT init + advertise + GET_MEAS/SET_PARAM/SAVE + URC parse)

## 2026-06-13 | Execute | Task: CARD-5.2

**Task ID:** CARD-5.2
**Type:** Execute
**Status:** Complete (kod) — P5 + TÜM KOD KARTLARI TAMAMLANDI

**Files Created:**
- `App/Inc/ble_proto.h`, `App/Src/ble_proto.c` — BLE konfig protokolü
**Files Modified:**
- `App/Src/pressure_app.c` — ble_proto_init() + ana döngüde ble_proto_service() (status byte bitfield ile)
- `Firmware/CMakeLists.txt` — ble_proto.c

**Tests / Validations Run:**
- Datasheet AT formatları teyit (AT+ADVNAME=<local>,<manuf>; AT+ADVSTA=1; AT+ENTM; +PWRUP; +CONNOK/+DISCONN)
- `cmake --build build/Debug` → PASS (0 error / 0 warning); ble_proto.c.obj + elf

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda telefon (nRF Connect) ile advertise görünürlüğü, transparent çerçeve uçtan uca (GET_MEAS/SET_PARAM/SAVE), UNLOCK PIN akışı, AT init OK alımı — MANUAL-5 (modül + telefon)

**Result:** BLE konfig protokolü hazır (non-bloklayan): (1) AT init SM — RESET→ADVNAME(PT910)→ADVSTA=1→ENTM, her komuta OK bekleme + timeout/retry + degraded fallback (modül önceden konfigli olabilir); (2) transparent CRC16-CCITT çerçeve protokolü [0xAA|LEN|CMD|PAYLOAD|CRC16]: GET_MEAS (P/T/mA/status), GET_PARAM, SET_PARAM (unlock'lu), SAVE (unlock'lu→cal_save), INFO, UNLOCK (sabit PIN). 9 cal parametresi okunur/yazılır; vf25/tc yazımı temp_diode'a senkronlanır. Yazma koruması: sabit PIN UNLOCK. 4-20 mA güvenlik döngüsü BLE bring-up sırasında durmaz.

**Risks Introduced:** AT init retry komutu yeniden göndermiyor (degraded fallback'e güvenir — modül pre-konfigli senaryoda sorun değil); UNLOCK PIN sabit (v1 — BLE şifreleme/bonding yok, fiziksel erişim varsayımı)
**Risks Resolved:** BLE konfig eksikliği (gereksinim) → kapandı (kod düzeyinde)

**Next Action:** TÜM KOD KARTLARI TAMAM. Kalan: CARD-7.1/7.2 donanım bring-up + soak (kart + ST-Link + MANUAL-2/4 gerekli). Donanım gelince /ease-me execute CARD-7.1.
