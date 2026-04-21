#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include "TCP1819.h"
#include "TCP1819ScriptedBus.h"

namespace {

BBI2C makeBus(uint8_t sdaPin, uint8_t sclPin)
{
    BBI2C bus{};
    bus.iSDA = sdaPin;
    bus.iSCL = sclPin;
    return bus;
}

void expect(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::abort();
    }
}

void testRoutesCoreOpsAndRecordsOrderedTraffic()
{
    TCP1819ScriptedBus::unbindAll();

    BBI2C bus = makeBus(10U, 11U);
    TCP1819ScriptedBus scriptedBus;
    TCP1819ScriptedBus::bind(bus, scriptedBus);

    scriptedBus.queueTestResult(1);
    scriptedBus.queueWriteCount(2);
    scriptedBus.queueReadVector({0xAAU, 0xBBU, 0xCCU}, 3);

    uint8_t readBuffer[3] = {0U, 0U, 0U};
    uint8_t writeBuffer[3] = {0x01U, 0x02U, 0x03U};

    I2CInit(&bus, 100000UL);
    expect(I2CTest(&bus, 0x68U) == 1U, "I2CTest should return the queued test result");
    expect(I2CWrite(&bus, 0x20U, writeBuffer, 3) == 2, "I2CWrite should return the queued short count");
    expect(I2CRead(&bus, 0x20U, readBuffer, 3) == 3, "I2CRead should return the queued count");

    expect(readBuffer[0] == 0xAAU && readBuffer[1] == 0xBBU && readBuffer[2] == 0xCCU,
           "I2CRead should copy queued bytes into the caller buffer");
    expect(scriptedBus.initCallCount() == 1U, "expected exactly one init call");
    expect(scriptedBus.testCallCount() == 1U, "expected exactly one test call");
    expect(scriptedBus.writeCallCount() == 1U, "expected exactly one write call");
    expect(scriptedBus.readCallCount() == 1U, "expected exactly one read call");
    expect(scriptedBus.operationCount() == 4U, "expected four recorded operations");
    expect(scriptedBus.scriptFullyConsumed(), "expected all queued script to be consumed");

    const TCP1819ScriptedBusOperation &initOp = scriptedBus.operationAt(0U);
    const TCP1819ScriptedBusOperation &testOp = scriptedBus.operationAt(1U);
    const TCP1819ScriptedBusOperation &writeOp = scriptedBus.operationAt(2U);
    const TCP1819ScriptedBusOperation &readOp = scriptedBus.operationAt(3U);

    expect(initOp.kind == TCP1819ScriptedBusOpKind::Init, "first op should be Init");
    expect(initOp.initClockHz == 100000UL, "Init op should record clock");
    expect(testOp.kind == TCP1819ScriptedBusOpKind::Test, "second op should be Test");
    expect(testOp.address == 0x68U, "Test op should record address");
    expect(testOp.returnedCountOrResult == 1, "Test op should record returned result");
    expect(writeOp.kind == TCP1819ScriptedBusOpKind::Write, "third op should be Write");
    expect(writeOp.address == 0x20U, "Write op should record address");
    expect(writeOp.requestedLength == 3, "Write op should record requested length");
    expect(writeOp.returnedCountOrResult == 2, "Write op should record short write count");
    expect(writeOp.bytes == std::vector<uint8_t>({0x01U, 0x02U, 0x03U}),
           "Write op should record exact bytes provided by the caller");
    expect(readOp.kind == TCP1819ScriptedBusOpKind::Read, "fourth op should be Read");
    expect(readOp.address == 0x20U, "Read op should record address");
    expect(readOp.requestedLength == 3, "Read op should record requested length");
    expect(readOp.returnedCountOrResult == 3, "Read op should record returned count");
    expect(readOp.bytes == std::vector<uint8_t>({0xAAU, 0xBBU, 0xCCU}),
           "Read op should record returned bytes in order");
}

void testResetClearsScriptAndObservations()
{
    TCP1819ScriptedBus::unbindAll();

    BBI2C bus = makeBus(12U, 13U);
    TCP1819ScriptedBus scriptedBus;
    TCP1819ScriptedBus::bind(bus, scriptedBus);

    scriptedBus.queueTestResult(0);
    scriptedBus.queueWriteCount(1);
    scriptedBus.queueReadVector({0x55U}, 1);

    uint8_t writeByte = 0xABU;
    uint8_t readByte = 0U;

    expect(I2CTest(&bus, 0x10U) == 0U, "queued test result should be consumed before reset");
    expect(I2CWrite(&bus, 0x10U, &writeByte, 1) == 1, "queued write count should be consumed before reset");
    expect(I2CRead(&bus, 0x10U, &readByte, 1) == 1, "queued read should be consumed before reset");
    expect(scriptedBus.operationCount() == 3U, "sanity check before reset");

    scriptedBus.reset();

    expect(scriptedBus.operationCount() == 0U, "reset should clear observed operations");
    expect(scriptedBus.scriptFullyConsumed(), "reset should clear queued script");
    expect(scriptedBus.pendingTestCount() == 0U, "reset should clear queued tests");
    expect(scriptedBus.pendingWriteCount() == 0U, "reset should clear queued writes");
    expect(scriptedBus.pendingReadCount() == 0U, "reset should clear queued reads");
}

void testMultipleBoundBusesStayIsolated()
{
    TCP1819ScriptedBus::unbindAll();

    BBI2C leftBus = makeBus(2U, 3U);
    BBI2C rightBus = makeBus(4U, 5U);
    TCP1819ScriptedBus leftScriptedBus;
    TCP1819ScriptedBus rightScriptedBus;
    TCP1819ScriptedBus::bind(leftBus, leftScriptedBus);
    TCP1819ScriptedBus::bind(rightBus, rightScriptedBus);

    leftScriptedBus.queueTestResult(1);
    leftScriptedBus.queueReadVector({0x11U}, 1);
    rightScriptedBus.queueTestResult(0);
    rightScriptedBus.queueWriteCount(0);

    uint8_t leftRead = 0U;
    uint8_t rightWrite = 0xEEU;

    expect(I2CTest(&leftBus, 0x30U) == 1U, "left bus should use left scripted test result");
    expect(I2CRead(&leftBus, 0x30U, &leftRead, 1) == 1, "left bus should use left scripted read");
    expect(I2CTest(&rightBus, 0x40U) == 0U, "right bus should use right scripted test result");
    expect(I2CWrite(&rightBus, 0x40U, &rightWrite, 1) == 0, "right bus should use right scripted write count");

    expect(leftRead == 0x11U, "left bus should receive only its queued read byte");
    expect(leftScriptedBus.operationCount() == 2U, "left bus should record only its own operations");
    expect(rightScriptedBus.operationCount() == 2U, "right bus should record only its own operations");
    expect(leftScriptedBus.readCallCount() == 1U && leftScriptedBus.writeCallCount() == 0U,
           "left bus should not observe right-bus writes");
    expect(rightScriptedBus.testCallCount() == 1U && rightScriptedBus.readCallCount() == 0U,
           "right bus should not observe left-bus reads");
}

} // namespace

int main()
{
    testRoutesCoreOpsAndRecordsOrderedTraffic();
    testResetClearsScriptAndObservations();
    testMultipleBoundBusesStayIsolated();
    std::cout << "test_WB_tcp1819_scripted_bus: PASS\n";
    return 0;
}