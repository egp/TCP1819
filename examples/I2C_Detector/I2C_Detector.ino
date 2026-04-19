#include <string.h>
#include <TCP1819.h>

namespace {
static const uint8_t kSdaPin = 10;
static const uint8_t kSclPin = 11;
static const uint32_t kClockHz = 100000UL;
static const uint32_t kRescanDelayMs = 5000UL;

BBI2C i2c;

void setupBus() {
  memset(&i2c, 0, sizeof(i2c));
  i2c.bWire = 0;
  i2c.iSDA = kSdaPin;
  i2c.iSCL = kSclPin;
  I2CInit(&i2c, kClockHz);
}

void printHexByte(uint8_t value) {
  if (value < 0x10) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}
}  // namespace

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000UL) {
    delay(10);
  }

  setupBus();
  delay(100);

  Serial.println();
  Serial.println(F("TCP1819 I2C_Detector"));
  Serial.print(F("SDA pin: "));
  Serial.println(kSdaPin);
  Serial.print(F("SCL pin: "));
  Serial.println(kSclPin);
  Serial.print(F("Clock: "));
  Serial.println(kClockHz);
  Serial.println();
}

void loop() {
  uint8_t map[16];
  char deviceName[32];
  int deviceCount = 0;

  Serial.println(F("Starting I2C scan"));
  I2CScan(&i2c, map);

  if (map[0] == 0xfe) {
    Serial.println(F("I2C scan failed: check pins, pull-ups, and attached devices"));
    Serial.println();
    delay(kRescanDelayMs);
    return;
  }

  for (uint8_t addr = 1; addr < 128; ++addr) {
    const uint8_t mask = static_cast<uint8_t>(1U << (addr & 7));
    if ((map[addr >> 3] & mask) == 0) {
      continue;
    }

    ++deviceCount;

    uint32_t capabilities = 0;
    const int device = I2CDiscoverDevice(&i2c, addr, &capabilities);
    if (device >= 0) {
      I2CGetDeviceName(device, deviceName);
    } else {
      strcpy(deviceName, "Unknown");
    }

    Serial.print(F("0x"));
    printHexByte(addr);
    Serial.print(F("  "));
    Serial.print(deviceName);

    if (capabilities != 0) {
      Serial.print(F("  caps=0x"));
      Serial.print(capabilities, HEX);
    }

    Serial.println();
  }

  if (deviceCount == 0) {
    Serial.println(F("No I2C devices detected"));
  } else {
    Serial.print(F("Total devices found: "));
    Serial.println(deviceCount);
  }

  Serial.println();
  delay(kRescanDelayMs);
}