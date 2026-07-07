# HART Entegrasyonu — `feature/hart`

**Hazırlayan:** Süeda Öz · **Tarih:** 06.07.2026
**Kapsam:** 4-20 mA + HART varyantı (`PCBPT910G1_4_20_HART`) için HART slave (field device) desteği.

---

## 1) Donanım gerçekleri (BOM + PIN_MAPPING + şema sf.4 doğrulaması)

- HART modemi **B404 = TI DAC8742H** (HART/FF/PA fiziksel katman modemi).
  Not: eski notlarda AD5700 varsayılıyordu; **HART varyant BOM'u DAC8742H diyor** (`BOM/PCBPT910G1_4_20_HART.xlsx`, satır B404). Datasheet: `DATASHEETS/__TI_DATASHEETS/dac8742h.pdf`.
- Loop sürücüsü **değişmiyor**: XTR111 (U400) + INA190 (U401) HART varyantında da aynen dizili. Modem, loop'a kuplaj devresiyle FSK bindirir.
- MCU bağlantıları (PIN_MAPPING "4-20 mA + HART" sütunu):

| MCU pini | Net | DAC8742H | İşlev |
|---|---|---|---|
| PA2 (LPUART1_TX, AF8) | LP_UART_TX | UART_IN | MCU → modem veri |
| PA3 (LPUART1_RX, AF8) | LP_UART_RX | UART_OUT | modem → MCU veri |
| PB1 (GPIO out) | TI_RTS | UART_RTS | LOW = gönder (modülatör aktif) |
| PC12 (GPIO out) | RST#TI | RST | aktif-LOW reset/power-down |
| PD2 (GPIO out) | CLK_EN | — | X402 4 MHz XO enable (X403/FDC ile ortak) |

- **CD (carrier detect) — DÜZELTME (şema sf.4 incelemesi):** B404 pin 7 (`CD/IRQ`) şemada `CD_IRQ` netiyle **PA0'a bağlı**. PIN_MAPPING.xlsx'in PA0 satırı "FDC2214 interrupt (B405)" diyor — bu bir **dokümantasyon hatası** (FDC'nin kesmesi INT_B/PB7'de). Mevcut kod CD kullanmıyor; çerçeve senkronizasyonu karakter-arası **gap timeout** (35 ms) ile yapılıyor ve bu doğru/yeterli. İyileştirme adayı: PA0'a EXTI0 bağlayıp CD ile alım pencereleme (bkz. §6).
- IF_SEL dahili pull-down → modem **UART modunda** (SPI değil; PA5/PA7 SPI1 hattı bilinçli boşta).
- 4-20 (HART'sız) varyantta B404 dizili değil: `hart_init()` yine çağrılabilir, hat sessiz kalır, yan etkisi yok.

## 2) Eklenen yazılım mimarisi (katmanlı)

```
                    pressure_app.c  (ölçüm + durum → hart_live_t)
                          │ hart_service(now, &live)   her döngü geçişi
                    ┌─────▼─────┐
                    │  hart.c   │  orkestratör (sayaçlar: rx/tx frame)
                    └─┬───────┬─┘
       byte in/out    │       │  istek → cevap
                ┌─────▼──┐ ┌──▼───────┐
                │hart_phy│ │ hart_dl  │  çerçeve parse/kur (HAL'SİZ)
                │ (HAL)  │ │hart_cmds │  universal komutlar (HAL'SİZ)
                └────────┘ └──────────┘
```

- `hart_phy.[ch]` — LPUART1 (1200 baud, **8 veri + odd parity + 1 stop** = HAL'de `WORDLENGTH_9B`), RTS/RST pin kontrolü, IT tabanlı RX ring + TX. LPUART1 CubeMX'te tanımlı değil; modül GPIO+clock+NVIC+IRQ handler'ı kendi kurar (**.ioc değişmedi** — ble_uart'ın USART3 deseniyle aynı).
- `hart_dl.[ch]` — HART çerçevesi: preamble/delimiter/adres(kısa+uzun)/cmd/bc/data/XOR-checksum; ACK kurucu. Saf C.
- `hart_cmds.[ch]` — cihaz veritabanı + komut işleyici. Saf C.
- `hart.[ch]` — ikisini bağlar; yalnız bize adresli STX işlenir, hattaki diğer trafik yok sayılır.

**Değişen mevcut dosyalar (küçük, `HART:` yorumuyla işaretli):**
- `ble_uart.c` — ortak `HAL_UART_RxCpltCallback` içine LPUART1 dispatch'i (2 satır).
- `pressure_app.c` — `hart_init()` (init) + `hart_service()` (süper döngü, BLE servisiyle aynı yerde).
- `CMakeLists.txt` — 4 yeni kaynak dosya.

## 3) Uygulanan komutlar (HART 5 universal + 2 common-practice)

| Cmd | İş | Cmd | İş |
|---|---|---|---|
| 0 | Read Unique Identifier | 13 | Read Tag/Descriptor/Date |
| 1 | Read PV (bar) | 14 | Read PV Transducer Info |
| 2 | Read Loop Current + % Range | 15 | Read Output Info (LRV/URV/damping) |
| 3 | Read Dynamic Variables | 16/19 | Read/Write Final Assembly |
| 6 | Write Polling Address | 17 | Write Message |
| 11 | Read Unique ID by Tag | 18 | Write Tag/Descriptor/Date |
| 38 | Reset Config-Changed (CP) | 48 | Read Additional Status (CP) |

Değişken eşlemesi: **PV** = basınç (bar, birim kodu 7); **SV** = proses sıcaklığı (1N4148, °C); **TV** = kart sıcaklığı (TMP108, °C). Loop akımı = INA190'dan **ölçülen** değer (komut edilen değil). Cmd 48 ilk baytı = BLE'deki `bstat` durum baytıyla aynı.

## 4) Doğrulama (yapılan)

- **Host birim testi:** `App/Test/hart_host_test.c` — dl+cmds katmanları PC'de gcc ile derlenip koşuldu. 28 kontrol, tümü geçti: parse/checksum, kısa+uzun adres eşleme, cmd 0/1/3/6/11 uçtan uca, bozuk checksum reddi, gap timeout, RC 64.
  ```
  cd Firmware/App/Test
  gcc -Wall -Wextra -I../Inc ../Src/hart_dl.c ../Src/hart_cmds.c hart_host_test.c -o hart_test && ./hart_test
  ```
- **Sözdizimi:** tüm yeni + değişen dosyalar gerçek HAL başlıklarıyla `gcc -fsyntax-only` temiz.
- Hedefte tam derleme + gerçek HART master ile test henüz **yapılmadı** (donanım/toolchain gerekli) → §6.

## 5) Branch akışı

```
git checkout -b feature/hart
git add Firmware/App/Inc/hart*.h Firmware/App/Src/hart*.c \
        Firmware/App/Test/hart_host_test.c Firmware/App/HART_INTEGRATION.md \
        Firmware/App/Src/ble_uart.c Firmware/App/Src/pressure_app.c Firmware/CMakeLists.txt
git commit -m "feat(hart): DAC8742H HART slave - universal komutlar + host testleri"
```

## 6) Açık işler / kararlar (ekiple konuşulacak)

1. **Kimlik kodları geçici:** `mfr_id=250` (kayıtsız), `dev_type=0x01`, `dev_id=000001`. Ticari ürün için FieldComm Group üyeliği/ID kaydı gerekir; dev_id üretimde seri numarasından yazılmalı (`hart_db_init`).
2. **Kalıcılık yok:** tag/message/poll adresi RAM'de; güç kesilince default'a döner. Öneri: `cal_storage` v3'e alan eklemek (CRC'li mevcut düzen uygun).
3. **Sensör limitleri placeholder:** `usl=10 bar, lsl=0, min_span=0.5` — ürün spec'iyle güncellenmeli (`hart_db_init`).
4. **Burst mode yok** (master sormadan periyodik yayın). Universal set için zorunlu değil; istenirse eklenir.
5. **HART 5 seçildi** (ekipte ilk HART; en yaygın ve en yalın taban). HART 7'ye geçiş = cmd 0'ın 2 baytlık genişletilmiş device-type formatı + long tag (cmd 20/22) + cmd 38/48'in zorunlu hale gelmesi; katmanlı yapıda lokal değişiklik.
6. **CD bağlı olmadığından** çok gürültülü hatlarda çerçeve senkronu yalnız gap-timeout'a dayanır. Sahada sorun görülürse: donanım rev'inde CD→GPIO önerilir.
7. **Doğrulama sırası:** (a) hedefte derle+flaşla, (b) HART modem/el terminali veya PC master (örn. USB HART modem) ile cmd 0→1→3 turu, (c) parity/checksum hata enjeksiyonu.
8. **Cold-start/cfg-changed basitleştirmesi:** bitler master-başına değil global takip ediliyor (tek-master saha kurulumu için yeterli; çift-master isteniyorsa ayrıştırılmalı).
9. **PIN_MAPPING düzeltmesi raporlanmalı:** PA0 satırı "CD_IRQ — FDC2214 (B405)" yazıyor; şemaya göre doğrusu **B404 DAC8742H CD/IRQ**. (FDC kesmesi INT_B = PB7.) HART varyantında PA0 → EXTI0 ile carrier-detect kullanımı opsiyonel iyileştirme.
10. **LP_HART varyantı farklı loop sürücüsü kullanıyor:** B402 DAC161S997 (SPI, PB2=NSS) + DAC8742H çifti; XTR111 dizilmiyor. Bu koddaki hart_dl/hart_cmds katmanları aynen geçerli kalır, hart_phy da değişmez (modem aynı); yalnız loop sürüş katmanı (xtr111_loop) LP varyantında DAC161 sürücüsüyle değiştirilmeli.
