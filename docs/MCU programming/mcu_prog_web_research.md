# mcu_prog — Web Research

**Araştırma tarihi:** 2026-06-12

## Yapılan Araştırma

### DL-CC2340-B BLE Modülü

| Kaynak | İlgisi |
|---|---|
| https://www.lcsc.com/datasheet/lcsc_datasheet_2312012005_DreamLNK-DL-CC2340-B_C19273634.pdf | Modül datasheet'i (PDF — henüz indirilmedi) |
| https://www.iot-rf.com/ble-module-with-ti-cc2340.html | Üretici ürün sayfası |
| https://www.ti.com/product/CC2340R5 | Çip datasheet'i |

**Bulgular:**
- DreamLNK DL-CC2340-B, TI SimpleLink CC2340R5 tabanlı (Cortex-M0+ 48 MHz, 512 KB flash), 18×12 mm, BLE 5.3
- **Dahili AT komut seti var** → UART üzerinden scan/connection/advertise interval, broadcast data, **baud rate** vb. ayarlanabilir; veri modu (transparent UART) destekler
- TX gücü −21…+8 dBm ayarlanabilir; RX 5.3 mA, standby <800 nA

**Uygulama çıkarımları:**
- Harici host-stack gerekmez: USART3 (115200 8N1) + AT komutlarıyla yönetim, bağlantı sonrası transparent veri akışı
- BLE_MODE (PB12) pini muhtemelen AT/veri modu seçimi, BLE_EVENT (PB0) bağlantı/event bildirimi — **kesin pin işlevleri modül datasheet'inden teyit edilmeli** (CARD-5.1 öncesi)
- Varsayılan baud (9600 olabilir!) firmware'in 115200 ayarıyla çelişebilir → ilk haberleşmede otomatik baud deneme veya AT ile ayar

**Stale/belirsizlik bayrağı:** AT komut seti sürümlere göre değişebilir; LCSC datasheet PDF'i CARD-5.1 başlamadan indirilip `DATASHEETS\` klasörüne konmalı (manuel veya WebFetch).

## Ertelenen Araştırmalar

| Konu | Ne zaman | Neden |
|---|---|---|
| TPS3851H30EDRBT pencere zamanlaması (t_window, WDI timeout) | CARD-6.1 | Datasheet repoda yok; kick zamanlaması doğrulanacak |
| NTC termistör parça no / Beta sabiti | CARD-1.3 | Önce şemadan parça teyidi (MANUAL-2) |
| ST7789V init dizisi referansları | Gerekirse CARD-3.1 | Mevcut init büyük olasılıkla yeterli; donanım testi sonrası |

Kaynaklar:
- [DL-CC2340-B datasheet (LCSC)](https://www.lcsc.com/datasheet/lcsc_datasheet_2312012005_DreamLNK-DL-CC2340-B_C19273634.pdf)
- [DreamLNK BLE Module with TI CC2340](https://www.iot-rf.com/ble-module-with-ti-cc2340.html)
- [TI CC2340R5](https://www.ti.com/product/CC2340R5)
