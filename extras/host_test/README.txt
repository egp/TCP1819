TCP1819 Host-Test Scripted Bus
==============================

Purpose
-------
The files in this directory provide a TCP1819-owned host-test seam at the BBI2C
interface level.

They are intended for host unit tests that want to:
- bind a BBI2C instance to a scripted bus
- script I2CTest, I2CRead, and I2CWrite behavior
- observe exact bus traffic in chronological order
- test higher-level code without adding a device-specific fake transport

These files are host-test support only. They are not part of the Arduino-facing
production API.

Files
-----
- TCP1819ScriptedBus.h
- TCP1819ScriptedBus.cpp
- TCP1819HostShim.cpp

Core idea
---------
A TCP1819ScriptedBus instance owns:
- queued scripted results for test/read/write operations
- accumulated observed operations
- default behaviors used when no queued script item matches

A host-only binding registry maps:
- BBI2C* -> TCP1819ScriptedBus*

The host shim functions:
- I2CInit(...)
- I2CTest(...)
- I2CRead(...)
- I2CWrite(...)
- I2CReadRegister(...)
- I2CScan(...)

route through the bound scripted bus when one is present.

Minimal usage
-------------
1. Create and bind a BBI2C instance:

    BBI2C bus{};
    bus.iSDA = 10;
    bus.iSCL = 11;

    TCP1819ScriptedBus scriptedBus;
    TCP1819ScriptedBus::bind(bus, scriptedBus);

2. Script responses:

    scriptedBus.queueTestResult(1);
    scriptedBus.queueWriteCount(2);
    scriptedBus.queueReadVector({0xAA, 0xBB, 0xCC}, 3);

3. Call the normal TCP1819 API:

    uint8_t readBuf[3] = {0, 0, 0};
    uint8_t writeBuf[3] = {0x01, 0x02, 0x03};

    I2CInit(&bus, 100000UL);
    int present = I2CTest(&bus, 0x68);
    int written = I2CWrite(&bus, 0x20, writeBuf, 3);
    int read = I2CRead(&bus, 0x20, readBuf, 3);

4. Inspect what happened:

    scriptedBus.operationCount()
    scriptedBus.operationAt(0)
    scriptedBus.lastInitClockHz()
    scriptedBus.lastTestAddress()
    scriptedBus.lastWriteAddress()
    scriptedBus.lastReadAddress()
    scriptedBus.writeHistoryCount()
    scriptedBus.writeBytesAt(0)
    scriptedBus.readResultBytesAt(0)

Address-aware scripting
-----------------------
The scripted bus also supports optional address-aware queue items:

- queueTestResultForAddress(...)
- queueReadVectorForAddress(...)
- queueWriteCountForAddress(...)

If the next queued item is address-aware and the attempted operation uses a
different address, the item is not consumed. The operation falls back to the
configured default behavior and is still recorded in the operation log.

Defaults
--------
Suggested defaults are:
- test default: success-like result 1
- read default: 0 bytes returned
- write default: requested length accepted

These defaults can be changed with:
- setDefaultTestResult(...)
- setDefaultReadCount(...)
- setDefaultWriteCount(...)

Reset and teardown
------------------
Use:
- clearScript()
- clearObservations()
- reset()

and optionally:
- TCP1819ScriptedBus::unbind(...)
- TCP1819ScriptedBus::unbindAll()

The reset() helper returns the scripted bus to a fresh default state.

Notes
-----
This seam is intentionally low-level and bus-oriented.
It does not know about specific sensors, displays, clocks, or register maps
beyond the normal byte traffic implied by the TCP1819 API.

Higher-level host tests may still add thin local adapters if that improves the
readability of device-specific tests.