// tests/host/test_WB_tcp1819_scripted_bus.cpp v3
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
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

template <typename Fn>
void expectOutOfRange(Fn fn, const char *message)
{
    try {
        fn();
    } catch (const std::out_of_range &) {
        return;
    } catch (...) {
        std::cerr << "FAILED: " << message << " (wrong exception type)\n";
        std::abort();
    }
    std::cerr << "FAILED: " << message << " (no exception)\n";
    std::abort();
}

bool mapBitIsSet(const unsigned char *map, unsigned char address)
{
    return (map[address >> 3] & static_cast<unsigned char>(1U << (address & 7U))) != 0U;
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

void testObservationHelpersExposeLastOpsAndSeparatedHistories()
{
    TCP1819ScriptedBus::unbindAll();

    BBI2C bus = makeBus(40U, 41U);
    TCP1819ScriptedBus scriptedBus;
    TCP1819ScriptedBus::bind(bus, scriptedBus);

    scriptedBus.queueTestResult(7);
    scriptedBus.queueWriteCount(2);
    scriptedBus.queueReadVector({0xA1U}, 1);
    scriptedBus.queueWriteCount(1);
    scriptedBus.queueReadVector({0xB2U, 0xB3U}, 2);

    uint8_t firstWrite[3] = {0x10U, 0x11U, 0x12U};
    uint8_t secondWrite[1] = {0x20U};
    uint8_t firstRead[1] = {0U};
    uint8_t secondRead[2] = {0U, 0U};

    I2CInit(&bus, 400000UL);
    expect(I2CTest(&bus, 0x33U) == 7U, "queued test result should be returned");
    expect(I2CWrite(&bus, 0x20U, firstWrite, 3) == 2, "first queued write count should be used");
    expect(I2CRead(&bus, 0x21U, firstRead, 1) == 1, "first queued read should be used");
    expect(I2CWrite(&bus, 0x22U, secondWrite, 1) == 1, "second queued write count should be used");
    expect(I2CRead(&bus, 0x23U, secondRead, 2) == 2, "second queued read should be used");

    expect(scriptedBus.lastInitClockHz() == 400000UL, "last init clock helper should report the latest init");
    expect(scriptedBus.lastTestAddress() == 0x33U, "last test address helper should report the latest test");
    expect(scriptedBus.lastWriteAddress() == 0x22U, "last write address helper should report the latest write");
    expect(scriptedBus.lastReadAddress() == 0x23U, "last read address helper should report the latest read");

    expect(scriptedBus.writeHistoryCount() == 2U, "write history count should match the number of writes");
    expect(scriptedBus.writeBytesAt(0U) == std::vector<uint8_t>({0x10U, 0x11U, 0x12U}),
           "writeBytesAt(0) should return the first write payload");
    expect(scriptedBus.writeBytesAt(1U) == std::vector<uint8_t>({0x20U}),
           "writeBytesAt(1) should return the second write payload");
    expect(scriptedBus.readResultBytesAt(0U) == std::vector<uint8_t>({0xA1U}),
           "readResultBytesAt(0) should return the first read result bytes");
    expect(scriptedBus.readResultBytesAt(1U) == std::vector<uint8_t>({0xB2U, 0xB3U}),
           "readResultBytesAt(1) should return the second read result bytes");
}

void testObservationHelpersThrowWhenHistoryIsMissing()
{
    TCP1819ScriptedBus::unbindAll();

    BBI2C bus = makeBus(42U, 43U);
    TCP1819ScriptedBus scriptedBus;
    TCP1819ScriptedBus::bind(bus, scriptedBus);

    expectOutOfRange([&]() { (void)scriptedBus.lastInitClockHz(); },
                     "lastInitClockHz should throw when no init has been recorded");
    expectOutOfRange([&]() { (void)scriptedBus.lastTestAddress(); },
                     "lastTestAddress should throw when no test has been recorded");
    expectOutOfRange([&]() { (void)scriptedBus.writeBytesAt(0U); },
                     "writeBytesAt should throw when no writes have been recorded");
    expectOutOfRange([&]() { (void)scriptedBus.readResultBytesAt(0U); },
                     "readResultBytesAt should throw when no reads have been recorded");
}

void testAddressAwareScriptsFallBackWithoutConsumptionOnMismatch()
{
    TCP1819ScriptedBus::unbindAll();

    BBI2C bus = makeBus(12U, 13U);
    TCP1819ScriptedBus scriptedBus;
    TCP1819ScriptedBus::bind(bus, scriptedBus);

    scriptedBus.setDefaultTestResult(9);
    scriptedBus.setDefaultReadCount(0);
    scriptedBus.setDefaultWriteCount(0);

    scriptedBus.queueTestResultForAddress(0x68U, 1);
    scriptedBus.queueReadVectorForAddress(0x68U, {0xABU}, 1);
    scriptedBus.queueWriteCountForAddress(0x20U, 2);

    uint8_t wrongRead = 0xEEU;
    uint8_t wrongWrite[3] = {0x01U, 0x02U, 0x03U};

    expect(I2CTest(&bus, 0x69U) == 9U, "wrong-address test should fall back to default result");
    expect(scriptedBus.pendingTestCount() == 1U, "wrong-address test should not consume queued item");

    expect(I2CRead(&bus, 0x69U, &wrongRead, 1) == 0, "wrong-address read should fall back to default count");
    expect(wrongRead == 0xEEU, "wrong-address read should not touch caller buffer");
    expect(scriptedBus.pendingReadCount() == 1U, "wrong-address read should not consume queued item");

    expect(I2CWrite(&bus, 0x21U, wrongWrite, 3) == 0, "wrong-address write should fall back to default count");
    expect(scriptedBus.pendingWriteCount() == 1U, "wrong-address write should not consume queued item");

    uint8_t matchedRead = 0U;
    expect(I2CTest(&bus, 0x68U) == 1U, "matching-address test should consume queued item");
    expect(I2CRead(&bus, 0x68U, &matchedRead, 1) == 1, "matching-address read should consume queued item");
    expect(matchedRead == 0xABU, "matching-address read should return queued byte");
    expect(I2CWrite(&bus, 0x20U, wrongWrite, 3) == 2, "matching-address write should consume queued item");
    expect(scriptedBus.scriptFullyConsumed(), "all address-aware items should now be consumed");

    expect(scriptedBus.operationCount() == 6U, "all attempts should still be recorded");
}

void testReadRegisterUsesWriteThenReadSequenceAndReturnsSuccessFlag()
{
    TCP1819ScriptedBus::unbindAll();

    BBI2C bus = makeBus(20U, 21U);
    TCP1819ScriptedBus scriptedBus;
    TCP1819ScriptedBus::bind(bus, scriptedBus);

    scriptedBus.queueWriteCount(1);
    scriptedBus.queueReadVector({0x12U, 0x34U}, 2);

    unsigned char buffer[2] = {0U, 0U};
    expect(I2CReadRegister(&bus, 0x68U, 0x0FU, buffer, 2) == 1,
           "I2CReadRegister should return 1 on success");
    expect(buffer[0] == 0x12U && buffer[1] == 0x34U,
           "I2CReadRegister should copy read bytes into the caller buffer");
    expect(scriptedBus.operationCount() == 2U, "I2CReadRegister should record one write and one read");

    const TCP1819ScriptedBusOperation &writeOp = scriptedBus.operationAt(0U);
    const TCP1819ScriptedBusOperation &readOp = scriptedBus.operationAt(1U);

    expect(writeOp.kind == TCP1819ScriptedBusOpKind::Write, "first register-read op should be Write");
    expect(writeOp.address == 0x68U, "register pointer write should target requested address");
    expect(writeOp.requestedLength == 1, "register pointer write should be length 1");
    expect(writeOp.returnedCountOrResult == 1, "register pointer write should report one byte written");
    expect(writeOp.bytes == std::vector<uint8_t>({0x0FU}),
           "register pointer write should contain the register byte");

    expect(readOp.kind == TCP1819ScriptedBusOpKind::Read, "second register-read op should be Read");
    expect(readOp.address == 0x68U, "register data read should target requested address");
    expect(readOp.requestedLength == 2, "register data read should request the requested read length");
    expect(readOp.returnedCountOrResult == 2, "register data read should still record the queued count");
    expect(readOp.bytes == std::vector<uint8_t>({0x12U, 0x34U}),
           "register data read should record returned bytes");
}

void testReadRegisterReturnsSuccessForShortPositiveRead()
{
    TCP1819ScriptedBus::unbindAll();

    BBI2C bus = makeBus(26U, 27U);
    TCP1819ScriptedBus scriptedBus;
    TCP1819ScriptedBus::bind(bus, scriptedBus);

    scriptedBus.queueWriteCount(1);
    scriptedBus.queueReadVector({0x77U}, 1);

    unsigned char buffer[2] = {0U, 0U};
    expect(I2CReadRegister(&bus, 0x68U, 0x02U, buffer, 2) == 1,
           "I2CReadRegister should return success when the read count is positive");
    expect(buffer[0] == 0x77U, "short successful register read should still copy returned data");
    expect(scriptedBus.operationCount() == 2U, "short successful register read should still perform write+read");
}

void testReadRegisterAbortsWhenPointerWriteFails()
{
    TCP1819ScriptedBus::unbindAll();

    BBI2C bus = makeBus(22U, 23U);
    TCP1819ScriptedBus scriptedBus;
    TCP1819ScriptedBus::bind(bus, scriptedBus);

    scriptedBus.queueWriteCount(0);
    scriptedBus.queueReadVector({0x99U}, 1);

    unsigned char buffer[1] = {0x55U};
    expect(I2CReadRegister(&bus, 0x68U, 0x01U, buffer, 1) == 0,
           "I2CReadRegister should fail when the register pointer write fails");
    expect(buffer[0] == 0x55U, "failed register read should leave caller buffer unchanged");
    expect(scriptedBus.operationCount() == 1U, "failed register read should not attempt the data read");
    expect(scriptedBus.pendingReadCount() == 1U, "failed register read should leave queued read untouched");
    expect(scriptedBus.operationAt(0U).kind == TCP1819ScriptedBusOpKind::Write,
           "failed register read should record only the pointer write");
}

void testReadRegisterFailsWhenReadReturnsZero()
{
    TCP1819ScriptedBus::unbindAll();

    BBI2C bus = makeBus(28U, 29U);
    TCP1819ScriptedBus scriptedBus;
    TCP1819ScriptedBus::bind(bus, scriptedBus);

    scriptedBus.queueWriteCount(1);
    scriptedBus.queueReadVector({}, 0);

    unsigned char buffer[1] = {0x44U};
    expect(I2CReadRegister(&bus, 0x68U, 0x03U, buffer, 1) == 0,
           "I2CReadRegister should fail when the data read returns zero");
    expect(buffer[0] == 0x44U, "zero-length register read should leave caller buffer unchanged");
    expect(scriptedBus.operationCount() == 2U, "zero-length register read should still record write+read");
}

void testScanBuildsBitmapFromTestResponses()
{
    TCP1819ScriptedBus::unbindAll();

    BBI2C bus = makeBus(24U, 25U);
    TCP1819ScriptedBus scriptedBus;
    TCP1819ScriptedBus::bind(bus, scriptedBus);

    scriptedBus.setDefaultTestResult(0);
    scriptedBus.queueTestResultForAddress(0x10U, 1);
    scriptedBus.queueTestResultForAddress(0x68U, 1);

    unsigned char map[16];
    I2CScan(&bus, map);

    expect(mapBitIsSet(map, 0x10U), "scan bitmap should contain address 0x10");
    expect(mapBitIsSet(map, 0x68U), "scan bitmap should contain address 0x68");
    expect(!mapBitIsSet(map, 0x11U), "scan bitmap should not contain unmatched address 0x11");
    expect(!mapBitIsSet(map, 0x67U), "scan bitmap should not contain unmatched address 0x67");
    expect(scriptedBus.testCallCount() == 127U, "scan should test every address from 1 through 127");
    expect(scriptedBus.operationCount() == 127U, "scan should record one Test op per address");
    expect(scriptedBus.scriptFullyConsumed(), "scan should consume matching queued test results in order");
}

void testResetClearsScriptAndObservations()
{
    TCP1819ScriptedBus::unbindAll();

    BBI2C bus = makeBus(30U, 31U);
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
    testObservationHelpersExposeLastOpsAndSeparatedHistories();
    testObservationHelpersThrowWhenHistoryIsMissing();
    testAddressAwareScriptsFallBackWithoutConsumptionOnMismatch();
    testReadRegisterUsesWriteThenReadSequenceAndReturnsSuccessFlag();
    testReadRegisterReturnsSuccessForShortPositiveRead();
    testReadRegisterAbortsWhenPointerWriteFails();
    testReadRegisterFailsWhenReadReturnsZero();
    testScanBuildsBitmapFromTestResponses();
    testResetClearsScriptAndObservations();
    testMultipleBoundBusesStayIsolated();
    std::cout << "test_WB_tcp1819_scripted_bus: PASS\n";
    return 0;
}
// tests/host/test_WB_tcp1819_scripted_bus.cpp v3