# Fiat Ducato X250 CAN Dashboard

I built this project to use my 2009 Ducato X250 cluster with ETS2 and BeamNG.

[Turkish setup](docs/KURULUM_TR.md) | [Connector pinout](docs/PINOUT.md) | [CAN messages](docs/CAN_MESSAGES.md)

## Hardware

Nano ATmega328P + MCP2515/TJA1050, **8 MHz crystal, 50 kbit/s B-CAN**.
CS D10, INT D2, MOSI D11, MISO D12, SCK D13. Serial: 115200 baud.

My cluster is **Magneti Marelli C141, 1362894080 / 503.001.210.203M**.
I verified pins 1 GND, 2 permanent +12 V, 3 ignition +12 V, 5 CAN-L, 6 CAN-H and 17 power-steering warning.
**Pin 17 is activated by GND; never apply +12 V.** Pin 18 is documented but I have not verified it; my socket has no terminal there.
Use the [pin table](docs/PINOUT.md), molded connector numbers, fused supply and common ground. A connector orientation photo is still pending. The table is not a physical left-to-right drawing. For a detached bench cluster only.

## Shared installation

1. Download **Code > Download ZIP** and extract it.
2. In Arduino IDE install **Arduino AVR Boards** and **mcp_can by Cory J. Fowler, 1.5.1**.
3. Open `firmware/Ducato_X250_Game/Ducato_X250_Game.ino`. Select Nano / ATmega328P and its COM port; upload. If a clone cannot upload, try **Old Bootloader**.
4. Check the successful startup message at 115200 baud, then **close Serial Monitor**.
5. Install Python 3 for Windows. In PowerShell opened in the repository root:

```powershell
py --version
py -m pip install -r requirements.txt
```

Both games use the same firmware. COM8 is the default port; the examples below override it with COM5. Run only one bridge at a time.

## ETS2

1. Download and extract [Funbit ETS2 Telemetry Server](https://github.com/Funbit/ets2-telemetry-server).
2. Run its `server/Ets2Telemetry.exe`, click **Install**, and follow its instructions.
3. Keep the server open, start ETS2 and enter a driving session.
4. Open `http://127.0.0.1:25555/api/ets2/telemetry`: JSON should appear and `game.connected` should be true.
5. Open `ETS2.cmd` for COM8 or run:

```powershell
.\ETS2.cmd COM5
```

Keep both windows open while playing. This uses localhost, not a remote internet server. Funbit binaries are not bundled. Bind flash-to-pass to left mouse click for the Windows workaround; normal high beams use telemetry.

## BeamNG

1. Open the active user folder through the BeamNG Launcher's **Manage User Folder / Open in Explorer** option (labels may vary by version).
2. Create `mods/unpacked/taha_ducato` in it.
3. Copy the **contents** of this repository's `beamng_mod` folder there. Final path:

```text
<BeamNG user folder>/mods/unpacked/taha_ducato/lua/vehicle/protocols/taha_ducato.lua
```

Do not add an extra `beamng_mod` directory.
4. Restart the game if needed to discover the mod. Enable the custom protocol under **Options/Settings > Other protocols**, load a vehicle and press **Ctrl+R**.
5. Open `BeamNG.cmd` for COM8 or run:

```powershell
.\BeamNG.cmd COM5
```

UDP is local **127.0.0.1:4568**, at up to 20 Hz. No Funbit server is needed. I used BeamNG 0.38.6 files; see [BeamNG protocol documentation](https://documentation.beamng.com/modding/protocols/).

## Features and limits

RPM, speed, fuel, temperature, turns/hazards, parking/high-beam/fog states, parking brake, cruise, oil, battery, ABS, airbag and seatbelt status. BeamNG also supplies available mechanical damage and door state.

DPF, water-in-fuel, brake-pad, glow-fault and ongoing SRS/belt warnings require vehicle-provided telemetry. Not every vehicle supplies it. Startup lamp checks, glow timing and immobilizer animation are simulation approximations.

**BeamNG temperature needle still sits lower than expected; I have not finished its calibration.** I have not established odometer increment support or the red coolant-warning bit. My cluster and other Ducato clusters I inspected have no separate dipped-beam lamp: the green indicator is for parking/exterior lights.

CAN display behavior does not replace physical ignition wiring. Gauge calibration is specific to my cluster.

## Stop and troubleshoot

Press **Ctrl+C** in the bridge window. Serial Monitor and two bridges cannot share a COM port. For missing ETS2 data check the JSON endpoint; for BeamNG check the mod path, protocol setting and vehicle reload.

Firmware accepts **game packets, stop/allstop and status only**. No sender, scanner or replay tools are included. It sends nothing at boot. Missing data zeros speed/RPM after one second and stops TX after three seconds. A valid packet resumes after `stop`; close the bridge to keep TX stopped. CAN faults latch a stop; check wiring and reset Nano.

## Validation

Game-only firmware compiled: **8,878 bytes flash, 508 bytes RAM**. Bridge checks pass:

```powershell
py bridge/ets2_bridge.py --self-test
py bridge/beamng_bridge.py --self-test
```

I used the original setup on real hardware with both games. The extracted public firmware has compiled but has not had a separate hardware run.

## Credits

[Cory Fowler MCP_CAN_lib](https://github.com/coryjfowler/MCP_CAN_lib),
[Ducato WiCAN research](https://github.com/danderik88/fiat-ducato-x250-wican),
[Peugeot Boxer research](https://github.com/stav242/esphome-canbus-peugeot-boxer),
[Funbit telemetry server](https://github.com/Funbit/ets2-telemetry-server).

I kept my raw notes and research tools private. No affiliation with Fiat, SCS Software or BeamNG.
