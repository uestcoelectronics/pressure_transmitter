# HART 7 Entegrasyonu — Durum ve Tasarım Notları

Güncelleme: 2026-07-08 (HART 5 → HART 7 geçişi tamamlandı)

## Katman yapısı

| Dosya | Katman | HART7 geçişinde değişti mi? |
|---|---|---|
| `hart_phy.c/h` | Fiziksel: DAC8742H modem (UART 1200 8O1) + RTS | Hayır (HART7 fiziksel katmanı değiştirmez) |
| `hart_dl.c/h`  | Data-link: çerçeve parse + ACK kurma           | Yalnız `HART_MAX_DATA` 64→80 (cmd 9 cevabı için) |
| `hart_cmds.c/h`| Uygulama: komut işleyici + cihaz DB            | Evet — HART 7 universal set |
| `hart.c/h`     | Orkestratör: `hart_init` / `hart_service`      | cmd 9 zaman damgası beslemesi eklendi |

## Uygulanan komut seti (HART 7 universal)

0, 1, 2, 3, 6 (0..63 + loop-current-mode), 7, 8, 9 (durum baytlı + zaman
damgalı), 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
38 (16-bit config-change sayacı doğrulamalı), 48 (9 bayt, extended status).

Cmd 0 cevabı 22 bayt: expanded device type (uzun adresle bit-uyumlu),
config-change sayacı, extended field device status, 16-bit mfr/priv-label,
device profile.

## Değişken eşlemesi

PV = basınç [bar], SV = proses sıcaklığı [°C, temp diyotları],
TV = kart sıcaklığı [°C, TMP108], QV yok (`max_dev_vars = 3`).
Loop akımı: INA190'dan ÖLÇÜLEN değer raporlanır (komut edilen değil).

## Bilinçli basitleştirmeler

- Cold-start biti global (master-başına ayrı takip yok). HART7 tam uyumluluk
  testi için primary/secondary master'a ayrı cold-start/cfg-changed takibi
  gerekebilir.
- Burst mode ve event notification uygulanmadı (opsiyonel).
- Cmd 9 zaman damgası boot'tan itibaren geçen süredir (time-of-day yok).

## Açık işler

1. **Kalıcılık (cal_storage v3):** long tag, tag/descriptor/date, message,
   poll adresi + loop-current-mode, cfg-change sayacı şu an RAM'de; güç
   kesilince default'a döner.
2. **Kimlik yer tutucuları:** `mfr_id=250`, `dev_type=0x01`, sabit dev_id.
   Üretimde FieldComm kaydı + seri numarasından türetme gerekir.
3. **Doğrulanacak tablo değerleri:** device profile (=65) ve classification
   kodları (basınç=65, sıcaklık=64) FieldComm Common Tables ile teyit edilmeli.
4. **Varyant kapılama:** 4-20 (modemsiz) varyantta `hart_init/service`
   çalışmaya devam eder (zararsız ama ideal değil) — derleme bayrağı veya
   donanım strap kararı bekliyor.
5. **Donanım testi:** host testleri (Test/hart_host_test.c, 50 kontrol)
   geçiyor; gerçek modem + HART master (el terminali/DTM) ile saha doğrulaması
   yapılmadı.

## Host testini çalıştırma

```
cd Firmware/App/Test
gcc -Wall -Wextra -I../Inc ../Src/hart_dl.c ../Src/hart_cmds.c hart_host_test.c -o hart_test
./hart_test
```
