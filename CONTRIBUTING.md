# Contributing

Thanks for helping improve my Ducato dashboard project. English and Turkish reports are welcome.

## Before opening an issue

Read the [setup guide](docs/KURULUM_TR.md), [pinout](docs/PINOUT.md), known limits in the [README](README.md), and existing issues. Please use the bug, feature or CAN-finding template. For security vulnerabilities use the private process in [SECURITY.md](SECURITY.md).

## What to include

- Repository commit or release, Windows/Python/Arduino IDE versions, and game version.
- Cluster part number, MCP2515 crystal frequency, module type and wiring.
- Expected behavior, actual behavior, steps to reproduce, and relevant serial output.
- Whether you tested on hardware, in software only, or inferred the result from another source.

Remove personal information, VINs, credentials and unrelated CAN traffic from logs. I do not need your private vehicle history or a complete unredacted capture.

## CAN findings

Use zero-based byte indexes b0..b7 and hex IDs/data. State the ID, DLC, interval, baseline payload, exact changed bits and observed effect. Include cluster identity, repeatability, and whether other frames were running. Keep source-derived information and hypotheses separate from bench observations. Link sources and preserve attribution. Do not claim a result applies to every Ducato variant.

Use an isolated, detached bench setup. Do not connect experimental transmitters to a vehicle in use. Verify numbered connector cavities for your exact part number; never infer power pins from wire colors.

## Pull requests

Keep changes focused and explain the problem, resulting behavior and validation. Preserve fixed hardware settings unless a change explicitly adds a documented configuration: Nano ATmega328P, 8 MHz MCP2515, 50 kbit/s, CS D10 and INT D2.

The public firmware is game-only. Do not add raw senders, scanning tools, private research logs or captured-drive replay. Keep boot TX disabled, bounded buffers, CAN fault handling and telemetry timeouts intact.

Run applicable checks from the repository root:

```powershell
py bridge/ets2_bridge.py --self-test
py bridge/beamng_bridge.py --self-test
```

For firmware changes, compile `firmware/Ducato_X250_Game` for Arduino Nano ATmega328P with mcp_can 1.5.1 and report flash/RAM use. For Lua changes, describe how you checked vehicle reload and the affected signals. State clearly if real hardware or a game was unavailable. Documentation-only changes need link/content checks, not a new firmware build.

By contributing, you agree that your original contributions are provided under this repository's [MIT License](LICENSE). Only submit material you have permission to share; external dependencies keep their own licenses. Please follow the [Code of Conduct](CODE_OF_CONDUCT.md).
