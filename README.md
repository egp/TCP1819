# TCP1819

TCP1819 is a compact software-I2C library for caller-selected GPIO pins, with helper functions for bus scans and common-device identification.

This repository is derived from the BitBank Bit Bang I2C library, but the current Arduino-facing API in this repo is centered on:

- `BBI2C`
- `I2CInit(...)`
- `I2CRead(...)`
- `I2CReadRegister(...)`
- `I2CWrite(...)`
- `I2CTest(...)`
- `I2CScan(...)`
- `I2CDiscoverDevice(...)`
- `I2CGetDeviceName(...)`

## Scope

TCP1819 is intentionally low-level and transport-oriented.

It provides:

- software I2C on arbitrary GPIO pins
- a small C-style API usable from Arduino sketches
- bus scan and probe helpers
- common-device identification helpers
- Linux-oriented host support under `linux/`

It does not provide device-specific wrappers for higher-level sensors or displays. Those belong in libraries layered on top of TCP1819.

## Public API

typedef struct mybbi2c
{
  unsigned char iSDA, iSCL;      // pin numbers
  unsigned char bWire, bAlign;   // bWire retained in the struct layout
  unsigned char iSDABit, iSCLBit;
  uint32_t iDelay;
  ...
} BBI2C;

int I2CRead(BBI2C *pI2C, uint8_t iAddr, uint8_t *pData, int iLen);
int I2CReadRegister(BBI2C *pI2C, unsigned char iAddr, unsigned char u8Register, unsigned char *pData, int iLen);
int I2CWrite(BBI2C *pI2C, unsigned char iAddr, unsigned char *pData, int iLen);
uint8_t I2CTest(BBI2C *pI2C, unsigned char addr);
void I2CScan(BBI2C *pI2C, unsigned char *pMap);
void I2CInit(BBI2C *pI2C, uint32_t iClock);
int I2CDiscoverDevice(BBI2C *pI2C, unsigned char iAddr, uint32_t *pCapabilities);
void I2CGetDeviceName(int iDevice, char *szName);

## Arduino usage

Minimal setup:

#include <string.h>
#include <TCP1819.h>

namespace {
static const uint8_t kSdaPin = 10;
static const uint8_t kSclPin = 11;
static const uint32_t kClockHz = 100000UL;

BBI2C i2c;
}

void setup() {
  memset(&i2c, 0, sizeof(i2c));
  i2c.bWire = 0;
  i2c.iSDA = kSdaPin;
  i2c.iSCL = kSclPin;
  I2CInit(&i2c, kClockHz);
}

void loop() {
}

For a simple presence test:

if (I2CTest(&i2c, 0x68)) {
  // device responded at address 0x68
}

For a full scan:

unsigned char map[16];
I2CScan(&i2c, map);

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

The root `Makefile` builds a host static library:

make clean
make

Output:

build/libTCP1819_host.a

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
- `Makefile` host static-library build
- `library.properties` Arduino library metadata
- `keywords.txt` Arduino IDE keyword highlighting

## CI

The GitHub Actions workflow builds the host library and compiles the Arduino example sketch on:

- Arduino Uno
- Arduino Uno R4 Minima
- Arduino Uno R4 WiFi

## License and attribution

This repository is derived from the BitBank Bit Bang I2C library by Larry Bank.

See `LICENSE` for the governing license in this repository.