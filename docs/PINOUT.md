# Cluster connector pinout

**Cluster:** Magneti Marelli C141  
**Part numbers:** 1362894080 / 503.001.210.203M

I verified pins **1, 2, 3, 5, 6 and 17** on my bench cluster. This is specific to the identified hardware; verify your own part number and the molded connector cavity numbers before connecting anything.

| Pin | Function | Evidence / notes |
|---:|---|---|
| 1 | GND (-) | Bench verified; negative supply / chassis ground |
| 2 | +12 V, Terminal 30 | Bench verified; permanent battery supply |
| 3 | +12 V, INT | Bench verified; ignition supply |
| 5 | CAN-L | Bench verified; B-CAN Low |
| 6 | CAN-H | Bench verified; B-CAN High |
| 17 | Power steering warning | Bench verified; active LOW, activated by GND. **Do not apply +12 V.** |
| 18 | ECU / EOBD MIL warning | Listed in a service document as a GND-active engine-warning input. **Not verified:** my connector has no physical terminal at this position. |

## Connector orientation

Use the **pin numbers molded into the connector**, not wire colors or an assumed left-to-right order. An annotated connector photograph and viewing direction have not yet been published. This table is a numbered function list, **not a spatial connector drawing**; socket and plug views may be mirrored.

## Bench setup

12 V DC cluster supply, fused appropriately; Nano/controller and cluster share a ground. Keep 12 V away from Nano GPIO and SPI. B-CAN uses **50 kbit/s, standard 11-bit IDs**. The tested interface is Arduino Nano ATmega328P with MCP2515 **8 MHz** and TJA1050; CS D10, INT D2, MOSI D11, MISO D12, SCK D13. Module CAN-L goes to cluster pin 5; CAN-H goes to pin 6.

Pin 17 is a separate wired lamp input, not a newly decoded CAN bit. Pin 18 is not part of the verified wiring or game implementation. The underlying service-document reference for pin 18 has not yet been supplied in this repository.
