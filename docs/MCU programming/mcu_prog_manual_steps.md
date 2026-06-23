# mcu_prog — Manual Steps

> Claude'un yapamadığı, kullanıcı/donanım gerektiren adımlar kuyruğu.

## MANUAL-1 — Donanım test düzeneği hazırlığı
**Status:** [ ] Waiting
**Blocking:** NO (kod kartları için), YES (donanım doğrulama kartları için)
**Tool:** ST-Link + kart + 24 V kaynak + 250 Ω yük + multimetre
**Added by:** BOOTSTRAP-01
**Instructions:**
1. **ST-Link'i board'a (SWD: SWDIO/PA13, SWCLK/PA14, GND, 3V3) ve bilgisayara (USB) tak** — flash + debug bunun üzerinden. Claude'a "ST-Link takılı" de; Claude `tools\probe_test.bat` ile teyit eder, sonra `flash.bat` ile yükler.
2. Kartı 24 V'a bağla, loop konektörüne **yavaş** yükselt (INTEGRATION.md uyarısı)
3. 250 Ω yük + multimetre (mA modu) seri bağla
**What Claude needs back:** "ST-Link takılı" + "düzenek hazır" onayı (flash/debug Claude'da — bkz. mcu_prog_flash_debug.md)

## MANUAL-2 — Şema teyitleri (4-20.PDF — Claude PDF görüntüsünü render edemiyor)
**Status:** [x] Done (2026-06-22 — kullanıcı/tasarımcı tüm değerleri verdi; tek açık nokta saat kaynağı kararı bring-up'a ertelendi)
**Blocking:** YES → CARD-1.3 (termistör), kısmen CARD-1.1/1.2
**Tool:** PDF görüntüleyici / Altium
**Added by:** BOOTSTRAP-01
**Instructions:** Şemadan şunları teyit et:
1. PC0/PC1'e bağlı 1N4148 diyotların bias devresi: seri/bias direnci değeri ve besleme (V_f25 varsayılanını belirler) — diyotlar sensör modülü içindeyse konektör pinleri
2. FDC2214 ADDR pini → GND mi VDD mi (I2C adres 0x2A / 0x2B)
3. TMP108 ADD0 bağlantısı (I2C adresi)
4. XTR111 R_SET direnci değeri (transfer fonksiyonu doğrulama)
5. X400/X401 kristal designator'ları: hangisi 24 MHz HSE, hangisi 32.768 kHz LSE (BOM ↔ pin haritası çelişkisi)
6. **TPS3851 (U701) CWD pini** (CARD-6.1): kapasitör değeri / VDD pullup direnci / NC — ve standart mı extended timing mi. Bu, watchdog penceresini (tWD_MIN..tWD_MAX) belirler. Firmware kick periyodu (100 ms) bu pencerenin **içinde** olmalı; aksi halde (özellikle çok hızlı kick) pencereli watchdog reset üretir.

**Kullanıcı yanıtları (2026-06-22) — tasarımcı teyitli:**
1. **Diyot bias:** 3.3 V besleme / **27 kΩ** seri direnç → bias akımı ≈ (3.3−Vf)/27k ≈ **~100 µA** (varsayım ~1 mA idi). Diğer varsayımlar korunuyor (V_f25, TC≈−2 mV/°C). NOT: 100 µA'da gerçek V_f25, 600 mV varsayımından **biraz düşük** olacak → kalibrasyonla ayarlanır; temp_diode geçerlilik aralığı (200–1000 mV) kapsıyor. **Kod değişikliği yok.**
2. **FDC2214 ADDR = GND → adres 0x2A.** Firmware zaten otomatik tespit ediyor. ✅
3. **TMP108:** tarama korunuyor (0x48–0x4B). ✅
4. **XTR111 R_SET = 1.2 kΩ (R409), divider yok.** Firmware ile **TAM UYUMLU** (`I[mA]=10·V_SET/R_SET=V_SET×8.333`; DAC1_OUT1 12-bit/3.3V → 4mA@0.48V, 20mA@2.40V). ✅ Kod değişikliği yok.
5. **Kristaller: X400 = 24 MHz HSE, X401 = 32.768 kHz LSE — ikisi de board'da VAR.** ⚠️ ANCAK firmware (`main.c SystemClock_Config`) **iç MSIS RC0 + iç LSI** kullanıyor; HSE/LSE seçili değil. **Karar (kullanıcı 2026-06-22): bring-up'a ertelendi** — ilk flash MSIS ile; BLE 115200 / timing'de sorun görülürse HSE+LSE'ye geçilir (CubeMX manuel `.ioc` adımı + main.c regenerate). HCLK değeri SWO baud için bring-up'ta GDB ile canlı okunacak (`SystemCoreClock`).
6. **TPS3851 CWD: ~1600 ms pencere.** Firmware kick 100 ms ≪ 1600 ms → pencere içinde, sorun yok. ✅ Üretimde jumper takılı (WDT aktif); programlama sırasında jumper ile WDT **disable** edilecek → ilk flash'ta reset-loop riski YOK.
7. **Boot/alarm seviyeleri doğrulandı:** boot loop disabled (PB2=LOW), fault→3.6 mA, NAMUR 3.6/3.8/20.5/21.0 mA. ✅

**What Claude needs back:** ~~6 maddenin değerleri~~ ALINDI. Açık tek nokta: saat kaynağı (HSE) kararı — bring-up'ta verilecek.

## MANUAL-3 — CubeMX .ioc değişiklikleri (gerektiğinde)
**Status:** [x] Done (2026-06-12 — kullanıcı regenerate etti: 4 kanal IN1/IN2/IN11/IN12, rank kanal bug'ı düzeldi; Claude diff kontrolü PASS, USER CODE korunmuş)
**Blocking:** YES → CARD-1.3
**Tool:** STM32CubeMX
**Added by:** BOOTSTRAP-01
**Instructions (CARD-1.3 zamanı — tek regenerate'te hepsi):**
1. `Firmware/PressureTransmitter.ioc` aç → ADC1 Regular Conversion'da:
   - **BUG DÜZELTMESİ (CARD-0.2'de tespit):** Rank 2 ve Rank 3'e kanal ATANMAMIŞ — `adc.c` üç rank'ta da CHANNEL_1 (PC0) örnekliyor. Rank 2 = **IN13 (PC4 VCC_FB)**, Rank 3 = **IN14 (PC5 Current_OUT_FB)** olarak ata
   - PC1 → **IN2 (TMP_ADC2, ikinci 1N4148)** yeni rank ekle; hedef sıra: PC0 → PC1 → PC4 → PC5 (Number of Conversions = 4)
2. ~~GPIO kontrolleri~~ TAMAM (CARD-0.2 teyidi): PD2/PB6 Output, PB7/PA1/PB5 EXTI zaten .ioc'ta tanımlı
3. Generate Code → Claude'a haber ver (regenerate sonrası USER CODE + App diff kontrolü Claude yapar; ADC_RANK_* sabitleri CARD-1.3'te güncellenir)
**What Claude needs back:** "Regenerate tamam" + varsa CubeMX uyarıları

## MANUAL-7 — CubeMX ADC DMA düzeltmesi (BRING-UP BLOCKER, 2026-06-23)
**Status:** [x] Done (2026-06-23 — 3 iterasyon; nihai: Continuous=DISABLE + ConversionDataManagement=DMA_ONESHOT + DMA Settings CH0 Circular Mode=DISABLE → DMA Mode=NORMAL + DestInc=INCREMENTED. Canlı doğrulandı: adc_buf her 100ms taze, jitter kanıtlı, firmware superloop'ta)
**Blocking:** YES → firmware boot'ta donuyor; düzeltilmeden hiçbir donanım testi yapılamaz
**Tool:** STM32CubeMX
**Added by:** CARD-7.1 bring-up (canlı debug ile teşhis)
**Bulgu (kanıtlı):** İlk flash sonrası firmware ~860 ms'de **ADC1_IRQHandler içinde** kilitlendi (uwTick dondu, HOTPLUG ile teşhis). Canlı register: ADC1->ISR=0x101F (**OVR=1**), IER=0x10 (OVRIE=1), CFGR=0x80002000 (CONT=1, **DMNGT=00**). Kök neden: `MX_ADC1_Init` (adc.c:52,59) `ContinuousConvMode=ENABLE` + `ConversionDataManagement=ADC_CONVERSIONDATA_DR` → ADC veriyi DMA'ya vermiyor → overrun fırtınası, ADC1 IRQ önceliği (0,0)=en yüksek → SysTick açlığı → tüm firmware ölü. DMA ayrıca `DestInc=FIXED`+`Mode=NORMAL` (yanlış).
**Instructions:**
1. `Firmware/PressureTransmitter.ioc` → Pinout&Configuration → Analog → ADC1 → Parameter Settings
2. **"Conversion Data Management Mode"** = `ADC_CONVERSIONDATA_DR` → **"DMA Circular Mode"**
3. DMA Settings sekmesi: GPDMA1 ADC1 request **Mode = Circular** (memory increment otomatik açılır)
4. Generate Code → Claude'a haber ver
**What Claude needs back:** "regenerate tamam" → Claude adc.c'yi doğrular (DMA_CIRCULAR + DINC_INCREMENTED), rebuild + reflash + canlı teyit (uwTick artışı + adc_buf dolumu)

## MANUAL-4 — Donanım bring-up testleri (kart kartlarında tekrar kullanılır)
**Status:** [ ] Waiting
**Blocking:** YES → CARD-7.1/7.2 ve tüm "MANUAL" doğrulamalı done-criteria'lar
**Tool:** Donanım + multimetre + basınç referansı
**Added by:** BOOTSTRAP-01
**Instructions:** İlgili kartın "Manual validation" bölümündeki senaryoyu koş (örn. INTEGRATION.md §7)
**What Claude needs back:** Ölçüm değerleri / PASS-FAIL

## MANUAL-5 — DL-CC2340-B datasheet'ini indir
**Status:** [x] Done (2026-06-13 — kullanıcı indirdi: `DATASHEETS\C19273634.pdf`; baud 115200, pinler ve AT formatı çıkarıldı)
**Blocking:** YES → CARD-5.1
**Tool:** Browser
**Added by:** BOOTSTRAP-01
**Instructions:**
1. https://www.lcsc.com/datasheet/lcsc_datasheet_2312012005_DreamLNK-DL-CC2340-B_C19273634.pdf indir
2. `C:\PressureTransmitter\DATASHEETS\` içine kaydet
(Alternatif: Claude WebFetch ile deneyebilir — CARD-5.1 başında)
**What Claude needs back:** Dosya adı/yolu

## MANUAL-6 — git init onayı
**Status:** [x] Done (2026-06-12 — onaylandı; main branch, ilk commit e2120fe)
**Blocking:** YES → CARD-0.3 (ve dolaylı olarak güvenli rollback)
**Tool:** Karar
**Added by:** BOOTSTRAP-01
**Instructions:** Repo'nun git'e alınmasını onayla/reddet (commit politikası: yalnız yerel, push yok)
**What Claude needs back:** Evet/Hayır
