# Kurulum

Bu projeyi kendi 2009 Ducato X250 göstergemi ETS2 ve BeamNG ile kullanmak için hazırladım. İki oyun aynı Nano kodunu kullanıyor. Adımlar Windows içindir.

## 1. Donanım

Gösterge: **Magneti Marelli C141, 1362894080 / 503.001.210.203M**.
[Pin tablosundaki](PINOUT.md) 1, 2, 3, 5, 6 ve 17'yi kendi düzeneğimde doğruladım.

| Gösterge pini | Bağlantı |
|---:|---|
| 1 | GND / besleme eksi |
| 2 | Sigortalı sürekli +12 V |
| 3 | Kontak +12 V |
| 5 | CAN modülü CAN-L |
| 6 | CAN modülü CAN-H |

Nano, modül ve gösterge ortak GND kullanmalı. 12 V'u Nano GPIO/SPI veya CAN uçlarına vermeyin. Pinleri konnektörün üzerindeki numaradan okuyun; tablo soldan sağa fiziksel sıralama değildir.

| Nano | MCP2515 + TJA1050 |
|---|---|
| D10 | CS |
| D2 | INT |
| D11 | MOSI / SI |
| D12 | MISO / SO |
| D13 | SCK |
| 5 V | 5 V modül VCC |
| GND | GND |

Kristal **8 MHz**, CAN hızı **50 kbit/s**.
**Pin 17'ye +12 V vermeyin:** GND ile direksiyon uyarısını yakar; oyun için gerekli değil. Pin 18'i doğrulamadım, benim soketimde terminal yok. Farklı parça numarası için kendi şemanızı kontrol edin.

## 2. Nano kodunu yükleme

1. GitHub'da **Code > Download ZIP** ile indirip ZIP'i çıkarın.
2. Arduino IDE kart yöneticisinden **Arduino AVR Boards** kurun.
3. Kütüphane yöneticisinden **mcp_can / Cory J. Fowler 1.5.1** kurun.
4. `firmware/Ducato_X250_Game/Ducato_X250_Game.ino` açın.
5. **Arduino Nano / ATmega328P** ve Nano'nun COM portunu seçip yükleyin. Klon Nano yüklenmiyorsa **Old Bootloader** deneyin.
6. Serial Monitor'ü **115200 baud** açın; `BASARILI` mesajını kontrol edin.
7. **Serial Monitor'ü kapatın**, köprü aynı portu kullanacak.

## 3. Python

Python 3 kurun. Deponun ana klasöründe PowerShell açıp çalıştırın:

```powershell
py --version
py -m pip install -r requirements.txt
```

Varsayılan COM8. Gerçek portu Arduino IDE'den öğrenin; aşağıdaki COM5 örneklerini kendi portunuzla değiştirin.

## 4. ETS2

1. [Funbit telemetri sunucusunu](https://github.com/Funbit/ets2-telemetry-server) indirip çıkarın.
2. Paketindeki `server/Ets2Telemetry.exe` açın, **Install** ile kurulumu tamamlayın; kendi yönergelerini izleyin. Sunucu bu depoya dahil değil.
3. Sunucuyu açık bırakın, ETS2'yi açıp sürüşe girin.
4. Tarayıcıda `http://127.0.0.1:25555/api/ets2/telemetry` açın. JSON gelmeli; `game.connected` true olmalı.
5. COM8 için `ETS2.cmd` açın. Başka port için:

```powershell
.\ETS2.cmd COM5
```

Konsolda `game ...` satırları görünür. İki pencere de oyun boyunca açık kalmalı. İnternet sunucusu gerekmez; telemetri aynı bilgisayarda çalışır.

Selektörü sol fare tuşuna atayın. Köprü ETS2 ön plandayken bu tuşu okur. Normal uzun far telemetriden gelir.

## 5. BeamNG

1. BeamNG Launcher'da **Manage User Folder > Open in Explorer** ile aktif kullanıcı klasörünü açın. Menü adı sürüme göre değişebilir; Steam oyun klasörüyle karıştırmayın.
2. İçeride `mods/unpacked/taha_ducato` oluşturun.
3. Depodaki `beamng_mod` klasörünün **içeriğini** buraya kopyalayın. Dosya yolu:

```text
<BeamNG kullanıcı klasörü>/mods/unpacked/taha_ducato/lua/vehicle/protocols/taha_ducato.lua
```

Araya fazladan `beamng_mod` koymayın.
4. Mod görünmüyorsa oyunu yeniden başlatın. **Options/Settings > Other protocols** bölümünde özel protokolü etkinleştirin.
5. Araç yükleyin ve **Ctrl+R** yapın.
6. COM8 için `BeamNG.cmd` açın. Başka port için:

```powershell
.\BeamNG.cmd COM5
```

Mod yerel **127.0.0.1:4568 UDP** adresine veri yollar. Funbit gerekmez. ETS2 köprüsü aynı anda açık olmasın. [BeamNG protokol belgesi](https://documentation.beamng.com/modding/protocols/).

## Sorunlar ve kapatma

- Köprüyü **Ctrl+C** ile kapatın.
- Port açılamıyorsa COM numarasını kontrol edin; Serial Monitor veya diğer köprü açık olabilir.
- ETS2 veri vermiyorsa önce JSON adresini kontrol edin.
- BeamNG veri vermiyorsa modun yolunu, protokol ayarını ve Ctrl+R adımını kontrol edin.
- Nano `HATA` / `LOCK=1` bildiriyorsa bağlantıyı kontrol edip resetleyin.
- Serial Monitor **115200 / Newline**: `status` sayaçları gösterir. `stop` / `allstop` gönderimi keser, fakat açık köprünün sonraki paketi yeniden başlatır. Kalıcı durdurmak için köprüyü kapatın.

## Bildiğim eksikler

BeamNG'de hararet ibresi hâlâ beklenenden aşağıda kalıyor; kalibrasyonu tamamlamadım. Kilometre artışını ve kırmızı hararet lambasının CAN bitini doğrulamadım.

Kendi göstergemde ve baktığım diğer Ducato göstergelerinde ayrı kısa far lambası yok; park/aydınlatma lambası var.

DPF, kemer, airbag ve diğer bazı arıza verilerini her BeamNG aracı göndermiyor. Kontak açılış ışıkları, immo ve kızdırma süreleri simülasyon amaçlı.

Donanıma göndermeden yazılım kontrolü:

```powershell
py bridge/ets2_bridge.py --self-test
py bridge/beamng_bridge.py --self-test
```

Bu, gerçek gösterge testinin yerine geçmez. Ham notlarımı ve tarama araçlarımı paylaşım paketine koymadım.
