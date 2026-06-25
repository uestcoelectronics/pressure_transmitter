# mcu_prog — State

> **Authoritative state file. Her görevden sonra güncelle.**

## Session Info

- **Last updated:** 2026-06-22 (HW-READY-01: donanım bring-up hazırlığı, MANUAL-2 kapandı)
- **Repo path:** `C:\PressureTransmitter`
- **Workspace path:** `C:\PressureTransmitter\docs\MCU programming`
- **Current branch:** main (git init 2026-06-12, CARD-0.3)
- **Last commit hash:** e2120fe (initial) — CARD-0.2 commit'i bu güncellemeyle atılacak

## Implementation Allowed

**YES — kart bazında** (kullanıcı 2026-06-12: "işe başlayalım"). Her kart yine gate'ten geçer; kapsam dışı iş için yeniden onay istenir.

## Assumed Defaults (kullanıcıya sorulmadan kabul edilenler)

| # | Varsayılan | Gerekçe |
|---|---|---|
| D1 | Görev öneki `mcu_prog` | Klasör adından türetildi |
| D2 | ~~TMP108 birincil~~ → **CONFIRMED (kullanıcı, 2026-06-12):** Kompanzasyon = 1N4148 diyotlar (PC0/PC1); TMP108 = yalnız ortam + 60 °C alert → FLT_TEMP# | Kullanıcı düzeltmesi |
| D3 | Alarm akımları NAMUR NE43 (3.6 / 3.8 / 20.5 mA) | Endüstri standardı |
| D4 | BLE: AT komut + transparent UART + CRC'li binary çerçeve | Modül AT destekli (web teyitli) |
| D5 | Menü timeout 60 s | Saha pratiği |
| D6 | 1N4148 varsayılanları: TC≈−2 mV/°C, V_f25 bias akımına bağlı (~600 mV @ ~1 mA) | onsemi 1N914-D.PDF (1N4x48 dahil) repoda; bias direnci MANUAL-2 ile teyit |
| D8 | TMP108: T_HIGH=60 °C, comparator mode, HYS=4 °C (cihazın maks'ı → alarm ~56 °C'de düşer); alarmda ölçüm devam eder, "AMB HOT" uyarısı | Kullanıcı 60 °C eşiğini verdi; 55 °C histerezis donanımda yok — 4 °C maks (datasheet Table 9) |
| D9 | Backlight % runtime-only (boot %60); menü timeout 60 s (CARD-3.2 onaylı) | Persistans için format bump gerekirdi (v2 kapalı) — ertelendi |
| D7 | Commit/push YOK (varsayılan) | ease-me politikası |

## Current Repo Status

| Alan | Durum |
|---|---|
| Build baseline | **complete** — CARD-0.1 ile düzeltildi (DOUBLEWORD); temiz build, 0 uyarı |
| FDC2214 sürücü | complete (kod) — CARD-1.1: sıralama+adres tespiti+ERRB/INT_B; donanım testi MANUAL-4 |
| Sıcaklık ölçümü | complete (kod) — çift 1N4148 + TMP108 ortam/alert; donanım testi MANUAL-4; rol entegrasyonu CARD-1.4 |
| Kalibrasyon + flash | complete (kod) — v2 format + migrasyon + kompanzasyon v2; sağlamlaştırma CARD-2.2; donanım testi yok |
| LCD + menü | complete (kod) — LCD init (3.1) + menü timeout/sayfa/alarm/backlight (3.2); donanım testi yok |
| 4-20 loop sürücü | complete (kod) — NAMUR + sapma tanısı + auto-retry (CARD-4.1); donanım testi yok |
| BLE | complete (kod) — taşıma (5.1) + AT init/advertise + CRC16 çerçeve protokol (5.2); donanım/telefon testi yok |
| Watchdog | complete (kod) — CARD-6.1: A.16 koşullu besleme + flash-safe + deterministik kenar; CWD pencere teyidi (MANUAL-2 m.6) açık |
| Tanılar (diag) | complete (kod) — CARD-6.2: ADC rail-stuck + GPIO read-back + I2C recovery; donanım testi yok |
| Donanım doğrulama | not started |

## Last Completed Task

- **Task ID:** HW-READY-01 | **Tarih:** 2026-06-22 | **Commit:** (uncommitted — doküman)
- **Özet:** Donanım bring-up hazırlığı. Toolchain fiziksel doğrulandı (Programmer v2.21 + gdbserver 7.13 + gdb/nm 15.2.1 + ELF 661 sembol + tools/ + ST-Link sürücü paketi; rebuild=no work). **MANUAL-2 KAPANDI** (tasarımcı değerleri): R_SET=1.2k firmware ile uyumlu✓, FDC ADDR=GND/0x2A✓, diyot bias 3.3V/27k→~100µA (kalibrasyonla ayarlanır), TMP108 0x48-0x4B tarama✓, CWD 1600ms (100ms kick içeride)✓, boot/alarm✓. **BULGU:** firmware HSE/LSE yerine iç MSIS RC0+LSI kullanıyor (X400 24MHz/X401 32.768kHz board'da var, .ioc'da seçili değil) — saat kaynağı kararı bring-up'a ertelendi. WDT programlamada jumper ile disable → reset-loop riski yok. Kod değişikliği YOK.
- **Önceki:** DBG-SWO | **Tarih:** 2026-06-13 | **Commit:** defb6d1
- **Özet:** SWO/ITM canlı telemetri (yeni dbg_swo.c/h): ITM port 0'a 1 Hz "P,T,I,ST" — SWV yoksa no-op. Flash/debug altyapısı kuruldu (tools/ + mcu_prog_flash_debug.md): Claude flash + canlı GDB + SWO telemetri yapabilir, tek fiziksel koşul ST-Link takılı olması. Build PASS 0/0.
- **Önceki:** CARD-5.2 | Commit: 208a458 — **TÜM KOD KARTLARI TAMAM**
- **Özet:** BLE konfig protokolü (yeni ble_proto.c/h): non-bloklayan AT init (advertise "PT910"), transparent CRC16 çerçeve protokolü (GET_MEAS/GET_PARAM/SET_PARAM/SAVE/INFO/UNLOCK, 9 param, sabit-PIN yazma koruması). Build PASS 0/0.
- **Önceki:** CARD-5.1 | Commit: b9e7c5b
- **Özet:** BLE taşıma katmanı (yeni ble_uart.c/h): USART3 IT RX ring (256B), TX, güç/reset/mode sırası, AUX data-ready. USART3 IRQ app'ten enable (.ioc değişmedi). Datasheet (C19273634.pdf): baud 115200 uyumlu. Build PASS 0/0. MANUAL-5 done.
- **Önceki:** CARD-6.2 | Commit: 02d0c3e — P6 kod tarafı TAMAM
- **Özet:** Diag modülü (yeni diag.c/h): A.13 ADC rail-stuck (VCC/I_loop, divider-bağımsız), A.7 GPIO read-back (LOOP_EN kritik→safe state), I2C 9-clock bus recovery (iki cihaz takılınca + reinit). Ekranda "DIAG CHK". Build PASS 0/0.
- **Önceki:** CARD-6.1 | Commit: f63ecaf
- **Özet:** TPS3851 windowed watchdog: deterministik düşen-kenar besleme (wdt_feed_raw), güvenlik görevi canlılığına koşullu (A.16, 400 ms health), cal_save erase öncesi besleme. Datasheet analizi (web). Build PASS 0/0. **Açık önkoşul:** CWD pencere değeri (MANUAL-2 m.6) → 100 ms kick uyumu doğrulanmalı.
- **Önceki:** CARD-3.2 | Commit: 0f43ab1 — P3 kod tarafı TAMAM
- **Özet:** Menü iyileştirmeleri: 60 s eylemsizlik timeout (discard→NORMAL); NORMAL'de 3 sayfa (MAIN/SENSOR/LOOP, UP/DN); loop fault alarm ekranı; backlight % menü öğesi (runtime, kalıcı değil). Build PASS 0/0.
- **Önceki:** CARD-3.1 | Commit: b5fd7c0
- **Özet:** LCD güç sırası + datasheet reset zamanlaması + üretici ST7789V güç-kontrol/gamma dizisi (0xB2/B7/BB/C2-C6/D0/E0/E1 — eksikti). SWRESET kaldırıldı, backlight DISPON sonrası. Build PASS 0/0. Görsel doğrulama MANUAL-4.
- **Önceki:** CARD-4.1 | Commit: 0fe297f — P4 kod tarafı TAMAM
- **Özet:** NAMUR NE43 seviyeleri + satürasyon; sapma monitörü (0.3 mA / 2 s, tanı bayrağı + LED 5 Hz + "LOOP DEV"); FLT auto-retry (5 s, pin seviyesi kontrollü). **BUG FIX:** alarm-low 3.6 mA clamp yüzünden üretilemiyordu. Build PASS 0/0.
- **Önceki:** CARD-2.2 | Commit: 42586fa — P2 kod tarafı TAMAM
- **Özet:** Kalibrasyon sağlamlaştırma: stabilite penceresi (8 örnek, p2p≤2000) + yakalama reddi; span ≥10000 kontrolü; Save öncesi cal_validate; Exit çift onay; transient mesaj API'si + ekran entegrasyonu. Build PASS 0/0. Eşikler donanımda ayarlanacak (MANUAL-4).
- **Önceki:** CARD-2.1 | Commit: 5159b8a
- **Özet:** Kompanzasyon v2: k_t_zero+k_t_span·frac modeli; flash format v2 (vf25/tc persistansı) + v1→v2 migrasyon; menü 11 öğe (kT span eklendi); Vf25/TC tek kaynak cal_params + boot/commit/reload senkronu. Build PASS 0/0. Risk notu: v2 kayıt sonrası eski firmware'e dönüş = defaults.
- **Önceki:** CARD-1.4 | Commit: b70c002 — **P1 kod tarafı TAMAM**
- **Özet:** Rol entegrasyonu: pressure_app'e sıcaklık mimarisi dokümantasyon bloğu; ekran önceliği *FAULT* > SENSOR ERR > TDIODE ERR > AMB HOT > OK. Politika: diyot tutarsızlığı degraded-but-operational (ölçüm durmaz); alarm-low yalnız FDC hatasında.
- **Önceki:** CARD-1.3 | **Tarih:** 2026-06-12
- **Özet:** 1N4148 çift kanal: temp_diode.c/h rework — PC0+PC1 okuma, kanal geçerlilik (V_f 200–1000 mV), |T1−T2|≤5 °C çapraz makullük, tutarlıysa ortalama / tek kanal fallback / son-değer tutma, temp_diode_is_consistent() tanı API'si. Menü bug fix: Vf25/TC edit'i birbirini varsayılana ezmiyor, edit mevcut değerden başlıyor. Build PASS 0/0. NOT: Vf25/TC flash persistansı CARD-2.1 v2 formatına eklendi.
- **Önceki:** CARD-1.2 (TMP108, 953ea00)
- **Özet:** TMP108 ortam monitörü: yeni tmp108.c/h — adres taraması (0x48-0x4B), config 0x2230 (continuous, 1 Hz, comparator, aktif-LOW, HYS=4 °C), THIGH=60 °C/TLOW=-50 °C + read-back doğrulama; 1 Hz poll; FLT_TEMP# EXTI → anında overtemp bayrağı, histeresisle otomatik temizlenme; ekran "AMB HOT >60C". Kompanzasyona bağlanmadı (rol ayrımı korunuyor). Build PASS 0/0.
- **Önceki:** CARD-1.1 (FDC bring-up, 07d6df2)
- **Önceki:** MANUAL-3 + ADC re-rank | **Tarih:** 2026-06-12
- **Özet:** Kullanıcı CubeMX'te ADC'yi düzeltti ve regenerate etti: 4 kanal — IN1 (PC0 diyot#1), IN2 (PC1 diyot#2), IN11 (PC4 VCC_FB), IN12 (PC5 I_FB). Rank kanal bug'ı kapandı. Diff kontrolü: yalnız adc.c/.ioc/.mxproject değişti, USER CODE korunmuş. bsp_pins.h: ADC_RANK_TDIODE2=1 eklendi, VCC_FB=2/ILOOP_FB=3/COUNT=4. Build PASS (0/0). **Düzeltme: PC4/PC5 = IN11/IN12** (PIN_MAPPING.xlsx'teki IN13/IN14 HATALI — kullanıcı görsel teyidi + CubeMX).
- **Önceki görev:** CARD-0.2 (+ CARD-0.3) | **Tarih:** 2026-06-12
- **Özet:** CARD-0.3: git init (main, e2120fe, 392 dosya, .gitignore). CARD-0.2: bsp_pins.h'a 7 eksik alias eklendi (FLT_TEMP, BLE×4, RST_TI, TEST_MODE), ADC/diyot yorumları düzeltildi. **Yeni bulgular:** (1) .ioc FDC pinlerini zaten tanımlıyor — CLK_EN boot'ta HIGH (saat riski kapandı); eksik olan uygulama tarafı işleme. (2) **ADC bug: adc.c 3 rank'ın üçünde de CHANNEL_1 örnekliyor** — VCC_FB/I_FB hiç okunmuyor; düzeltme .ioc'ta (MANUAL-3'e eklendi).
- **Değişen:** `App/Inc/bsp_pins.h` (1 dosya), `.gitignore` (yeni)
- **Önceki:** CARD-0.1 (build fix), CORRECTION-01, BOOTSTRAP-01

## Current Task

**CARD-3.3 KAPANDI (2026-06-25, seviye 4) — ekranda metrik float değerleri görünmüyordu, düzeltildi + donanımda doğrulandı.** `pressure_app.c`'deki 10 `%f` çağrısı integer-ölçekli `fmt_fixed` helper'ına çevrildi. Build PASS, .map'te float-printf linklenmedi, flash/verify PASS, **kullanıcı ekranda değerleri gördü.**

**CARD-7.1 donanım bring-up — DEVAM EDİYOR (2026-06-25 ileri).** Canlı: ADC 4 kanal ✓, FDC2214 okuyor ✓, loop enable ✓, 4-20mA çıkış kalibre ✓, **ekran metrikleri ✓ (CARD-3.3)**, **TEMP_MEAS_ON (PB4) diyot bias enable eklendi+flash'lı ✓** (register doğrulandı).
- **Diyot bias:** PB4 HIGH sürülüyor ama PC0/PC1 rail'e (3.3V) çıkıyor → **diyot ileri yolu açık devre** (sensör sıcaklık-diyot hattı bağlı değil — HW kullanıcıda). Sıcaklık şimdilik ertelendi.
- **Basınç:** boşta ~0.5 bar = **sıfır offset** (iki LC tankı farklı, normal); zero-cal ile silinir (canlı kanıtlandı). Cal şu an default (cap_at_zero=0, span=1e6 placeholder, p_min=0/p_max=10). dC ~48–53k, ±0.012 bar gürültü + ~0.05 bar ısınma drift.
- **Kullanıcı (2026-06-25):** devreyi stabilize etmek için güç+ST-Link çıkardı; tekrar kurunca **menüden zero-cal** yapacak (UP=PA11/DN=PA12/SET=PC13). Stabilite kapısı p2p≤2000 takılırsa FDC RCOUNT artırılır.
- **Kalibrasyon stabilite eşiği gevşetildi:** CAL_STAB_P2P_MAX 2000→6000 (gürültü ±2000). FDC drive düşürme denendi (IDRIVE 31→20) ama osilasyon çöktü → IDRIVE=31 geri. RCOUNT zaten max. Drift (~0.37 bar) = warm-up.
- **cal_save FLASH WRITE ERR ÇÖZÜLDÜ:** STM32U3 128-bit ECC → eksik quad-word PGSERR. Fix: 16-bayt yuvarlama + geri-okuma doğrulaması + flag clear + 3× retry (VDD 2.84V düşük). Kullanıcı testi: hata yok, güç çevrimde ayar korunuyor. **Backlight de çalışıyor** (kod gerekmedi).
- KALAN: (1) **bilinen basınçla span-cal** (zero-cal artık çalışıyor + kalıcı), (2) ERRB kalıcı fix (PA1 hayalet; gate STATUS/data-MSB'ye), (3) diyot HW bağlantısı (PC0/PC1 açık devre, kullanıcı) → sonra sıcaklık, (4) debug scaffolding (if(1) bypass + g_loop_dbg_*) temizliği, (5) VDD'yi 3.3V'a çek (kendi beslemesi). Detay changelog 2026-06-25.

---
**ESKİ NOT:** ADC bug **ÇÖZÜLDÜ** (3 CubeMX iterasyonu, canlı doğrulandı): nihai konfig ContinuousConvMode=DISABLE + ConversionDataManagement=DMA_ONESHOT + DMA Mode=NORMAL + DestInc=INCREMENTED. Firmware artık tam superloop'ta canlı (uwTick akıyor, adc_buf her 100ms taze, jitter kanıtlı, ICSR=0 thread mode). HCLK=48MHz, I2C bus OK (TMP108@0x48, FDC@0x2A). **Kullanıcı sensörleri + 24V beslemeyi bağlamak için gücü kesti (2026-06-23).** Tekrar bağlanınca: probe (VDD kontrol) + canlı FDC/diyot/loop/LCD doğrulama. NOT: VDD 2.85V düşüktü, loop s_fault=1 — kendi beslemesiyle düzelmesi bekleniyor.

## Oturum kapanış durumu (2026-06-22)

**Tüm kod kartları + debug altyapısı TAMAM. Toolchain doğrulandı, MANUAL-2 kapandı. Donanım yarın gelecek.** Bir sonraki oturumda (donanım geldiğinde):
1. Kullanıcı ST-Link'i board'a + USB'ye takar **+ WDT-disable jumper'ını takar** → "ST-Link takılı" der
2. Claude: `docs\MCU programming\tools\probe_test.bat` → bağlantı teyidi (Device name STM32U3...). Sürücü hatası çıkarsa stsw-link009_v3 kurulur (manuel)
3. Claude: `flash.bat` → firmware yükle + verify (WDT jumper-disable, reset-loop riski yok)
4. Claude: gdb_server + SWO telemetri ile bring-up (CARD-7.1); HCLK'yi GDB ile canlı oku (`SystemCoreClock`) → SWO baud
5. 24V + 250Ω + multimetre ile mA doğrulama; R_SET=1.2k transfer fonksiyonu teyidi
**MANUAL-2 KAPANDI** (değerler manual_steps.md'de). **Tek açık nokta:** saat kaynağı — firmware iç MSIS+LSI kullanıyor, board'da HSE 24MHz + LSE 32.768kHz var; BLE/timing sorunluysa bring-up'ta CubeMX ile HSE'ye geçilir (manuel).
Tüm flash/debug detayı: `mcu_prog_flash_debug.md`. Bring-up planı: `~/.claude/plans/yar-n-donan-m-elimde-olacak-wondrous-sifakis.md`.

## Next Recommended Task

- **TÜM KOD KARTLARI TAMAMLANDI** (CARD-0.1 … 6.2, 5.1, 5.2 — 14 kart, hepsi build PASS seviye 2). Kalan: **CARD-7.1/7.2 donanım bring-up + soak** — kart + ST-Link + MANUAL-2 (şema teyitleri) + MANUAL-4 gerektirir. Donanım gelince `/ease-me execute CARD-7.1`.
- **Donanım öncesi blocking:** MANUAL-2 (6 madde: diyot bias, FDC ADDR, TMP108 ADD0, XTR111 R_SET, kristaller, **TPS3851 CWD penceresi**) — özellikle CWD watchdog penceresi (HIGH risk).

## Open Risks

| Risk | Şiddet | Azaltım |
|---|---|---|
| ~~Git yok~~ | — | KAPANDI: CARD-0.3 (main, e2120fe) |
| ~~ADC rank kanal bug'ı~~ | — | KAPANDI: MANUAL-3 regenerate (4 kanal doğru atandı) |
| PIN_MAPPING.xlsx AF kolonunda hatalar olabilir (IN13/14 örneği) | LOW | Pin haritası net isimleri için otorite; AF/kanal no'ları CubeMX/datasheet'ten teyit edilir |
| ~~ADC DMA yanlış konfig → boot'ta hang (OVR storm + DMA TC storm + tek-tarama donma)~~ | — | **KAPANDI (2026-06-23):** MANUAL-7, 3 CubeMX iterasyonu. Nihai: Continuous=DISABLE + DMA_ONESHOT + DMA Mode=NORMAL + DestInc=INCREMENTED. Canlı doğrulandı |
| INT_B data-ready gating: pin beklenmedik HIGH kalırsa ölçüm durur | MEDIUM | MANUAL-4 ilk test maddesi; gerekirse timeout fallback eklenir |
| SWD verify 8MHz'de mismatch veriyor (jumper kablo/2.85V VDD) | LOW | freq=4000 (gerçek 3.3MHz) + mass erase ile flash güvenilir; flash.bat'a freq eklenebilir |
| ST-Link FW (V3J8M3B5S1) gdbserver 7.13 için eski → live GDB breakpoint çalışmıyor | LOW | HOTPLUG bellek/register okuması teşhis için yeterli; breakpoint gerekirse ST-Link FW upgrade |
| TMP108 ortam alert'i ve PC1 diyot kanalı eksik | MEDIUM | CARD-1.2/1.3/1.4 |
| ~~TPS3851 CWD penceresi 100 ms kick'i içermezse reset döngüsü~~ | — | KAPANDI (2026-06-22): ~1600 ms pencere, 100 ms kick içeride; programlamada jumper ile WDT disable |
| Firmware HSE/LSE kristallerini kullanmıyor — iç MSIS RC0 + LSI (X400 24MHz/X401 32.768kHz board'da var, .ioc'da seçili değil) | MEDIUM | Karar bring-up'a ertelendi (2026-06-22): MSIS ~%1; BLE 115200/timing sorunluysa CubeMX'te HSE+LSE'ye geç (manuel `.ioc`) + regenerate. HCLK SWO için GDB ile canlı okunacak |
| ~~cal_save flash erase watchdog starvation~~ | — | KAPANDI: CARD-6.1 erase öncesi besleme |
| CubeMX regenerate USER CODE dışı kayıplar | MEDIUM | .ioc değişiklikleri manuel + regenerate sonrası diff |
| FDC I2C adresi 0x2A/0x2B belirsiz | LOW | CARD-1.1 çift deneme + MANUAL-2 |

## Open Questions

| # | Soru | Blocking? | Varsayılan / Yanıt |
|---|---|---|---|
| Q1 | ~~Kompanzasyon sıcaklık kaynağı?~~ | — | **KAPANDI:** 1N4148 diyotlar (kullanıcı teyidi); TMP108 yalnız ortam + 60 °C alert |
| Q2 | ~~Diyot bias direnci/akımı (V_f25 varsayılanı için)?~~ | — | **KAPANDI (2026-06-22):** 3.3V/27kΩ → ~100µA; V_f25 600mV'den biraz düşük, kalibrasyonla ayarlanır |
| Q3 | git init onayı? | CARD-0.3 için evet | Kullanıcı kararı |
| Q4 | ~~X400/X401 kristal designator çelişkisi (BOM 24 MHz vs pin map 32.768 kHz)~~ | — | **KAPANDI (2026-06-22):** X400=24MHz HSE, X401=32.768kHz LSE (ikisi de var). Firmware şu an MSIS+LSI; HSE kararı bring-up'ta |
| Q5 | BLE yazma koruması: PIN mi menü-unlock mu? | Hayır | D4 kapsamında v1: sabit PIN |

## Manual Steps Waiting

Bkz. `mcu_prog_manual_steps.md` — MANUAL-1 (donanım hazırlığı), MANUAL-2 (şema teyitleri), MANUAL-5 (BLE datasheet indirme). Hiçbiri CARD-0.1 için blocking değil.

## Last Validation Results

- **Tarih:** 2026-06-25 | **Tip:** canlı donanım (CARD-7.1 bring-up: ekran, TEMP_MEAS_ON, stabilite eşiği, cal_save flash fix)
- **Sonuç:** PASS — build 0/0 (FLASH 69324 B); flash/verify PASS; ekran metrikleri ✓, zero-cal STABLE ✓, Save & Exit hatasız + güç çevrimde kalıcı ✓, backlight ✓
- **Ulaşılan seviye:** **4** (canlı donanım, kullanıcı teyitli) — span-cal + diyot HW + ERRB kalıcı fix bekliyor

## Continuity Rules

Her oturum başında: memory → state → roadmap → backlog → changelog oku. Her kart sonunda: state, changelog, tracker güncelle; mimari değiştiyse memory güncelle. Chat geçmişine güvenme — bu dosyalar otorite.
