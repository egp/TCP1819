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
    void queueReadVector(const std::vector<uint8_t> &data, int returnedCount);
    void queueWriteCount(int returnedCount);

    std::size_t initCallCount() const;
    std::size_t testCallCount() const;
    std::size_t readCallCount() const;
    std::size_t writeCallCount() const;

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
    struct ReadScriptItem {
        std::vector<uint8_t> bytes;
        int returnedCount;
    };

    void recordOperation(const TCP1819ScriptedBusOperation &operation);

    static std::map<BBI2C *, TCP1819ScriptedBus *> &bindings();

    int defaultTestResult_;
    int defaultReadCount_;
    int defaultWriteCount_;
    std::deque<int> scriptedTestResults_;
    std::deque<ReadScriptItem> scriptedReads_;
    std::deque<int> scriptedWriteCounts_;
    std::vector<TCP1819ScriptedBusOperation> operations_;
};