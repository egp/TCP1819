// extras/host_test/TCP1819ScriptedBus.h v1
#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <vector>

#include "TCP1819.h"

enum class TCP1819ScriptedBusOpKind {
    Init,
    Test,
    Read,
    Write,
};

struct TCP1819ScriptedBusOperation {
    TCP1819ScriptedBusOpKind kind;
    BBI2C *bus;
    uint8_t address;
    uint32_t initClockHz;
    int requestedLength;
    int returnedCountOrResult;
    std::vector<uint8_t> bytes;
};

class TCP1819ScriptedBus {
public:
    TCP1819ScriptedBus();

    void reset();
    void clearScript();
    void clearObservations();

    void setDefaultTestResult(int result);
    void setDefaultReadCount(int count);
    void setDefaultWriteCount(int count);

    void queueTestResult(int result);
    void queueTestResultForAddress(uint8_t address, int result);

    void queueReadVector(const std::vector<uint8_t> &data, int returnedCount);
    void queueReadVectorForAddress(uint8_t address, const std::vector<uint8_t> &data, int returnedCount);

    void queueWriteCount(int returnedCount);
    void queueWriteCountForAddress(uint8_t address, int returnedCount);

    std::size_t initCallCount() const;
    std::size_t testCallCount() const;
    std::size_t readCallCount() const;
    std::size_t writeCallCount() const;

    uint32_t lastInitClockHz() const;
    uint8_t lastTestAddress() const;
    uint8_t lastReadAddress() const;
    uint8_t lastWriteAddress() const;

    std::size_t writeHistoryCount() const;
    const std::vector<uint8_t> &writeBytesAt(std::size_t index) const;
    const std::vector<uint8_t> &readResultBytesAt(std::size_t index) const;

    std::size_t operationCount() const;
    const TCP1819ScriptedBusOperation &operationAt(std::size_t index) const;

    std::size_t pendingTestCount() const;
    std::size_t pendingReadCount() const;
    std::size_t pendingWriteCount() const;
    bool hasPendingScript() const;
    bool scriptFullyConsumed() const;

    static void bind(BBI2C &bus, TCP1819ScriptedBus &scriptedBus);
    static void unbind(BBI2C &bus);
    static void unbindAll();
    static TCP1819ScriptedBus *lookup(BBI2C *bus);

    void handleInit(BBI2C *bus, uint32_t clockHz);
    int handleTest(BBI2C *bus, uint8_t address);
    int handleRead(BBI2C *bus, uint8_t address, uint8_t *data, int requestedLength);
    int handleWrite(BBI2C *bus, uint8_t address, const uint8_t *data, int requestedLength);

private:
    struct AddressedResult {
        bool hasAddress;
        uint8_t address;
        int value;
    };

    struct ReadScriptItem {
        bool hasAddress;
        uint8_t address;
        std::vector<uint8_t> bytes;
        int returnedCount;
    };

    void recordOperation(const TCP1819ScriptedBusOperation &operation);

    const TCP1819ScriptedBusOperation &lastOperationOfKind(TCP1819ScriptedBusOpKind kind) const;
    const TCP1819ScriptedBusOperation &operationOfKindAt(TCP1819ScriptedBusOpKind kind,
                                                         std::size_t index) const;

    static std::map<BBI2C *, TCP1819ScriptedBus *> &bindings();

    int defaultTestResult_;
    int defaultReadCount_;
    int defaultWriteCount_;
    std::deque<AddressedResult> scriptedTestResults_;
    std::deque<ReadScriptItem> scriptedReads_;
    std::deque<AddressedResult> scriptedWriteCounts_;
    std::vector<TCP1819ScriptedBusOperation> operations_;
};
// extras/host_test/TCP1819ScriptedBus.h v1