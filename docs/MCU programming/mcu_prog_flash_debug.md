# mcu_prog — Flash & Debug Altyapısı

**Tarih:** 2026-06-13 | **Soru:** Yükleme ve debug'ı Claude yapsın — nasıl mümkün?

## Özet: EVET, mümkün — tüm araçlar kurulu

Sistemde gerekli her şey mevcut; ekstra kuruluma gerek yok. Claude yüklemeyi ve **kaynak-seviyesi canlı debug'ı** (breakpoint, step, `print degisken`, backtrace) komut satırından (headless) yapabilir. Tek fiziksel gereksinim: **ST-Link board'a + bilgisayara takılı olmalı** — bunu sen yaparsın, gerisini Claude.

### Kurulu araçlar (tespit edildi)

| Araç | Yol | İşlev |
|---|---|---|
| STM32_Programmer_CLI v2.21.0 | `C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\` | Flash, doğrulama, bellek oku/yaz, reset |
| ST-LINK_gdbserver 7.13.0 | `C:\Users\Alper\AppData\Local\stm32cube\bundles\stlink-gdbserver\7.13.0+st.3\bin\` | GDB server (canlı debug) |
| arm-none-eabi-gdb 15.2.1 | `C:\tools\arm-none-eabi-gcc\...\bin\` | GDB istemci |

STM32CubeProgrammer v2.21 STM32U385'i (U3 serisi, dahili flash) destekler — harici loader gerekmez.

---

## İş bölümü (kritik)

Claude **probe'u yazılımla sürer**, ama fiziksel dünyaya dokunamaz.

| Claude (CLI ile, otomatik) | Sen (fiziksel) |
|---|---|
| Firmware derle (cmake) | ST-Link'i board'a + USB'ye tak |
| Flash + verify (Programmer CLI) | 24 V + 250 Ω yük + multimetre bağla |
| GDB server başlat + gdb sür | Basınç referansı uygula |
| Breakpoint, step, değişken/bellek oku | Butonlara bas, LCD'yi gözle |
| RAM yapılarını/flash kaydını incele | Multimetre/scope değerlerini Claude'a ilet |
| Reset, core register oku, hard fault analizi | Şema teyitleri (MANUAL-2) |

Yani: **sen kabloları bağlar + gözlemleri iletirsin; Claude flash + GDB ile kod tarafını sürer ve yorumlar.**

---

## 1) Flashing (şu an çalışır)

Board takılıyken Claude şunu çalıştırır (veya `tools\flash.bat`):

```
STM32_Programmer_CLI -c port=SWD mode=UR reset=HWrst -d <ELF> --verify -rst
```

- `mode=UR` (connect-under-reset): SWD pinleri yeniden konfigüre edilse bile güvenli bağlanır.
- `--verify`: yazılanı geri okuyup doğrular.
- `-rst`: yükleme sonrası çalıştırır.
- ELF: `C:\PressureTransmitter\Firmware\build\Debug\PressureTransmitter.elf`

**İlk bağlantı testi:** `tools\probe_test.bat` → "Device name: STM32U3..." görürsek ST-Link + hedef sağlam.

---

## 2) Canlı kaynak-seviyesi debug (GDB)

İki adım (Claude ikisini de yapar):

**a) GDB server'ı arka planda başlat** (`tools\gdb_server.bat`):
```
ST-LINK_gdbserver -p 61234 -cp "<CubeProgrammer\bin>" -d -e -r 1 -k
```

**b) GDB istemcisini batch komutlarla sür:**
```
arm-none-eabi-gdb -q -x <komut-dosyasi> <ELF>
```
Örnek komutlar `tools\gdb_cmds_example.txt`'te. Claude duruma göre üretir:
- `target extended-remote localhost:61234`
- `monitor reset halt`
- `break cal_save` / `continue` / `backtrace` / `info locals`
- `print s_cal` (kalibrasyon yapısı), `print loop_get_measured_ma()`
- `x/4hx &s_adc_buf` (ham ADC tamponu)
- hard fault sonrası: `print/x $pc`, `info registers`, stack unwind

Bunların tamamı **scriptlenebilir/headless** — Claude breakpoint koyar, çalıştırır, değişken okur, yorumlar.

---

## 3) Post-mortem bellek incelemesi (GDB server'sız da çalışır)

Hızlı "ne yazıldı / RAM'de ne var" için GDB server bile gerekmez:
```
# sembol adresini öğren
arm-none-eabi-nm PressureTransmitter.elf | findstr s_cal
# o adresi canlı oku (örn. kalibrasyon RAM kopyası)
STM32_Programmer_CLI -c port=SWD mode=UR -r32 0x20000xxx 0x20
# flash kalibrasyon kaydını oku (page 127 = 0x0807E000)
STM32_Programmer_CLI -c port=SWD mode=UR -r32 0x0807E000 0x40
```

---

## 4) Canlı telemetri seçenekleri

- **SWO/ITM trace:** PB3 = SWO kartta mevcut (pin haritası). ST-LINK_gdbserver SWV yakalayabilir → printf'i ITM'e yönlendirip Claude canlı P/T/mA akışını okuyabilir. (İstenirse küçük bir `dbg_swo` kartı açılır — CARD-7 kapsamı.)
- **BLE GET_MEAS:** ble_proto zaten P/T/mA/status veriyor; ama BLE central (telefon) tarafını Claude süremez — bu senin nRF Connect testin.

---

## Donanım geldiğinde akış

1. Sen: ST-Link tak → Claude'a "takılı" de.
2. Claude: `probe_test` → bağlantı teyidi.
3. Claude: `flash` → firmware yüklenir + doğrulanır.
4. Sen: güç/yük/sensör bağla, gözlemleri ilet.
5. Claude: GDB ile bring-up kartlarını (CARD-7.1) sürer; sorun olursa breakpoint/bellek ile teşhis.

**Not:** Bunlardan önce MANUAL-2 şema teyitleri (özellikle TPS3851 CWD watchdog penceresi — yanlışsa flash sonrası sürekli reset riski) netleşmeli.
