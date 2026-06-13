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

---

## TPS3851 Windowed Watchdog (CARD-6.1) — 2026-06-13

**Kaynak:** TI datasheet SBVS300B (Kasım 2016 – rev. Haziran 2025), https://www.ti.com/lit/ds/symlink/tps3851.pdf (indirildi, metin çıkarıldı)

**Bulgular:**
- WDI: **düşen kenar** tetiklemeli — "A falling edge must occur at WDI before the timeout (tWD) expires."
- **Pencereli (windowed)** watchdog: WDI valid bölgesi tWD(MIN)..tWD(MAX); **çok erken kick = fault** (WDO Late/Early). Şek. 6-2 tolerance window.
- tWD **CWD pinine bağlı** (programlanabilir):
  - Standart zamanlama: CWD=0.1nF → tWD(typ) **0.704 ms**; 1000nF → **3.23 s**
  - Extended zamanlama: 0.1nF → **62.74 ms**; 1000nF → **77.45 s**
  - CWD pullup direnci VDD'ye 9-11 kΩ ile fabrika-default gecikme seçenekleri
- WDO ayrı çıkış (RESET'ten bağımsız fault flag); WDI, RESET veya WDO low iken yok sayılır.
- WDI, watchdog disable (SET1) ile kapatılabilir.

**Uygulama çıkarımları (KRİTİK):**
- Firmware kick periyodu (şu an 100 ms) **kart CWD konfigine bağlı** pencerenin içinde olmalı: **tWD(MIN) < KICK_PERIOD < tWD(MAX)**. CWD değeri şemadan okunmadan mutlak pencere bilinemez.
- 100 ms kick anlamlıysa kart **extended timing + uygun CWD** kullanıyor olmalı (standart 0.1nF=0.7ms olsaydı cihaz sürekli reset atardı). **Şema teyidi şart** → MANUAL-2'ye eklendi.
- Pencereli olduğu için **çok hızlı kick de reset üretir** — periyot tek sabitle (WDT_KICK_PERIOD_MS) ayarlanabilir tutuldu.

**Stale/belirsiz:** Kartın CWD bağlantısı (kapasitör değeri / pullup / NC) ve standart-vs-extended seçimi datasheet'ten değil ŞEMADAN gelir — MANUAL-2 madde 6.

Kaynaklar:
- [TPS3851 datasheet (TI SBVS300B)](https://www.ti.com/lit/ds/symlink/tps3851.pdf)
- [TPS3851 ürün sayfası](https://www.ti.com/product/TPS3851)

---

## DL-CC2340-B Datasheet (CARD-5.1) — 2026-06-13

**Kaynak:** `DATASHEETS\C19273634.pdf` (DreamLNK DL-CC2340-B V1.0, kullanıcı indirdi — MANUAL-5)

**Teyitler:**
- **Varsayılan UART: 115200 8N1** (firmware USART3 ile UYUMLU — baud değişimi gerekmiyor)
- AT formatı: `AT+<cmd> p1,p2` (execute) / `AT+<cmd>=p1,p2` (set) → yanıt `OK\r\n` veya `ERROR:<errno>\r\n`
- Async URC (bağlantı durumu UART'tan gelir, ayrı pin YOK): `+SCANRET:...`, `+CONNOK:Handle=%u,Addr=%s,Role=%u,Num=%d`, `+DISCONN:Handle=%u,Reason=%u,Num=%d`, BLE param update
- Pin eşleştirme:
  - DIO20 UART-TX (out) → MCU RX = PC10 (USART3_RX)
  - DIO22 UART-RX (in)  ← MCU TX = PC11 (USART3_TX)
  - **DIO24 MODE** (sleep ctrl): 0=Sleep, 1=Wake (default HIGH) → BLE_MODE/PB12
  - **DIO21 AUX** (data-ready/busy): L=idle, H=veri var/buffer dolu → BLE_EVENT/PB0 (EXTI0)
  - **RESET** aktif-LOW → BLE_RESET/PA15 (open-drain)
  - VCC enable → BLE_PWR_ON/PC2
- Zamanlama: Reset duration <100 ms; AT↔transparent geçiş <2 ms; serial→BLE buffer 200 B TX/RX, taşma paket kaybı
- Transparent service UUID + write/read UUID AT ile ayarlanabilir (CARD-5.2)

**Uygulama çıkarımı:** Taşıma katmanı 115200'de hazır; bağlantı durumu URC parse ile (CARD-5.2). AUX pini MCU uyandırma/data-ready; transparent veri RX IT ring'e düşer.
