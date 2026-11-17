# CAN message reference - tested 2009 Ducato X250 cluster

Standard 11-bit IDs, 50 kbit/s, Classical CAN. Hexadecimal values throughout except explicitly marked decimal. Byte indexes are **zero-based b0...b7**. These are behaviors I observed on the cluster, not a complete OEM database. Preserve unrelated fields when changing flags.

## Frames used by the game firmware

| ID | DLC | Period (ms) | Purpose |
|---|---:|---:|---|
| 281 | 8 | 50 | RPM, temperature, engine warnings, cruise |
| 2A0 | 4 | 100 | Speed |
| 380 | 8 | 250 | Body status, illumination, fuel, parking brake |
| 180 | 6 | 50 | Exterior-light indicators and turn indicators |
| 286 | 8 | 100 | ABS/brake status |
| 39A | 8 | 250 | Airbag and seatbelt status |
| 3C0 | 8 | 250 | Battery indicator (tested DLC8; captures elsewhere use DLC4) |

## 281 - baseline `00 00 80 38 01 00 00 00`

| Field | Observed behavior |
|---|---|
| b5 low, b6 high | RPM = little-endian uint16 / 8. 1000 RPM: b5=40, b6=1F |
| b3 | Temperature field, nominal raw = temperature C + 40 decimal; display-specific calibration |
| b1 mask 02 | Cruise indicator |
| b1 mask 08 | Flashing glow warning |
| b1 mask 10 | Steady glow indicator |
| b1 mask 20 | Water-in-fuel symbol |
| b1 mask 40 | General warning triangle, without FPS ON |
| b1 mask 80 | Steady red oil indicator |
| b7 01 or 02 | Blinking red oil indicator; precise distinction unresolved |
| b7 mask 80 | DPF indicator (bench observation) |

RPM example: `00 00 80 38 01 40 1F 00`. Temperature upper position used here is b3=9C (156 decimal). BeamNG's visual mapping sends this value at 100 C game temperature. **Known unresolved issue:** the actual needle still sits lower than expected during gameplay; the scaling change has not been confirmed to fix the physical display. Do not interpret that mapping as the real ECU coolant threshold.

## 2A0 - speed, baseline `00 00 00 C3`

Research reference encoding: big-endian b0:b1 / 16. That alone did not produce an accurate needle on my cluster. A valid continuing 380 body frame was needed. My display calibration is:

| Desired km/h | Raw decimal | b0 b1 |
|---:|---:|---|
| 0 | 0 | 00 00 |
| 40 | 560 | 02 30 |
| about 50 | 720 | 02 D0 |
| 100 | 1504 | 05 E0 |
| 180 | 2800 | 0A F0 |

The bridge interpolates between these points; the 180 endpoint is provisional. Uncorrected values showed about +5 km/h at several bench points. This is a gauge calibration, not a proven universal Fiat speed formula. 286 alone turned off ABS but did not drive the speed needle. Odometer behavior remains unresolved.

## 380 - baseline `08 00 40 59 00 32 0B 04`

| Field | Observed behavior |
|---|---|
| b0 mask 08 | Dial/needle illumination enabled; clearing it left LCD available. Not proof of ignition control |
| b0 mask 20 | Parking-brake indicator; b0=28 keeps illumination on |
| b0 mask 80 | Brake-pad warning |
| b1 nonzero door masks | Door warning; firmware uses 0C. Individual door identification unresolved |
| b2=C0 | Immobilizer warning observed; exact status encoding unresolved |
| b3 mask 80 | Triangle with LCD FPS ON; preserve baseline lower bits |
| b4 mask 02 | Low-fuel/reserve warning |
| b5 | Fuel gauge; 00 empty command, 32 half, 60 full on tested cluster. Higher values can overshoot |
| b6=0A versus baseline 0B | Immobilizer indication; alternating these simulates startup flashing |

An all-zero 380 payload caused blanking/reinitialization-like behavior in bench tests. Maintain a meaningful body baseline. b2/b3/b6 contain unresolved status fields; do not treat every toggled bit as an independent lamp.

## 180 - baseline `00 00 00 00 00 00`

| Field | Observed behavior |
|---|---|
| b1 mask 04 | Front fog, green |
| b1 mask 10 | High beam, blue |
| b1 mask 20 | Parking/exterior lights, green |
| b1 mask 08 | Low-beam state field used by the firmware; does **not** light a separate dipped-beam indicator |
| b2 mask 40 | Left turn indicator (verified in-game) |
| b2 mask 20 | Right turn indicator (verified in-game) |

My bench cluster and the other Ducato clusters I inspected have **no separate dipped-beam indicator**; they have the parking/exterior-light indicator. Do not describe mask 08 as a dedicated low-beam lamp command. This observation does not establish compatibility with every cluster variant.

Both turn masks give hazards. Follow the game's actual flash phase. Rear-fog encoding was not independently confirmed here.

## 286 - baseline `00 00 00 00 08 DB 08 7F`

This baseline turned both ABS and round brake warnings off. Setting b1 mask 20 (`00 20 00 00 08 DB 08 7F`) turned ABS on while leaving brake warning off. Parking brake is independently controlled through 380 b0 mask20. Unknown remaining fields are preserved, not decoded.

## 39A - baseline eight zero bytes

| Field | Observed behavior |
|---|---|
| b0=80 | Steady airbag indicator |
| b0=40 | Fast flashing airbag indicator |
| b2=01 | Seatbelt indicator |

Zero baseline turned both off in bench tests. Combining flags is implemented but vehicle telemetry availability is separate from cluster support.

## 3C0

Eight zero bytes turned the battery warning off; b0=40 with the remaining seven bytes zero turned it on. DLC8 is the bench-tested implementation; captured DLC4 is not asserted equivalent.

## Cluster transmit IDs

`3C3, 603, 643, 663, 683, 6E3, 703` were received from the isolated cluster. The game firmware never transmits these IDs.

## Evidence and limitations

I observed these gauges and lamps on my bench setup. Startup sequencing, immobilizer animation and game-to-lamp fault selection are simulation choices. No confirmed red coolant-warning bit, odometer command, complete checksum/counter model or universal compatibility across cluster variants is claimed. CAN acknowledgment confirms reception at bus level, not the intended dashboard meaning.

Initial references: [Ducato WiCAN research](https://github.com/danderik88/fiat-ducato-x250-wican), [Peugeot Boxer research](https://github.com/stav242/esphome-canbus-peugeot-boxer). This table records the public distilled results; private experiment logs and captured-drive sequences are omitted.
