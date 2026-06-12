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
