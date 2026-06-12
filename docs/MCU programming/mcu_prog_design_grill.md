# mcu_prog — Design Grill (Spec ↔ Repo Karşılaştırması)

**Tarih:** 2026-06-12

## Okunan Kaynak Dokümanlar

| Doküman | Özet |
|---|---|
| `docs\MCU programming\mcu programming memory.txt` | Görev tanımı: 4-20 konfig firmware, DS154S10Z0TG01 LCD, sıcaklık kompanzasyonu, kalibrasyon, 3 buton menü state machine, BLE konfig |
| `PIN_MAPPING.xlsx` | MCU Pin FMEDA — 5 konfig için pin haritası + EN 61508/EN 50402 tanı teknikleri. Pin otorite kaynağı |
| `BOM\PCBPT910G1_4_20.xlsx` | 4-20 konfig dizgi listesi — komponent varlığı teyidi |
| `DATASHEETS\DS154S10Z0TG01.pdf` (s.1-4) | LCD: BDS154S10Z0TG01, ST7789V denetleyici, 240×240, SPI-4wire ✓ |
| `Firmware\App\INTEGRATION.md` | Mevcut firmware entegrasyon rehberi + bilinen TODO'lar |

## Spec ↔ Repo Boşlukları

| Gereksinim (memory.txt) | Repo durumu | Boşluk |
|---|---|---|
| 4-20 konfig firmware | 8 App modülü yazılmış, entegre | Build hatası (QUADWORD); donanım doğrulaması hiç yapılmamış |
| Sıcaklık kompanzasyonu | Diyot modeli var (kavramsal doğru) | Parça 1N4148 (teyitli); PC1 ikinci diyot kanalı eksik; TMP108 ortam izleme + 60 °C alert hiç yok |
| Kalibrasyon | 2 nokta ΔC + flash saklama + CRC var | Çalışıyor görünüyor; flash program tipi hatalı (build kırığı); donanımda test edilmedi |
| Ekran DS154S10Z0TG01 | lcd400.c ST7789V — datasheet teyitli ✓ | LCD_PWR_ON (PA10) sürülüyor mu doğrulanmalı; donanımda görsel test yok |
| Menü state machine "iyi ayarlanmalı" | NORMAL/MENU/EDIT/CAL_LIVE çalışır durumda | İyileştirme alanları: menü timeout→NORMAL, birim gösterimi, hata ekranları, kontrast/backlight menüsü |
| 3 buton ile ölçüm + kalibrasyon | AH1913 Hall switch ×3, event tabanlı ✓ | Uyumlu — donanım testi bekliyor |
| BLE ile bağlantı + konfig | **HİÇ YOK.** USART3 + GPIO'lar hazır, sürücü/protokol yok | En büyük geliştirme kalemi (Faz 5) |
| Basınç sensörü bağlantısı | FDC2214 sürücüsü var | CLK_EN/SD/INT_B/ERRB pinleri sürülmüyor → bring-up'ta sensör çalışmayabilir |

## Çelişkiler

1. ~~Sıcaklık kaynağı çelişkisi~~ **ÇÖZÜLDÜ — KULLANICI TEYİDİ (2026-06-12):** PC0/PC1 = **1N4148 diyotlar** → sensörün sıcaklık kompanzasyonu için (pin haritasındaki "thermistor" etiketi yanıltıcı). **TMP108 = yalnız ortam sıcaklığı**; 60 °C üstünde ALERT pini FLT_TEMP# (PB5) kesmesi üretecek şekilde konfigüre edilecek. `temp_diode.c` modeli kavramsal olarak doğru; 1N4148 parametre teyidi (onsemi 1N914-D.PDF, 1N4x48'i kapsıyor — repoda mevcut) + PC1 ikinci kanal eklenecek.
2. **Pin haritası X400 = LSE 32.768 kHz** ↔ **BOM X400 = 24.000 MHz kristal**. Designator çelişkisi; şemadan teyit (MANUAL-2).
3. **Pin haritası LCD400'ü "off-BOM external display option"** diye işaretliyor ↔ **4-20 BOM'unda LCD400 = BDS154S10Z0TG01 dizgide VAR**. BOM güncel kabul edildi: ekran var.
4. INTEGRATION.md FDC2214 adresi 0x2A/0x2B belirsizliği — şema/ADDR pin teyidi gerekli (MANUAL-2).

## Belirsiz Alanlar + Önerilen Varsayılanlar

| # | Konu | Önerilen varsayılan |
|---|---|---|
| U1 | Kompanzasyon sıcaklık kaynağı | **CONFIRMED (kullanıcı):** 1N4148 diyot(lar) (PC0/PC1) → sensör kompanzasyonu. TMP108 → yalnız ortam izleme + 60 °C alert (FLT_TEMP#) |
| U2 | Diyot bias devresi (seri direnç / besleme) | BOM'da ayrık diyot yok → diyotlar sensör modülü içinde (Yantai metal kapasitif sensör). Bias direnci/akımı şemadan teyit (MANUAL-2) — V_f25 varsayılanını etkiler |
| U3 | Alarm akımları | NAMUR NE43: düşük alarm 3.6 mA (mevcut), üst saturasyon 20.5 mA, üst alarm 21.0 mA |
| U4 | BLE protokolü | DL-CC2340-B AT-komut modülü (web teyitli). Varsayılan: AT ile advertise/bağlantı yönetimi + transparent UART üzerinde CRC'li basit binary çerçeve protokolü |
| U5 | Menü timeout | 60 s işlem yoksa NORMAL'e dön (kaydetmeden) |
| U6 | TPS3851H30 pencere zamanlaması | Datasheet repoda yok → web'den teyit (CARD-6.1). Mevcut 100 ms kick muhtemelen uyumlu ama doğrulanmalı |

## Eksik Gereksinimler (spec'te olmayan ama gerekli)

- FDC2214 saat/güç sırası (CLK_EN, SD) — bring-up ön koşulu
- TMP108 sürücüsü: **ortam izleme + T_HIGH=60 °C alert konfigürasyonu** + FLT_TEMP# (PB5) kesme işleme + 60 °C aşımında alarm davranışı (ekran/tanı bayrağı)
- ADC taramasına PC1 (ikinci 1N4148 kanalı) eklenmesi (.ioc değişikliği → CubeMX manuel adımı)
- Git versiyon kontrolü (rollback için)
- FMEDA tanı teknikleri (PIN_MAPPING "Standards Ref"): ADC makullük, GPIO read-back, saat izleme, CRC — uzun vadeli faz

## Blocking Sorular

**YOK** — tüm belirsizlikler güvenli varsayılanlarla ilerletilebilir. U1–U6 varsayılanları state.md'de kayıtlı; kullanıcı onayıyla CONFIRMED'e çevrilecek.

## Roadmap'e Etkisi

- Faz 0 baseline (build fix + pin mutabakatı) her şeyin ön koşulu
- Sıcaklık alt sistemi tamamlama (Faz 1: diyot modeli 1N4148 teyidi + PC1 kanalı + TMP108 ortam/alert) kompanzasyondan (Faz 2) önce gelmek zorunda
- BLE (Faz 5) bağımsız ilerleyebilir ama donanım bring-up (Faz 1) sonrası test edilebilir
- .ioc değişiklikleri kullanıcının CubeMX'te regenerate etmesini gerektiriyor → manuel adım kapıları
