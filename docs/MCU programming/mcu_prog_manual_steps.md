# mcu_prog — Manual Steps

> Claude'un yapamadığı, kullanıcı/donanım gerektiren adımlar kuyruğu.

## MANUAL-1 — Donanım test düzeneği hazırlığı
**Status:** [ ] Waiting
**Blocking:** NO (kod kartları için), YES (donanım doğrulama kartları için)
**Tool:** ST-Link + kart + 24 V kaynak + 250 Ω yük + multimetre
**Added by:** BOOTSTRAP-01
**Instructions:**
1. Kartı ST-Link ile bağla, 24 V'u loop konektörüne **yavaş** yükselt (INTEGRATION.md uyarısı)
2. 250 Ω yük + multimetre (mA modu) seri bağla
**What Claude needs back:** "Düzenek hazır" onayı

## MANUAL-2 — Şema teyitleri (4-20.PDF — Claude PDF görüntüsünü render edemiyor)
**Status:** [ ] Waiting
**Blocking:** YES → CARD-1.3 (termistör), kısmen CARD-1.1/1.2
**Tool:** PDF görüntüleyici / Altium
**Added by:** BOOTSTRAP-01
**Instructions:** Şemadan şunları teyit et:
1. PC0/PC1'e bağlı 1N4148 diyotların bias devresi: seri/bias direnci değeri ve besleme (V_f25 varsayılanını belirler) — diyotlar sensör modülü içindeyse konektör pinleri
2. FDC2214 ADDR pini → GND mi VDD mi (I2C adres 0x2A / 0x2B)
3. TMP108 ADD0 bağlantısı (I2C adresi)
4. XTR111 R_SET direnci değeri (transfer fonksiyonu doğrulama)
5. X400/X401 kristal designator'ları: hangisi 24 MHz HSE, hangisi 32.768 kHz LSE (BOM ↔ pin haritası çelişkisi)
**What Claude needs back:** 5 maddenin değerleri (metin olarak yeterli)

## MANUAL-3 — CubeMX .ioc değişiklikleri (gerektiğinde)
**Status:** [ ] Waiting (CARD-1.3 sırasında aktifleşir)
**Blocking:** YES → CARD-1.3
**Tool:** STM32CubeMX
**Added by:** BOOTSTRAP-01
**Instructions (CARD-1.3 zamanı):**
1. `Firmware/PressureTransmitter.ioc` aç → ADC1'e IN2 (PC1) ekle, tarama sırası PC0→PC1→PC4→PC5
2. PD2 (CLK_EN), PB6 (SD) GPIO Output; PB7 (INT_B), PA1 (ERRB), PB5 (FLT_TEMP#) GPIO Input/EXTI tanımlı mı kontrol et — değilse ekle
3. Generate Code → Claude'a haber ver (regenerate sonrası USER CODE diff kontrolü Claude yapar)
**What Claude needs back:** "Regenerate tamam" + varsa CubeMX uyarıları

## MANUAL-4 — Donanım bring-up testleri (kart kartlarında tekrar kullanılır)
**Status:** [ ] Waiting
**Blocking:** YES → CARD-7.1/7.2 ve tüm "MANUAL" doğrulamalı done-criteria'lar
**Tool:** Donanım + multimetre + basınç referansı
**Added by:** BOOTSTRAP-01
**Instructions:** İlgili kartın "Manual validation" bölümündeki senaryoyu koş (örn. INTEGRATION.md §7)
**What Claude needs back:** Ölçüm değerleri / PASS-FAIL

## MANUAL-5 — DL-CC2340-B datasheet'ini indir
**Status:** [ ] Waiting
**Blocking:** YES → CARD-5.1
**Tool:** Browser
**Added by:** BOOTSTRAP-01
**Instructions:**
1. https://www.lcsc.com/datasheet/lcsc_datasheet_2312012005_DreamLNK-DL-CC2340-B_C19273634.pdf indir
2. `C:\PressureTransmitter\DATASHEETS\` içine kaydet
(Alternatif: Claude WebFetch ile deneyebilir — CARD-5.1 başında)
**What Claude needs back:** Dosya adı/yolu

## MANUAL-6 — git init onayı
**Status:** [ ] Waiting
**Blocking:** YES → CARD-0.3 (ve dolaylı olarak güvenli rollback)
**Tool:** Karar
**Added by:** BOOTSTRAP-01
**Instructions:** Repo'nun git'e alınmasını onayla/reddet (commit politikası: yalnız yerel, push yok)
**What Claude needs back:** Evet/Hayır
