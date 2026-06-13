# mcu_prog — Self Check

## Bootstrap self-check (2026-06-12)

- [x] 18 workspace dosyası mevcut
- [x] Repo path (`C:\PressureTransmitter`) ve workspace path (`docs\MCU programming`) doğru
- [x] Launcher'lar `where claude` ile doğrulanmış yolu kullanıyor (`C:\Users\Alper\.local\bin\claude.exe`, v2.1.175); `--append-system-prompt-file`, `--continue`, `--permission-mode` CLI help'te teyitli
- [x] Tehlikeli flag yok (`--dangerously-skip-permissions` kullanılmadı)
- [x] State/memory dosyaları okunabilir ve tam
- [x] Roadmap (8 faz) ↔ backlog (16 kart) hizalı; her kart bir faza ait
- [ ] Tracker HTML tarayıcıda açılıp kontrol edildi (kullanıcı: `status_mcu_prog.bat` ile aç)
- [x] Sıradaki aksiyon net: CARD-0.1 (onay gerekli)
- [x] Implementation gate GEÇİLMEDİ — bu oturum planning-only, production kod değişmedi
- [x] "NEXT PROMPT TO CLAUDE" bloğu raporda var

## Kod fazı tamamlanma durumu (2026-06-13)

- [x] **14 kod kartının tamamı çalıştırıldı:** CARD-0.1/0.2/0.3, 1.1/1.2/1.3/1.4, 2.1/2.2, 3.1/3.2, 4.1, 5.1/5.2, 6.1/6.2
- [x] Her kart build PASS (seviye 2); commit'li; git geçmişinde izlenebilir
- [x] state/memory/changelog/tracker/task_packets her kart sonunda güncellendi
- [x] Faz durumu: P0✅ P1✅ P2✅ P3✅ P4✅ P5✅ P6✅ (kod) | P7 donanım bekliyor
- [ ] **Donanım doğrulaması (seviye 4-5) HİÇ yapılmadı** — kart + ST-Link gerekli
- [ ] **Blocking donanım önkoşulu MANUAL-2** (6 madde, özellikle TPS3851 CWD penceresi — HIGH risk)
- Sonraki: `/ease-me execute CARD-7.1` (donanım gelince)

## Per-task self-check (her kart sonunda kopyala-doldur)

- [ ] Pre-implementation gate geçildi
- [ ] Yalnız izinli dosyalar değişti
- [ ] Diff budget aşılmadı (aşıldıysa onay alındı)
- [ ] state.md güncellendi
- [ ] memory.md güncellendi (mimari değiştiyse)
- [ ] changelog.md girişi eklendi
- [ ] tracker.html güncellendi
- [ ] Manuel adımlar kaydedildi
- [ ] Doğrulama seviyesi dürüstçe raporlandı
- [ ] Sıradaki aksiyon net
- [ ] "NEXT PROMPT TO CLAUDE" bloğu var
