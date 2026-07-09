# Profibus PA Stack Araştırması — v3 (birleştirilmiş)

Tarih: 2026-07-09 · Hazırlayan: Süeda
v1: hızlı tarama · v2: iki derin rapor · v3: gevşetilmiş kriterlerle ek tur
(bellek/RTOS/ASIC kısıtları kaldırılarak; yeni ticari hatlar bulundu)
Soru: "Profibus PA stack'lerinden public olanlar var mı?" (Alper, 7.07)

## Yönetici özeti

**Kriterleri tam karşılayan (permissive lisanslı, C, bare-metal, ASIC'siz,
DAC8742H üstünde çalışan) açık kaynak PA slave stack YOK — bu artık iki
bağımsız derin araştırmayla da teyitli.** Gerçekçi yollar, riskleri azalan
sırayla: (1) TMG TE ile ticari görüşme, (2) Softing PA stack/PAeasy teklifi,
(3) tek-MCU şartından vazgeçilirse Microcyber sertifikalı PA modülü,
(4) kurum içi geliştirme (en düşük BOM, en yüksek süre/sertifikasyon riski).

## Donanım gerçeği (değişmedi)

- DAC8742H (B404) = PA/FF MBP fiziksel katmanı hazır (31,25 kbit/s Manchester
  codec, RX demodülatör, 16B FIFO'lar, ~180 µA). AMA yalnız modem — FDL ve
  üstü tamamen MCU yazılımına kalıyor.
- TI resmi olarak PA stack SAĞLAMIYOR; kendi referans tasarımlarında bile
  stack'i TMG TE'den lisanslatıyor (ti.com/tool/PROFIBUS, sbaa367).
- Kritik zamanlama: FDL'de master sorgusuna T_SDR penceresinde (11 bit-time'a
  kadar inebilir) cevap zorunlu → süper döngüde SPI/UART kesmesi FDL durum
  makinesini doğrudan tetiklemeli, NVIC'te en yüksek öncelik bu hatta.

## Aday tablosu (birleştirilmiş)

| Aday | Lisans/erişim | PA? | Mimarimize uyum | Karar |
|---|---|---|---|---|
| **TMG TE** (DE) | Ticari; tek seferlik kaynak kod lisansı + bakım | PA yetkinliği PI competence-center kaydıyla teyitli; DP-V1 slave çözümü ASIC'siz, ANSI-C, RTOS'suz port edilebilir | En yakın aday; DAC8742H+PA kapsamı kamuya açık DOĞRULANMADI → sorulacak | **1. sıra: teknik görüşme** |
| **Softing** (DE) | Ticari; PAeasy FDK (PA Profile 3.02, sertifikasyon garantili) | ✅ | FBK-2 tabanlı; footprint/DAC8742H uyumu sorulmalı | **2. sıra: teklif al** |
| **Microcyber** (CN) | Ticari kit/modül (NCS-RC105 PA kartı; MC0307 modül) | ✅ gerçek sertifika izi: DP-V0, SAP55, DP-V1 MS1/MS2, I&M, PA 3.02, MBP (ITEI test lab) | Kit: ARM7 + Nucleus RTOS + 256K/128K + FB3050/FBC0409 denetleyici → tek-MCU bütçemizin ÇOK dışında | Ancak "hazır modül ekle" stratejisi kabul edilirse |
| **AB Tech Solution** | Ticari, kaynak kodlu | "DP/PA" diyor | PA için SPC4-2 **ASIC bağımlı** → BOM ihlali | Elendi |
| **M2M craft** (JP) | Ticari C99 kaynak | ❌ (yalnız DP-V0/V1/V2, ASIC'siz, UART+timer) | Temiz DP tabanı; PA profili üstüne bizim yazmamız gerekir | "Kendimiz yazarız" yolunda DP katmanı başlangıcı |
| **Hilscher** | Ticari | DP ağırlıklı | netX çipi/modülü şart → BOM ihlali | Elendi |
| **Port GmbH GOAL** | Ticari middleware | DP | Linux/RTOS sınıfı, 16K RAM için hantal | Elendi |
| **zhaoxuji/profibus_DP_PA_soft** | Yarı açık: yalnız derlenmiş libdppa.a; ticari kullanım/değişiklik yazar izinli | ✅ (iddia: DPV0/V1+PA, <48K/<12K, ASIC'siz) | Kara kutu → SIL2/PI sertifikasyonunda denetlenemez kod sorunu | Yalnız fizibilite denemesi; ürün için riskli |
| **PBMaster** (CZ, GPL-2) / **fredericdepuydt** (GPL-3, arşivli) | Açık ama GPL | ❌ (FDL/DP-V0; PA profili, DP-V1, I&M yok) | **GPL statik bağlama tuzağı**: bare-metal'de tek .elf'e linklenir → firmware'in TAMAMI (kalibrasyon algoritmalarımız dahil) GPL'e tabi olur | Ürüne kopyalanamaz; yalnız mimari referans (clean-room) + lab aracı |
| **pyprofibus** (GPL-2, Python) / **profirust** (Rust) | Açık | ❌ (DP master) | Cihaza uymaz | Test düzeneğinde master simülatörü |
| Trebing+Himstedt, ifak, Deutschmann, Smar, ISIT | — | — | Stack üreticisi değiller (tanı araçları / gateway / son ürün) | Kapsam dışı |

## v3 ek turu: gevşetilmiş kriterlerle yeni adaylar

Ek turda da açık kaynak PA slave stack çıkmadı (sonuç 4. kez teyit). Yeni
bulunan ticari hatlar:

| Aday | Tür | Değerlendirme |
|---|---|---|
| **Fint (Fieldbus International, NO) + Fieldbus Inc. (US)** | Gömülü PA converter modülleri (T410 Modbus→PA, T500 HART→PA) + custom firmware/donanım hizmeti; FF H1 stack'i STM32 Cortex-M3/M4 için **one-time buyout, source code dahil** lisanslıyor; 2025'te Fieldbus Inc.'i satın aldı | **Yeni turun en iyi lead'i.** İki açıdan ilginç: (a) T410 kartı (65 mm yuvarlak, 14 mA) transmitter içine gömülebilir "hazır PA modülü" B-planı — mevcut cihaza Modbus/HART iç arayüzle bağlanır; (b) FF H1 stack'leri STM32'de kanıtlı → "PA portu yapar mısınız?" sorusu çok yerinde. Dikkat: T500 için PA Profile sürümü kaynaklarda çelişkili (3.0 vs 3.02) — teyit şart |
| **Siemens SPC4-2 / DPC31 + SIM1** | PA destekli iletişim ASIC'i + devkit (örnek kaynak kodlu) | Ek çip kabulünde köklü, doğrulanabilir yol (~269 €/5'li lab paketi, 3. parti stok). Ama PA Profile/AI/transducer block/GSD yine bizde kalır — "stack" değil, zamanlama yükünü donanıma alan hızlandırıcı |
| **Aniotek/Softing UFC100-L2** | PA+FF H1 fiziksel+data-link ASIC'i (44-LQFP, düşük güç) | DAC8742H yerine geçen daha yetenekli PHY+DL çipi; üstüne yine stack gerekir. B-plan |
| **SMAR FBStack** | FF H1 stack (STM32L1/F2'ye optimize edilmiş) | PA değil; ama FF+PA ürün geçmişi olan firma — PA port/OEM kartı sorulabilir (zayıf/orta lead) |
| **ifak PA Tester / DP Tester** | PI'nin resmi test araçları | Stack değil; hangi yol seçilirse seçilsin pre-test için plana alınmalı |

v3'ün ana katkısı: **"tek MCU'da soft stack" saplantısından çıkınca** iki
pratik mimari daha görünür oldu — (1) hazır sertifikalı PA modülünü (T410
benzeri) cihazın içine koyup mevcut MCU ile seri hatla konuşturmak (en hızlı
pazara çıkış, BOM +modül maliyeti), (2) PA ASIC + ticari/özel stack karması.

## Kritik teknik notlar (in-house yolu seçilirse)

1. **Bellek stratejisi:** heap yok — tüm tamponlar statik; yalnız 1 Transducer
   Block + 1 AI Block derlenir (#ifdef ile diğer bloklar kapatılır); HAL
   soyutlaması minimum. NOT (v2 düzeltme): araştırma brifindeki "≤64K flash /
   ≤16K RAM" hedefi tahsis VARSAYIMIYDI; STM32U385RG gerçekte 1 MB flash /
   256 KB RAM taşıyor (datasheet teyitli). Yani Microcyber gibi 256K'lık
   stack'ler bellek açısından sığar — oradaki gerçek engeller RTOS (Nucleus)
   ve özel iletişim denetleyicisi bağımlılığı. Bellek bütçesi Alper'le
   netleştirilmeli; ticari görüşmelerde katı sınır olarak sunulmamalı.
2. **Zorunlu kapsam:** FDL slave (T_SDR!), DP-V0 cyclic, DP-V1 MS1/MS2 acyclic
   + I&M0, SAP 55 (Set Slave Address), Ext_Diag (NAMUR NE 107), PA Profile
   3.02/4.x AI + pressure transducer block, quality/status baytları,
   FSAFE_VALUE fail-safe davranışı, hatasız GSD (Ident No. dahil).
3. **Sertifikasyon:** PI akredite lab (ComDeC/ifak) testi şart; sıfırdan stack
   ilk denemede geçmez (edge-case'ler aylar sürer). Ticari stack'in asıl
   sattığı şey bu garanti.

## Eylem önerisi (Alper'e) — v3 ile güncel

1. **TMG TE'ye teknik soru seti gönder** (aşağıda hazır).
2. **Fint'e paralel sorgu** (yeni): (a) T410/T500 firmware'i veya özel gömülü
   PA kartı, PA Profile 3.02/4.x + AI + pressure transducer block + DP-V1 +
   SAP55 + Ext_Diag + GSD ile basınç transmitterine özelleştirilebilir mi?
   (b) FF H1 stack'inizin (STM32, one-time buyout, source) PA portu var mı /
   yapılır mı? T500'ün PA Profile sürümünü (3.0 mu 3.02 mi) netleştirin.
3. **Softing'den PAeasy/PA stack teklifi** al; footprint ve DAC8742H referansı sor.
4. Modül B-planı: Microcyber NCS-RC105 **veya Fint T410** — tek-MCU şartı
   esnerse en hızlı pazara çıkış yolu.
5. ASIC B-planı (SPC4-2 / UFC100-L2) yalnız yukarıdakiler sonuçsuz kalırsa.
6. Hangi yol seçilirse seçilsin **ifak PA Tester/DP Tester** ile pre-test planla.
7. pyprofibus'u lab'da master simülatörü olarak kur.
8. GPL'li repo'lardan ürün koduna TEK SATIR kopyalanmayacak (lisans bulaşması).

### TMG TE'ye sorulacaklar (hazır metin)

> STM32U385 (Cortex-M33) + TI DAC8742H üzerinde, RTOS'suz (cooperative
> super-loop) çalışan PROFIBUS PA field-device stack sağlayabiliyor musunuz?
> Kapsam: DP-V0 cyclic, DP-V1 MS1/MS2, I&M0-4, SAP55/Set_Slave_Add,
> Ext_Diag/alarm, PA Profile 3.02 veya 4.x AI + Pressure Transducer Block,
> GSD/EDD/FDI üretimi, standart quality/status byte, PI pre-test/sertifikasyon
> desteği. Kaynak kod teslimi, flash/RAM footprint, CPU yükü, lisans modeli
> (tek seferlik / royalty) ve DAC8742H referans entegrasyonunuz var mı?

## Kaynaklar (seçme)

- TI: https://www.ti.com/tool/PROFIBUS · https://www.ti.com/lit/pdf/sbaa367 · DAC8742H datasheet
- TMG TE: https://www.io-link.jp/product/mky/doc/Flyer_TMG_TE_2017.pdf · PI competence center (uk.profibus.com)
- Softing PA stack: https://industrial.softing.com/...profibus-pa-field-device-protocol-software.html · PAeasy (profibus.com product finder)
- Microcyber: PA communication board & PA FDK (profibus.com product finder) · NCS-TT106 PA sertifikası (salcas.com.br PDF)
- ABTS: https://abtechteam.com/Software_Libraries_profibus.html
- M2M craft: https://www.m2mcraft.co.jp/en/profibus.html
- Açık kaynak: github.com/zhaoxuji/profibus_DP_PA_soft · sourceforge.net/p/pbmaster · github.com/fredericdepuydt/profibus-dp-software-stack · github.com/mbuesch/pyprofibus
- SMAR teknik makale (status byte & fail-safe): smar.com.br
- v3 ek turu: Fint https://www.fint.no/products/profibus-dp-and-pa · T410 https://www.fint.no/b/t410-modbus-rtu-to-profibus-pa-built-in-converter · FF stack https://www.fint.no/b/foundation-fieldbus-protocol-stack · Fieldbus Inc. satın alma duyurusu (fint.no) · T500 datasheet (fieldbusinc.com) · SPC4-2 (profibus.com product finder + Siemens support DPC31 devkit) · UFC100-L2 (industrial.softing.com) · SMAR FBStack (smar.com.br) · ifak test araçları https://www.ifak.eu/en/produkte/profibus-testwerkzeuge
