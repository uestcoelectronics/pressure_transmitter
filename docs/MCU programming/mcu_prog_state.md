# mcu_prog — State

> **Authoritative state file. Her görevden sonra güncelle.**

## Session Info

- **Last updated:** 2026-06-12 (Bootstrap oturumu)
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
| D8 | TMP108 alert histerezisi: T_HIGH=60 °C / T_LOW=55 °C; alarmda ölçüm devam eder, uyarı gösterilir | Kullanıcı 60 °C eşiğini verdi; histerezis ve davranış önerilen varsayılan |
| D7 | Commit/push YOK (varsayılan) | ease-me politikası |

## Current Repo Status

| Alan | Durum |
|---|---|
| Build baseline | **complete** — CARD-0.1 ile düzeltildi (DOUBLEWORD); temiz build, 0 uyarı |
| FDC2214 sürücü | in progress — kod var, CLK_EN/SD eksik (CARD-1.1) |
| Sıcaklık ölçümü | in progress — diyot modeli doğru (1N4148 teyitli); PC1 kanalı + TMP108 ortam/alert eksik (CARD-1.2/1.3) |
| Kalibrasyon + flash | complete (build fix sonrası) — donanım testi yok |
| LCD + menü | complete (kod) — donanım testi yok, iyileştirme planlı |
| 4-20 loop sürücü | complete (kod) — donanım testi yok |
| BLE | not started |
| Watchdog | in progress — kick var, pencere teyitsiz |
| Tanılar (diag) | not started |
| Donanım doğrulama | not started |

## Last Completed Task

- **Task ID:** MANUAL-3 + ADC re-rank (CARD-1.3'ün .ioc ayağı) | **Tarih:** 2026-06-12
- **Özet:** Kullanıcı CubeMX'te ADC'yi düzeltti ve regenerate etti: 4 kanal — IN1 (PC0 diyot#1), IN2 (PC1 diyot#2), IN11 (PC4 VCC_FB), IN12 (PC5 I_FB). Rank kanal bug'ı kapandı. Diff kontrolü: yalnız adc.c/.ioc/.mxproject değişti, USER CODE korunmuş. bsp_pins.h: ADC_RANK_TDIODE2=1 eklendi, VCC_FB=2/ILOOP_FB=3/COUNT=4. Build PASS (0/0). **Düzeltme: PC4/PC5 = IN11/IN12** (PIN_MAPPING.xlsx'teki IN13/IN14 HATALI — kullanıcı görsel teyidi + CubeMX).
- **Önceki görev:** CARD-0.2 (+ CARD-0.3) | **Tarih:** 2026-06-12
- **Özet:** CARD-0.3: git init (main, e2120fe, 392 dosya, .gitignore). CARD-0.2: bsp_pins.h'a 7 eksik alias eklendi (FLT_TEMP, BLE×4, RST_TI, TEST_MODE), ADC/diyot yorumları düzeltildi. **Yeni bulgular:** (1) .ioc FDC pinlerini zaten tanımlıyor — CLK_EN boot'ta HIGH (saat riski kapandı); eksik olan uygulama tarafı işleme. (2) **ADC bug: adc.c 3 rank'ın üçünde de CHANNEL_1 örnekliyor** — VCC_FB/I_FB hiç okunmuyor; düzeltme .ioc'ta (MANUAL-3'e eklendi).
- **Değişen:** `App/Inc/bsp_pins.h` (1 dosya), `.gitignore` (yeni)
- **Önceki:** CARD-0.1 (build fix), CORRECTION-01, BOOTSTRAP-01

## Current Task

— (bootstrap tamamlandı, bekleme)

## Next Recommended Task

- **CARD-1.1 — FDC2214 bring-up (uygulama tarafı):** ERRB/INT_B kesme işleme, SD/CLK_EN sıralamasının uygulama kontrolüne alınması, I2C adres tanısı. Donanım gerektirmeyen kısmı kodlanabilir.
- Paralel açık: **MANUAL-3** (CubeMX: ADC rank kanalları bug fix + PC1 ekleme) — CARD-1.3'ün ön koşulu; erken yapılması CARD-4.1'in de önünü açar.

## Open Risks

| Risk | Şiddet | Azaltım |
|---|---|---|
| ~~Git yok~~ | — | KAPANDI: CARD-0.3 (main, e2120fe) |
| ~~ADC rank kanal bug'ı~~ | — | KAPANDI: MANUAL-3 regenerate (4 kanal doğru atandı) |
| PIN_MAPPING.xlsx AF kolonunda hatalar olabilir (IN13/14 örneği) | LOW | Pin haritası net isimleri için otorite; AF/kanal no'ları CubeMX/datasheet'ten teyit edilir |
| FDC ERRB/INT_B kesmeleri uygulamada işlenmiyor | MEDIUM | CARD-1.1 |
| TMP108 ortam alert'i ve PC1 diyot kanalı eksik | MEDIUM | CARD-1.2/1.3/1.4 |
| TPS3851 pencere ihlali → reset döngüsü | MEDIUM | CARD-6.1 zamanlama analizi |
| cal_save flash erase sırasında IWDG reset | MEDIUM | CARD-6.1 kick stratejisi |
| CubeMX regenerate USER CODE dışı kayıplar | MEDIUM | .ioc değişiklikleri manuel + regenerate sonrası diff |
| FDC I2C adresi 0x2A/0x2B belirsiz | LOW | CARD-1.1 çift deneme + MANUAL-2 |

## Open Questions

| # | Soru | Blocking? | Varsayılan / Yanıt |
|---|---|---|---|
| Q1 | ~~Kompanzasyon sıcaklık kaynağı?~~ | — | **KAPANDI:** 1N4148 diyotlar (kullanıcı teyidi); TMP108 yalnız ortam + 60 °C alert |
| Q2 | Diyot bias direnci/akımı (V_f25 varsayılanı için)? | Hayır (varsayılanla ilerlenebilir) | MANUAL-2 şema teyidi |
| Q3 | git init onayı? | CARD-0.3 için evet | Kullanıcı kararı |
| Q4 | X400/X401 kristal designator çelişkisi (BOM 24 MHz vs pin map 32.768 kHz) | Hayır | MANUAL-2 |
| Q5 | BLE yazma koruması: PIN mi menü-unlock mu? | Hayır | D4 kapsamında v1: sabit PIN |

## Manual Steps Waiting

Bkz. `mcu_prog_manual_steps.md` — MANUAL-1 (donanım hazırlığı), MANUAL-2 (şema teyitleri), MANUAL-5 (BLE datasheet indirme). Hiçbiri CARD-0.1 için blocking değil.

## Last Validation Results

- **Tarih:** 2026-06-12 | **Tip:** `cmake --build build/Debug` (CARD-0.1 sonrası)
- **Sonuç:** PASS — 0 hata, 0 uyarı, `PressureTransmitter.elf` link edildi
- **Ulaşılan seviye:** **2** (derleme) — donanım testi yapılmadı

## Continuity Rules

Her oturum başında: memory → state → roadmap → backlog → changelog oku. Her kart sonunda: state, changelog, tracker güncelle; mimari değiştiyse memory güncelle. Chat geçmişine güvenme — bu dosyalar otorite.
