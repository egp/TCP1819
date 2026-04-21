# TCP1819

TCP1819 is a compact software-I2C library for caller-selected GPIO pins, with helper functions for bus scans and common-device identification.

This repository is derived from the BitBank Bit Bang I2C library, but the current Arduino-facing API in this repo is centered on `BBI2C`, `I2CInit(...)`, `I2CRead(...)`, `I2CReadRegister(...)`, `I2CWrite(...)`, `I2CTest(...)`, `I2CScan(...)`, `I2CDiscoverDevice(...)`, and `I2CGetDeviceName(...)`.

## Scope

TCP1819 is intentionally low-level and transport-oriented.

It provides:

- software I2C on arbitrary GPIO pins
- a small C-style API usable from Arduino sketches
- bus scan and probe helpers
- common-device identification helpers
- Linux-oriented host support under `linux/`
- host-test scripted-bus support under `extras/host_test/`

It does not provide device-specific wrappers for higher-level sensors or displays. Those belong in libraries layered on top of TCP1819.

## Public API

The public API is built around the `BBI2C` struct plus these functions:

- `I2CInit(...)`
- `I2CTest(...)`
- `I2CRead(...)`
- `I2CReadRegister(...)`
- `I2CWrite(...)`
- `I2CScan(...)`
- `I2CDiscoverDevice(...)`
- `I2CGetDeviceName(...)`

## Arduino usage

Typical Arduino use is:

1. Zero-initialize a `BBI2C` instance.
2. Set `bWire` to `0`.
3. Set `iSDA` and `iSCL` to the caller-selected GPIO pins.
4. Call `I2CInit(...)` with the desired bus clock.
5. Use `I2CTest(...)`, `I2CRead(...)`, `I2CReadRegister(...)`, `I2CWrite(...)`, or `I2CScan(...)` as needed.

For a simple presence test, call `I2CTest(...)` with the 7-bit device address.

For a full scan, allocate a 16-byte map buffer and call `I2CScan(...)`.

## Example sketch

`examples/I2C_Detector/I2C_Detector.ino` is the main Arduino example.

It:

- initializes a TCP1819 bus on caller-selected pins
- scans all 7-bit I2C addresses
- prints responding addresses
- attempts to identify recognized devices
- prints capability flags when available

## Linux host support

The `linux/` directory contains Linux-oriented host support and upstream-derived files used by the host build.

The root `Makefile` builds a host static library at `build/libTCP1819_host.a`.

## Host-test scripted bus support

The `extras/host_test/` directory provides a TCP1819-owned scripted bus for host tests at the BBI2C interface level.

It is intended for host-side unit tests that want to:

- bind a `BBI2C` instance to a scripted bus
- script `I2CTest`, `I2CRead`, and `I2CWrite` outcomes
- inspect exact ordered bus traffic
- reuse one common TCP1819-owned test seam across higher-level libraries

The host tests live under `tests/host/`.

See:

- `extras/host_test/TCP1819ScriptedBus.h`
- `extras/host_test/TCP1819ScriptedBus.cpp`
- `extras/host_test/TCP1819HostShim.cpp`
- `extras/host_test/README.txt`

## Device identification

TCP1819 includes helpers to identify many common I2C devices by probing known registers and response patterns.

Use:

- `I2CDiscoverDevice(...)`
- `I2CGetDeviceName(...)`

This is intended as a convenience feature for diagnostics and detector sketches, not as a substitute for device-specific drivers.

## Repository layout

- `src/` Arduino-facing public header and implementation
- `examples/` Arduino example sketches
- `linux/` Linux-oriented support code
- `extras/host_test/` host-test scripted-bus support
- `tests/host/` TCP1819-owned host tests
- `docs/` design and implementation notes
- `Makefile` host static-library build and host-test entry point
- `library.properties` Arduino library metadata
- `keywords.txt` Arduino IDE keyword highlighting

## CI

The GitHub Actions workflow:

- builds the host library
- runs the host tests
- compiles the Arduino example sketch on:
  - Arduino Uno
  - Arduino Uno R4 Minima
  - Arduino Uno R4 WiFi

## License and attribution

This repository is derived from the BitBank Bit Bang I2C library by Larry Bank.

See `LICENSE` for the governing license in this repository.