# mcu_prog — Changelog

> Append-only. Yeni girişler en alta eklenir.

## 2026-06-22 | Readiness | Task: HW-READY-01

**Task ID:** HW-READY-01
**Type:** Donanım bring-up hazırlığı + MANUAL-2 kapanışı
**Status:** Complete (kod değişikliği YOK — yalnız doğrulama + doküman)

**Yapılanlar:**
- **Toolchain fiziksel doğrulaması:** STM32_Programmer_CLI v2.21.0 (çalışıyor), ST-LINK_gdbserver 7.13.0, arm-none-eabi-gdb + nm 15.2.1, ELF (2.2 MB, 661 sembol — cal_save/dbg_swo_init/ADC_CHANNEL_LUT mevcut), tools/ 4 script, ST-Link USB sürücü paketi (stsw-link009_v3) — hepsi hazır. Temiz rebuild: `ninja: no work to do` (ELF güncel).
- **MANUAL-2 KAPANDI** (tasarımcı/kullanıcı değerleri verdi):
  - #1 Diyot bias 3.3V/27kΩ → ~100µA (varsayım ~1mA idi); V_f25 600mV'den biraz düşük olur → kalibrasyonla ayarlanır, geçerlilik aralığı kapsıyor. Kod değişmez.
  - #2 FDC2214 ADDR=GND → 0x2A (firmware otomatik tespit). ✅
  - #3 TMP108 tarama korunuyor (0x48–0x4B). ✅
  - #4 XTR111 R_SET=1.2kΩ (R409), divider yok — firmware ile **TAM UYUMLU** (V_SET×8.333). ✅
  - #5 **BULGU:** X400=24MHz HSE + X401=32.768kHz LSE board'da var ama firmware iç MSIS RC0 + LSI kullanıyor (.ioc'da HSE/LSE seçili değil). Karar **bring-up'a ertelendi**: ilk flash MSIS ile; BLE 115200/timing sorunluysa CubeMX'te HSE+LSE'ye geç (manuel) + regenerate.
  - #6 TPS3851 CWD ~1600ms pencere; 100ms kick içeride. ✅ Programlamada jumper ile WDT disable → reset-loop riski yok.
  - #7 Boot/alarm (3.6/3.8/20.5/21.0 mA, boot loop-disabled) doğrulandı. ✅

**Files Changed (doküman):** mcu_prog_manual_steps.md, mcu_prog_state.md, mcu_prog_memory.md, mcu_prog_changelog.md, mcu_prog_tracker.html

**Validation:** Seviye 2 (build PASS, ELF güncel). Donanım doğrulaması (seviye 3) yarın ST-Link ile.

**NEXT:** Kullanıcı ST-Link + WDT-disable jumper takar → probe_test → flash → SWO/GDB bring-up (CARD-7.1).

## 2026-06-23 | Bring-up | Task: CARD-7.1 (kısmi — ADC DMA bug yakalandı)

**Task ID:** CARD-7.1 (donanım bring-up, devam ediyor)
**Type:** Donanım flash + canlı debug teşhisi
**Status:** BLOCKED → MANUAL-7 (CubeMX ADC DMA düzeltmesi) bekleniyor

**Yapılanlar:**
- **İlk flash:** probe OK (STM32U3xx, 1MB, ST-LINK V3SET, VDD 2.85V). İlk denemede verify mismatch (8MHz SWD); **freq=4000 (gerçek 3.3MHz) + mass erase ile başarılı** → verify PASS. SWD'yi düşük tutmak gerekiyor (jumper kablo/marjinal VDD).
- **HCLK = 48 MHz** doğrulandı (SystemCoreClock@0x20000000=0x02DC6C00; MSIS 96MHz÷2). SWO baud için bu değer.
- **BUG (kanıtlı, HOTPLUG canlı debug):** Firmware ~860 ms'de **ADC1_IRQHandler'da kilitlendi** (uwTick dondu). ADC1->ISR=0x101F **OVR=1**, IER OVRIE=1, CFGR CONT=1/**DMNGT=00**. Kök neden: adc.c `ConversionDataManagement=ADC_CONVERSIONDATA_DR` (continuous mode'da ADC veriyi DMA'ya vermiyor) + DMA DestInc=FIXED/Mode=NORMAL → overrun fırtınası, ADC1 IRQ pri(0,0) → SysTick açlığı. adc_buf=0 (DMA hiç çalışmadı).
- **Düzeltme MANUAL-7'ye yazıldı:** CubeMX ADC1 "Conversion Data Management Mode" = DMA Circular Mode + regenerate. Sonra Claude doğrular + reflash.
- ST-Link FW (V3J8M3B5S1) gdbserver 7.13 için eski → live GDB breakpoint için FW upgrade gerekebilir; ama HOTPLUG bellek/register okuması teşhis için yetti.

- **LCD teşhisi (canlı register):** Firmware tarafı TEMİZ — PA10 LCD_PWR_ON=HIGH, PA8 backlight TIM1_CH1 PWM çalışıyor (CEN=1, MOE=1, CC1E=1, ARR=99/CCR1=59 → %60), DISPON gönderildi, splash yazıldı. Ekran boşsa neden **fiziksel** (backlight devresi / SPI kablo / besleme). VDD=2.85V (3.3V için düşük) — besleme şüphesi. **Kullanıcı: "sorunu buldum, donanımsal halledeceğim" (2026-06-23).**

**Files Changed:** yok (kod değişikliği YOK — teşhis + doküman). ADC düzeltmesi CubeMX'te kullanıcıda; LCD/güç donanımda kullanıcıda.

**Validation:** Seviye 3 kısmi — flash+verify PASS, ama firmware boot'ta hang (ADC bug). Düzeltme sonrası tam bring-up.

**AÇIK İŞLER (firmware'in çalışması için):** (1) MANUAL-7 CubeMX ADC DMA Circular → reflash [Claude doğrular]. (2) LCD/güç donanım [kullanıcı].

## 2026-06-23 | Bring-up | Task: CARD-7.1 (ADC bug ÇÖZÜLDÜ — firmware canlı)

**Task ID:** CARD-7.1 (devam)
**Type:** Donanım flash + canlı debug
**Status:** ADC zinciri TAMAM; firmware tam superloop'ta. Sensör bağlantısı kullanıcıda (güç kesip bağlayacak).

**ADC bug fix süreci (3 CubeMX iterasyonu, hepsi canlı HOTPLUG ile doğrulandı):**
1. İlk regenerate: ConversionDataManagement DR→DMA_CIRCULAR + DestInc FIXED→INCREMENTED. Sonuç: OVR çözüldü AMA ContinuousConvMode=ENABLE yüzünden DMA TC kesme fırtınası (VECTACTIVE=45=GPDMA1_Ch0) → yine hang.
2. İkinci regenerate: ContinuousConvMode ENABLE→DISABLE + ConversionDataManagement→DMA_ONESHOT. Sonuç: storm bitti, uwTick akıyor, AMA DMA hâlâ circular kaldığı için ADC tek tarama yapıp dondu (adc_buf 5 örnekte birebir aynı, s_adc_done=0).
3. Üçüncü regenerate: DMA Settings CH0 Circular Mode ENABLE→DISABLE. Sonuç: **TAM ÇÖZÜLDÜ** — adc_buf her 100ms taze (diyot kanallarında jitter 1,2,1,2.. kanıtlı), VCC_FB=1930 stabil, uwTick akıyor, ICSR=0 thread mode.

**Nihai doğru ADC konfig:** ContinuousConvMode=DISABLE, ConversionDataManagement=ADC_CONVERSIONDATA_DMA_ONESHOT, DMA Mode=NORMAL (non-circular), SrcInc=FIXED, DestInc=INCREMENTED, software trigger, 4 kanal. Flash 68.7KB.

**I2C bus doğrulandı:** TMP108 s_addr7=0x48 (bulundu), FDC s_addr7=0x2A. I2C1 peripheral + hat çalışıyor.

**Açık (sensör bağlanınca netleşecek):** FDC s_errflag=1/s_disp_p=0 (sensörsüz → beklenen); loop s_fault=1 + VDD 2.85V düşük (kendi 24V beslemesi gerekli).

**Flash güvenilirlik notu:** Tüm flash'lar freq=4000 (gerçek ~3.3MHz) + mass erase ile yapıldı; 8MHz'de verify mismatch oluyordu.

**Validation:** Seviye 3 — flash+verify PASS + canlı çalışan firmware doğrulandı (ADC zinciri). Gerçek ölçüm doğrulaması sensör+güç sonrası.

**NEXT:** Kullanıcı sensörleri + 24V beslemeyi bağlar → "hazırım" → probe (VDD kontrol) + canlı FDC/diyot/loop/LCD izleme.

## 2026-06-23 | Bring-up | Task: CARD-7.1 (sensör + loop kalibrasyonu)

**Type:** Donanım bring-up — canlı debug + firmware düzeltmeleri
**Status:** FDC okuyor, loop enable + 4-20mA kalibre. Kalan: ERRB kalıcı fix, basınç kalibrasyonu, debug scaffolding temizliği.

**Yapılanlar (hepsi canlı doğrulandı):**
1. **LCD:** firmware tarafı temiz; ekran sorunu donanımdı (kullanıcı çözdü, splash görünüyor).
2. **FDC2214 okuma:** Donanım tarafında tank seri-kayıp/bağlantı düzeltildi (kullanıcı; L=6.8µH/C=135pF/f≈5.25MHz). Firmware: drive max (IDRIVE=31), SETTLECOUNT=0x400, SENSOR_ACTIVATE full-current. **BULGU: FDC2214'ün ERRB pini YOK** (sadece INTB+SD); firmware okumaları PA1 "FDC_ERRB" üzerinden geçitliyordu → hayalet bloklama. **Geçici bypass** (pressure_app `if(1)`) ile okuma açıldı → s_disp_dc canlı değişiyor. **KALICI FIX GEREKLİ:** PA1 gerçekte ne (FDC INTB hangi pinde?), gate'i STATUS/data-MSB hata bitlerine taşı.
3. **Loop enable (FLT glitch):** Fault kaynağı diag değil (s_loopen_bad=0), PA6 EXTI idi. **XTR111 güç-açılışında FLT'yi (PA6) anlık LOW yapıyor** → EXTI fault latch → PB2 LOW → retry → döngü. **FIX:** FLT settling mask (`FLT_SETTLE_MS=100`, `loop_on_flt_edge` + settling-sonrası seviye kontrolü; güvenlik korundu). Sonuç: PB2 HIGH stabil, s_fault=0.
4. **4-20mA çıkış kalibrasyonu:** Feedback (INA190) DOĞRU (multimetre=s_meas). Komut tarafı şaşıyordu. DAC doğru (kod→V_SET lineer). XTR111 zincirinde **~-0.84mA offset** var. 2-nokta uç-trim ölçümü: **kod 699→4.00mA, kod 3011→20.00mA** → `ma_to_dac_code`: **kod = 144.5*mA + 121** (LOOP_CAL_CODE_PER_MA/OFFSET). Doğrulama: 4/12/20 mA hepsi tam. Eski offset'siz V_SET modeli değişti.

**Files Changed:** App/Src/fdc2214.c (drive/settle/config), App/Src/pressure_app.c (ERRB bypass + loop_dbg_apply çağrısı), App/Src/xtr111_loop.c + Inc/xtr111_loop.h (FLT mask + cal + debug hook), Core/Src/main.c (EXTI→loop_on_flt_edge), Core/Src/adc.c + .ioc (one-shot DMA — daha önce).

**Geçici/bring-up scaffolding (üretimden önce temizlenecek):** pressure_app ERRB `if(1)` bypass; g_loop_dbg_dac/g_loop_dbg_ma + loop_dbg_apply; FDC drive/settle bring-up değerleri (tune edilebilir).

**Validation:** Seviye 4 (canlı donanım) — ADC, FDC okuma, loop enable, 4-20mA çıkış kalibrasyonu multimetre ile doğrulandı. Basınç kalibrasyonu + temp henüz YOK.

**NEXT:** (1) Basınç kalibrasyonu (FDC ham → bar, zero/span — bilinen basınç uygula). (2) ERRB kalıcı fix (PA1 gerçeği). (3) TEMP_MEAS_ON (diyot bias). (4) Debug scaffolding temizliği.

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

## 2026-06-13 | Execute | Task: DBG-SWO (canlı telemetri — kullanıcı isteği)

**Task ID:** DBG-SWO
**Type:** Execute (debug altyapısı)
**Status:** Complete (kod); SWO okuma donanımda (ST-Link SWV)

**Files Created:**
- `App/Inc/dbg_swo.h`, `App/Src/dbg_swo.c` — SWO/ITM telemetri modülü
**Files Modified:**
- `App/Src/pressure_app.c` — dbg_swo_init() + 1 Hz dbg_swo_telemetry()
- `Firmware/CMakeLists.txt` — dbg_swo.c
- `mcu_prog_flash_debug.md` — SWO okuma tarafı notu

**Tests / Validations Run:**
- `cmake --build build/Debug` → PASS (0 error / 0 warning); dbg_swo.c.obj + elf

**Validation Level Reached:** 2 — derleme/link

**What was NOT validated:** Donanımda SWV akışı (ST-Link + çekirdek saati gerekli) — bring-up'ta

**Result:** Non-intrusive canlı telemetri eklendi: ITM port 0'a 1 Hz "P=..,T=..,I=..,ST=0x.." satırı. CMSIS ITM_SendChar — SWV bağlı değilken no-op (sıfır maliyet, core halt yok, 4-20 döngüsü bozulmaz). PB3=SWO (AF0) zaten konfigli; firmware yalnız TRCENA + ITM yazımı yapar, trace baud'unu debugger ayarlar. Donanımda Claude SWV ile P/T/mA/status akışını canlı okuyabilecek.

**Risks Introduced:** None (no-op without debugger)
**Risks Resolved:** Canlı telemetri kanalı eksikliği → kapandı (firmware tarafı)

**Next Action:** Donanım bekleniyor → CARD-7.1 bring-up (ST-Link takılı + MANUAL-2 teyitleri gerekli)

## 2026-06-25 | Finding | Task: DISP-FLOAT-DIAG (tartışma — kod değişikliği YOK)

**Task ID:** DISP-FLOAT-DIAG
**Type:** Tanı / bulgu kaydı (kullanıcı: "yarın direkt uygulayalım")
**Status:** Kök neden bulundu — uygulama CARD-3.3 olarak backlog'a yazıldı, BUGÜN kod değişmedi.

**Belirti (kullanıcı):** Ekranda "mA", "Pa/bar", "C" etiketleri görünüyor ama **sayısal değerler boş**. O sırada sensöre bir şey bağlı değil (bağlamak gerekmiyor — sorun ondan değil).

**Kök neden (kesin):**
- Proje `--specs=nano.specs` (newlib-nano) ile linkleniyor: `cmake/gcc-arm-none-eabi.cmake:40`, `cmake/starm-clang.cmake:53`.
- nano-printf **varsayılan olarak `%f` (float) dönüşümünü desteklemez**; açmak için linker'a `-u _printf_float` gerekir — **tüm projede bu bayrak YOK** (`grep _printf_float` → bulunamadı).
- `pressure_app.c:159-176, 197, 207` değerleri `%7.3f / %5.2f / %5.1f / %+5.2f / %.3f` ile basıyor → float dönüşümü sessizce boş çıkıyor; string sabitleri ("mA", "bar", "C") kalıyor. Senin gördüğün tam bu.

**Sensör tartışması (ayrı, bağlamsal):** Bağlı sensör Yantai metal-kapasitif **7E = 0~345~2068 kPa** (LRL~min span~URL; 345 "discrete sapma" DEĞİL, minimum kalibre edilebilir span; turndown ≈6:1). Boştayken okunan **-0.68 kPa**, ±0.1% URL = ±2.07 kPa hata bandının ~1/3'ü → tamamen makul sıfır offset'i. Donanım/okuma sağlam.

**Karar (yarın uygulanacak):** **B yolu** — `pressure_app.c` içindeki `%f` çağrılarını integer-ölçekli formatlamaya çevir (nano'yu bozma, binary büyütme). A yolu (`-u _printf_float`) REDDEDİLDİ: `cmake/**` FORBIDDEN + ~2-4 KB büyüme. Detay: **CARD-3.3** (backlog).

**Files Changed (doküman):** mcu_prog_backlog.md (CARD-3.3 eklendi), mcu_prog_changelog.md.

**Validation:** Seviye 0 (yalnız analiz/okuma — derleme yapılmadı, kod değişmedi).

**NEXT PROMPT TO CLAUDE:** `/ease-me execute CARD-3.3` — pressure_app.c float→integer formatlama; build + map'te float-printf linklenmediğini teyit; donanımda ekran görsel doğrulama (MANUAL).

## 2026-06-25 | Execute | Task: CARD-3.3 (ekranda metrik float değerleri görünmüyor)

**Task ID:** CARD-3.3
**Type:** Execute (firmware düzeltmesi)
**Status:** Complete (kod + build); donanım görsel doğrulaması MANUAL (kullanıcı ST-Link bağlayacak)

**Belirti:** NORMAL/sensör/loop/kalibrasyon ekranlarında etiketler (mA/bar/C) görünüyor, **sayısal değerler boş**. Kök neden DISP-FLOAT-DIAG'da tanılı: nano-printf `%f` desteklemiyor (`-u _printf_float` yok).

**Yapılanlar (B yolu — integer formatlama):**
- `pressure_app.c`'ye statik `fmt_fixed(buf, n, v, decimals, force_sign)` helper'ı eklendi: float'ı ölçek×yuvarlama ile tamsayıya çevirip `%s%ld.%0*ld` (integer yolu) ile basıyor; negatif/işaret ve ondalık sıfır-dolgu doğru ele alınıyor. Hizalama dış snprintf'te `%Ns` ile korundu.
- 10 `%f`/`%+f` çağrısı dönüştürüldü: MAIN (P 3-ondalık / I 2 / T 1), SENSOR (Td1/Td2/Tamb 1-ondalık — iki-değerli satırda iki ayrı buffer), LOOP (cmd/meas 2, err 2 + force_sign), render_edit (Value 3), render_cal_live (P 2). `dC=%ld` / `MENU %d` integer formatları değişmedi.
- `case` blokları yerel buffer için `{ }` ile kapsandı (C89 declaration-after-label).

**Files Changed:** `Firmware/App/Src/pressure_app.c` (1 dosya, 0 yeni — diff budget içinde).

**Tests / Validations Run:**
- `cmake --build build/Debug` → **PASS** (0 error / 0 warning). FLASH 69224 B, RAM 12392 B.
- **.map float-link teyidi:** linklenen tek vfprintf yolu `nano-svfprintf.o` + `nano-vfprintf_i.o` (**integer** formatlayıcı). Float formatlayıcı obje (`vfprintf_float`/`_printf_float`/`dtoa`/`mprec`) **linklenmedi** → B yolu doğrulandı, binary float-printf ile şişmedi.

**Validation Level Reached:** **4 (donanım)** — build PASS + .map float-printf linklenmedi + **ST-Link flash/verify PASS + ekranda sayısal değerler görünüyor (kullanıcı görsel teyidi 2026-06-25)**.

**Flash:** STM32U3xx, V3SET, VDD 2.82V, SWD 3.3 MHz; mass erase + 67.60 KB download + verify PASS + reset. Kullanıcı: "değerler görünüyor" → CARD-3.3 KAPANDI.

**Risks Introduced:** None (yalnız biçimlendirme; değer matematiği değişmedi).
**Risks Resolved:** "Ekranda metrik değerleri boş" → kod tarafı kapandı (donanım teyidi bekliyor).

**NEXT PROMPT TO CLAUDE:** Kullanıcı ST-Link'i bağlayınca → `flash.bat` (freq=4000 + mass erase) → NORMAL/sensör/loop ekranlarında sayısal değerlerin doğru ondalıkla göründüğünü görsel teyit (bilinen 12 mA ile karşılaştır). Sonra CARD-7.1 kalan işler: basınç kalibrasyonu, ERRB kalıcı fix, debug scaffolding temizliği.

## 2026-06-25 | Bring-up | Task: CARD-7.1 (TEMP_MEAS_ON diyot bias enable + basınç teşhisi)

**Type:** Donanım bring-up — canlı debug + firmware değişikliği
**Status:** TEMP_MEAS_ON enable eklendi (flash'lı, register seviyesinde doğrulandı). Diyot HW açık devre (kullanıcıda). Basınç offset teşhisi yapıldı → zero-cal kullanıcıda (menüden).

**1) TEMP_MEAS_ON (PB4) diyot bias enable:**
- **BULGU:** Diyot kanalları (PC0/PC1) bias yokken ~0 V (5/7 count) okuyordu → sıcaklık 25 °C'de donmuş, ekranda Td1/Td2 çöp (~323 °C). Pin haritasında diyot bias enable yoktu.
- **TASARIMCI TEYİDİ:** PB4 (pin haritasında "TEST_MODE" etiketli, opsiyonlu) gerçekte **TEMP_MEAS_ON** = 1N4148 diyot bias enable. TEST_MODE kullanılmayacak (kullanıcı Excel'i düzeltiyor). TEST_MODE firmware'de hiç okunmuyordu → pin güvenle yeniden kullanıldı.
- **FIX:** `bsp_pins.h`'a TEMP_MEAS_ON alias (PB4); `pressure_app_init` başında PB4 input-pulldown→**Output PP + HIGH** (CubeMX'i değiştirmeden, app tarafında). Kalıcı çözüm: CubeMX'te PB4→Output PP (MANUAL).
- **Canlı doğrulama:** flash sonrası GPIOB MODER pin4=01 (output)✓, ODR bit4=1 (HIGH)✓. AMA PC0/PC1 **tam rail'e (4094/4095 = 3.3 V)** çıktı, beklenen ~0.5 V diyot düşümü DEĞİL → Vf=3299 mV (>1000 mV max) → hâlâ invalid. **Yorum: diyot ileri yolu AÇIK DEVRE** (sensörün sıcaklık-diyot hattı bağlı değil). Firmware doğru; donanım bağlantısı kullanıcıda. Diyot şimdilik ertelendi.

**2) Basınç okuma teşhisi (sensör bağlı, iki taraf boşta):**
- **Kablaj (kullanıcı):** IN2A=low→CH2, IN3A=high→CH3, sensör G/E→GND, IN2B/IN3B floating. = tek-uçlu diferansiyel basınç ölçümü. `fdc2214_read_delta`: dC = raw_CH3(high) − raw_CH2(low). **Topoloji doğru.**
- **Cal = tamamen default** (kalibrasyon yapılmamış): cap_at_zero=0, cap_at_span=1.000.000, p_min=0, p_max=10 bar, damping=0.5s → **p = dC/100000 bar**.
- **Canlı dC ölçümü:** ~47.800–53.000 count arası (oturum boyunca ~5k drift ≈ 0.05 bar; tur-içi gürültü ±1.200 count ≈ ±0.012 bar). Filtreli p ~0.50 bar STABİL görünüyordu.
- **KÖK NEDEN:** ~0.5 bar "yanlış" okuma = **sıfır offset** (iki LC tankı fiziksel farklı → eşit basınçta bile ham count'lar ~48k farklı). Gürültü/arıza DEĞİL. Kullanıcının gördüğü 0.4–0.9 = soğuk açılış/ısınma geçişi + offset.
- **Canlı kanıt:** cap_at_zero=güncel dC yazıldı (RAM) → basınç 0.29→0.20→0.13→0.07→0.03→0.016 bar (IIR yakınsadı) → **zero-cal offset'i siliyor doğrulandı.** Sonra reset (RAM temizlendi).

**Files Changed:** `App/Inc/bsp_pins.h` (TEMP_MEAS_ON alias), `App/Src/pressure_app.c` (PB4 output HIGH init).

**Validation:** Seviye 4 (canlı). Build PASS (FLASH 69280 B), flash/verify PASS, register + ADC + dC canlı okundu.

**NEXT:** Kullanıcı devreyi stabilize edip (güç+ST-Link çıkarıp tekrar) **menüden zero-cal** yapacak (Cal Zero→SET uzun→Save&Exit). Stabilite kapısı (p2p≤2000) takılırsa FDC RCOUNT averaging artırılır. Sonra bilinen basınçla span. Diyot bias: sensör sıcaklık-diyot hattı bağlanınca PC0/PC1 ~0.6 V vermeli (HW kullanıcıda).

## 2026-06-25 | Bring-up | Task: CARD-7.1 (kalibrasyon stabilite eşiği + drive denemesi)

**Type:** Donanım bring-up — canlı debug + firmware tuning
**Status:** Stabilite eşiği gevşetildi (flash'lı). Drive düşürme denendi→geri alındı. Kullanıcı menüden zero-cal yapacak.

**Belirti:** Menüden zero-cal'da "WAIT: unstable.." çok sık → kullanıcı yakalayamıyor.

**Teşhis (canlı):**
- Stabilite kapısı: 8-örnek pencere, p2p ≤ CAL_STAB_P2P_MAX=2000 (state_machine.c). RCOUNT zaten 0xFFFF max (averaging artırılamaz).
- Canlı dC: örnek-başı gürültü ±1.500–2.300 count → 8-örnek p2p sık sık 2000'i aşıyor. AYRICA **büyük warm-up drifti**: dC 50k→66k→87k (+0.37 bar, reset'ten beri tırmanıyor).
- Drift hipotezi: FDC drive MAX (IDRIVE=31) → LC self-heating → diferansiyel drift.

**Deneme + sonuç:**
- **IDRIVE 31→20 (0xA000) denendi:** dC ±8.000–22.000 ÇÖP oldu (osilasyon zayıfladı; errflag temiz ama okuma kullanılamaz). → drive düşürmek bu sensörde yanlış; **IDRIVE=31 (0xF800) geri yüklendi** (changelog notu doğruymuş: max drive temiz osilasyon için gerekli). Drift self-heating değil, warm-up.
- **CAL_STAB_P2P_MAX 2000→6000** (state_machine.c) — gürültü ±2000 olduğundan 2000 eşiği fazla sıkıydı. Doğrulama: IDRIVE=31'de 12-örnek p2p=2109 → 6000 eşiğini rahat geçer.

**Files Changed:** `App/Src/state_machine.c` (CAL_STAB_P2P_MAX 2000→6000), `App/Src/fdc2214.c` (DRIVE: net değişiklik yok, IDRIVE=31; yorum güncellendi—IDRIVE=20 denemesi belgelendi).

**Validation:** Seviye 4 (canlı). Build PASS (FLASH 69284 B), flash/verify PASS, gürültü/p2p canlı doğrulandı.

**NEXT:** Kullanıcı menüden zero-cal (artık STABLE gelir). Cihaz ısınıp dC tırmanışı durduktan sonra kalibre etmesi önerildi (drift oturması için). Sonra bilinen basınçla span. AÇIK: warm-up drifti (~0.37 bar) — sıcaklık kompanzasyonu (diyot) devreye girince düzelmesi beklenir; diyot HW bağlantısı kullanıcıda.

## 2026-06-25 | Bring-up | Task: CARD-7.1 (cal_save flash write hatası — kalibrasyon kalıcılığı)

**Type:** Donanım bring-up — bug fix (flash persistans)
**Status:** ÇÖZÜLDÜ (kullanıcı testi: "sorun yok"). Backlight de çalışıyor (kod değişikliği gerekmedi — yanlış alarm).

**Belirti:** Menüde "Save & Exit" → "FLASH WRITE ERR"; güç çevrimde kalibrasyon kayboluyor gibi.

**Teşhis (canlı debug):**
- Flash bölgesi 0x080FE000 incelendi: veri ASLINDA yazılıyordu (magic/version/crc/alanlar tam) ve reset sonrası cal_init doğru yüklüyordu → "kayboluyor" yanılgıydı; asıl sorun cal_save'in false dönüp hata mesajı göstermesi.
- pFlash.ErrorCode = **0x88 = PGSERR (bit7) + PROGERR (bit3)**. Kök neden: **STM32U3 flash ECC 128-bit (quad-word) granülaritede**; kayıt 56 bayttı (7 doubleword) → son doubleword 128-bit kelimenin yarısını boş bırakıyordu → PGSERR (veri yazılsa da HAL hata).
- ICACHE kapalı (CR=0x4, EN=0) → stale-cache değil. VDD=2.84V (düşük) → 16-bayt fix sonrası kalan aralıklı hatanın muhtemel sebebi.

**Fix (cal_storage.c, cal_save):**
1. Tampon **16 baytın katına** yuvarlandı (her 128-bit ECC kelimesi tam yazılır → sistematik PGSERR gitti).
2. Başarı ölçütü **geri okuma doğrulaması** (memcmp flash↔buf) — HAL bayrağına değil fiilî içeriğe güven.
3. Başta **__HAL_FLASH_CLEAR_FLAG** (stale bayraklar).
4. Erase+program+doğrula **3 kez retry** (marjinal VDD kaynaklı kalan aralıklı hatayı yutar).

**Files Changed:** `App/Src/cal_storage.c`.

**Validation:** Seviye 4 (canlı). Build PASS (FLASH 69324 B), flash/verify PASS. Kullanıcı menüden defalarca Save & Exit → hata yok, güç çevrimde ayar korunuyor. Backlight menüden çalışıyor (teyit).

**AÇIK:** VDD 2.84V düşük (kullanıcının 3.3V/24V beslemesiyle flash zaten ilk denemede güvenilir olacak; retry bring-up emniyeti). Kalan CARD-7.1: bilinen basınçla span-cal, ERRB kalıcı fix, diyot HW (PC0/PC1 açık devre), debug scaffolding temizliği.
