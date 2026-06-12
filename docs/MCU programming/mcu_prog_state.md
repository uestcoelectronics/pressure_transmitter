# mcu_prog — State

> **Authoritative state file. Her görevden sonra güncelle.**

## Session Info

- **Last updated:** 2026-06-12 (Bootstrap oturumu)
- **Repo path:** `C:\PressureTransmitter`
- **Workspace path:** `C:\PressureTransmitter\docs\MCU programming`
- **Current branch:** YOK — git repo değil (CARD-0.3 bekliyor)
- **Last commit hash:** —

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

- **Task ID:** CARD-0.1 | **Tarih:** 2026-06-12
- **Özet:** Build baseline düzeltildi: `cal_storage.c` flash yazımı QUADWORD → DOUBLEWORD (8 bayt, `uint64_t` hizalı tampon). HAL imzası teyitli (`DataAddress`). Build: 0 hata, 0 uyarı, elf üretildi. Yedek: `cal_storage.c.bak`.
- **Değişen:** `Firmware/App/Src/cal_storage.c` (1 dosya — bütçe içinde)
- **Önceki:** CORRECTION-01 (sıcaklık mimarisi teyidi: 1N4148 kompanzasyon / TMP108 ortam+60°C alert), BOOTSTRAP-01 (planlama)

## Current Task

— (bootstrap tamamlandı, bekleme)

## Next Recommended Task

- **CARD-0.2 — bsp_pins.h ↔ PIN_MAPPING mutabakatı** (onay alındı — kart bazında ilerlenebilir).
- Hâlâ açık: **CARD-0.3 git init onayı** (MANUAL-6) — o gelene dek .bak yedekleriyle çalışılıyor.

## Open Risks

| Risk | Şiddet | Azaltım |
|---|---|---|
| Git yok → rollback imkânsız | HIGH | CARD-0.3 git init; o zamana dek .bak kopyaları |
| FDC2214 saatsiz (CLK_EN sürülmüyor) → sensör ölü | HIGH | CARD-1.1 |
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
