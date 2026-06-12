# mcu_prog — Memory (her oturum başında oku)

## Proje Hedefi
STM32U385RGT7 basınç transmitteri, **4-20 mA konfig** firmware'i: FDC2214 kapasitif sensör → sıcaklık kompanzasyonlu basınç → XTR111 4-20 mA; 3 Hall-buton LCD menü/kalibrasyon; BLE (DL-CC2340-B) konfigürasyon.

## Anahtar Mimari Gerçekler
- Bare-metal superloop (RTOS yok), zaman dilimli: buton 5 ms / sensör+ADC 100 ms / ekran 250 ms / watchdog kick 100 ms
- App katmanı `Firmware/App/` — 8 modül tam yazılı; `Core/` CubeMX üretimi (yalnızca USER CODE blokları düzenlenir)
- Build: `cd Firmware && cmake --preset Debug && cmake --build build/Debug` (Ninja + arm-none-eabi-gcc 15.2.1)
- Build temiz (CARD-0.1: flash yazımı DOUBLEWORD'e geçirildi; U3 HAL'de QUADWORD yok)
- Kalibrasyon: dahili flash page 127 (son 8 KB), CRC32 + magic "CAL0" v1
- **PIN_MAPPING.xlsx = pin otoritesi** (FMEDA). .ioc/gpio.c tüm pinleri tanımlıyor (CLK_EN boot'ta HIGH ✓); bsp_pins.h alias'ları tam (CARD-0.2). Eksik: uygulama tarafı ERRB/INT_B/FLT_TEMP kesme işleme. CD_IRQ=PA0 kullanılamıyor (EXTI0 hattı BLE_EVENT PB0'da)
- ADC düzeni (regenerate 2026-06-12, bug kapandı): Rank1=IN1/PC0 diyot#1, Rank2=IN2/PC1 diyot#2, Rank3=IN11/PC4 VCC_FB, Rank4=IN12/PC5 I_FB; ADC_RANK_COUNT=4. NOT: PIN_MAPPING.xlsx'in IN13/IN14 bilgisi HATALIYDI — kanal no'larında CubeMX/datasheet otorite
- **Sıcaklık mimarisi (KULLANICI TEYİTLİ):** Kompanzasyon = PC0/PC1 **1N4148 diyotlar** (sensör içi; datasheet: `__TI_DATASHEETS\1N914-D.PDF` 1N4x48 dahil) — temp_diode.c modeli doğru, PC1 kanalı eklenecek. **TMP108 = yalnız ortam sıcaklığı**, T_HIGH=60 °C alert → FLT_TEMP# (PB5) kesmesi. TMP108 kompanzasyonda KULLANILMAZ, failover da yapılmaz
- LCD: BDS154S10Z0TG01 = **ST7789V** 240×240 SPI (datasheet teyitli) — lcd400.c uyumlu
- BLE modülü AT komutlu (DreamLNK DL-CC2340-B, CC2340R5); USART3 PC10/PC11; sürücü YOK
- Watchdog: TPS3851H30 (windowed!) WDI=PC3 + IWDG ~256 ms; pencere zamanlaması teyitsiz

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
- **Faz:** P1 in progress | **Son:** CARD-1.2 TMP108 ortam+alert (seviye 2)
- **Sıradaki:** CARD-1.3 1N4148 çift kanal (ADC .ioc ayağı hazır) → CARD-1.4 rol entegrasyonu
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
