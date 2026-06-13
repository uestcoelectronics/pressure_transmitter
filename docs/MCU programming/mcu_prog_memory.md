# mcu_prog — Memory (her oturum başında oku)

## Proje Hedefi
STM32U385RGT7 basınç transmitteri, **4-20 mA konfig** firmware'i: FDC2214 kapasitif sensör → sıcaklık kompanzasyonlu basınç → XTR111 4-20 mA; 3 Hall-buton LCD menü/kalibrasyon; BLE (DL-CC2340-B) konfigürasyon.

## Anahtar Mimari Gerçekler
- Bare-metal superloop (RTOS yok), zaman dilimli: buton 5 ms / sensör+ADC 100 ms / ekran 250 ms / watchdog kick 100 ms
- App katmanı `Firmware/App/` — 8 modül tam yazılı; `Core/` CubeMX üretimi (yalnızca USER CODE blokları düzenlenir)
- Build: `cd Firmware && cmake --preset Debug && cmake --build build/Debug` (Ninja + arm-none-eabi-gcc 15.2.1)
- Build temiz (CARD-0.1: flash yazımı DOUBLEWORD'e geçirildi; U3 HAL'de QUADWORD yok)
- Kalibrasyon: dahili flash page 127 (son 8 KB), CRC32 + magic "CAL0" **v2** (k_t_zero/k_t_span + vf25/tc persistans; v1 otomatik migrate; eski firmware v2'yi okuyamaz)
- **PIN_MAPPING.xlsx = pin otoritesi** (FMEDA). .ioc/gpio.c tüm pinleri tanımlıyor (CLK_EN boot'ta HIGH ✓); bsp_pins.h alias'ları tam (CARD-0.2). Eksik: uygulama tarafı ERRB/INT_B/FLT_TEMP kesme işleme. CD_IRQ=PA0 kullanılamıyor (EXTI0 hattı BLE_EVENT PB0'da)
- ADC düzeni (regenerate 2026-06-12, bug kapandı): Rank1=IN1/PC0 diyot#1, Rank2=IN2/PC1 diyot#2, Rank3=IN11/PC4 VCC_FB, Rank4=IN12/PC5 I_FB; ADC_RANK_COUNT=4. NOT: PIN_MAPPING.xlsx'in IN13/IN14 bilgisi HATALIYDI — kanal no'larında CubeMX/datasheet otorite
- **Sıcaklık mimarisi (KULLANICI TEYİTLİ):** Kompanzasyon = PC0/PC1 **1N4148 diyotlar** (sensör içi; datasheet: `__TI_DATASHEETS\1N914-D.PDF` 1N4x48 dahil) — temp_diode.c modeli doğru, PC1 kanalı eklenecek. **TMP108 = yalnız ortam sıcaklığı**, T_HIGH=60 °C alert → FLT_TEMP# (PB5) kesmesi. TMP108 kompanzasyonda KULLANILMAZ, failover da yapılmaz
- LCD: BDS154S10Z0TG01 = **ST7789V** 240×240 SPI (datasheet teyitli) — lcd400.c uyumlu
- BLE: DL-CC2340-B (datasheet C19273634.pdf), 115200 8N1, AT "AT+<cmd> p1,p2"→OK/ERROR, URC +CONNOK/+DISCONN. Pinler: DIO20→PC10(RX), DIO22←PC11(TX), MODE/PB12(wake=HIGH), AUX/PB0(data-ready), RESET/PA15(OD), PWR/PC2. CARD-5.1 taşıma katmanı (ble_uart.c) hazır; USART3 IRQ app'ten enable (.ioc'de kapalı)
- Watchdog: TPS3851H30 (windowed, düşen-kenar) WDI=PC3 + IWDG ~256 ms. CARD-6.1: wdt_feed_raw() koşullu besleme (güvenlik görevi canlı + 400 ms), cal_save erase öncesi besler. tWD CWD'ye bağlı (std 0.7ms-3.23s/ext 62ms-77s) — KICK 100 ms pencereye düşmeli, CWD şema teyidi MANUAL-2 m.6

## Kullanıcı Tercihleri
- İletişim Türkçe; teknik terimler İngilizce kalabilir
- Önce 4-20 konfig; diğer konfigler (HART/LP/MODBUS) kapsam dışı
- ease-me süreci: planlama → onay → kart kart uygulama

## Sert Korkuluklar / Do-Not-Break
- Boot'ta loop disabled (PB2=LOW); fault → 3.6 mA
- Watchdog kick disiplini bozulmamalı
- Kalibrasyon flash formatı değişirse version alanı artmalı + migrasyon
- Git: main branch (init 2026-06-12); commit'ler yerel, push YOK

## Do-Not-Touch
`Firmware/Drivers/**`, `Firmware/cmake/**`, `startup_*.s`, `*.ld`, `Core/**` USER CODE dışı, `Firmware/build/**`, `.ioc` (elle düzenleme yok → CubeMX manuel adımı)

## Roadmap Konumu
- **Faz:** P0-P4+P6 + CARD-5.1 kod TAMAM | **Son:** CARD-5.1 BLE taşıma (b9e7c5b)
- **Sıradaki:** CARD-5.2 BLE protokolü (AT init, advertise, URC parse, GET/SET çerçeve+CRC). Son kod kartı; sonrası 7.x donanım manuel
- diag modülü: ADC rail-stuck, GPIO read-back (LOOP_EN kritik→safe state), I2C 9-clock recovery; ekranda "DIAG CHK"
- UI: 60 s timeout, NORMAL 3 sayfa (sm_get_normal_page), fault alarm ekranı, backlight runtime (%60 boot, kalıcı değil)
- Loop: NAMUR sabitleri xtr111_loop.h'ta; komut penceresi 3.6..21.0; sapma 0.3 mA/2 s; FLT retry 5 s
- Kalibrasyon eşikleri: CAL_STAB_P2P_MAX=2000, CAL_MIN_SPAN_COUNTS=10000 (state_machine.c, donanımda ayarlanacak)
- Kompanzasyon: P −= (k_t_zero + k_t_span·frac)·ΔT; menü 11 öğe ("/11" pressure_app'te hardcoded — MI__COUNT ile senkron tut)
- Ekran satır 3 önceliği: *FAULT* > SENSOR ERR > TDIODE ERR > AMB HOT > OK; alarm-low yalnız FDC hatası
- temp_diode: çift kanal arbitrasyon (geçerlilik 200–1000 mV, |ΔT|≤5°C, ortalama/fallback/son-değer), is_consistent() tanısı; Vf25/TC flash persistansı CARD-2.1'de
- TMP108: adres tarama 0x48-0x4B, comparator HYS=4°C (60→~56°C), FLT_TEMP# EXTI main.c'de bağlı, "AMB HOT" ekranda
- Implementation: kart bazında ONAYLI (kullanıcı 2026-06-12)
- FDC2214: adres otomatik tespit (0x2A/0x2B), ERRB/INT_B polling, SD+CLK_EN sıralaması app'te

## Önemli Varsayımlar (onaylanmamış)
- NAMUR NE43 alarm seviyeleri (D3); BLE AT+transparent+CRC çerçeve (D4); menü timeout 60 s (D5); 1N4148 V_f25≈600 mV @ ~1 mA / TC≈−2 mV/°C (D6 — bias direnci MANUAL-2); TMP108 histerezis 60/55 °C, alarmda ölçüm sürer (D8)

## Manuel Araç Sınırları
- Donanım flash/test: ST-Link + kart + multimetre (kullanıcı)
- .ioc değişikliği: STM32CubeMX regenerate (kullanıcı)
- Şema PDF (4-20.PDF) görüntü tabanlı — Claude render edemiyor (pdftoppm yok); şema teyitleri kullanıcıdan
- BLE testi: telefon + nRF Connect (kullanıcı)
