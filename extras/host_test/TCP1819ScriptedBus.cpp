#include "TCP1819ScriptedBus.h"

#include <algorithm>
#include <stdexcept>

namespace {
constexpr int kDefaultRequestedLength = -1;
}

TCP1819ScriptedBus::TCP1819ScriptedBus()
    : defaultTestResult_(1),
      defaultReadCount_(0),
      defaultWriteCount_(kDefaultRequestedLength) {}

void TCP1819ScriptedBus::reset()
{
    clearScript();
    clearObservations();
    defaultTestResult_ = 1;
    defaultReadCount_ = 0;
    defaultWriteCount_ = kDefaultRequestedLength;
}

void TCP1819ScriptedBus::clearScript()
{
    scriptedTestResults_.clear();
    scriptedReads_.clear();
    scriptedWriteCounts_.clear();
}

void TCP1819ScriptedBus::clearObservations()
{
    operations_.clear();
}

void TCP1819ScriptedBus::setDefaultTestResult(int result)
{
    defaultTestResult_ = result;
}

void TCP1819ScriptedBus::setDefaultReadCount(int count)
{
    if (count < 0) {
        throw std::invalid_argument("default read count must be non-negative");
    }
    defaultReadCount_ = count;
}

void TCP1819ScriptedBus::setDefaultWriteCount(int count)
{
    if (count < 0 && count != kDefaultRequestedLength) {
        throw std::invalid_argument("default write count must be non-negative or -1");
    }
    defaultWriteCount_ = count;
}

void TCP1819ScriptedBus::queueTestResult(int result)
{
    scriptedTestResults_.push_back(result);
}

void TCP1819ScriptedBus::queueReadVector(const std::vector<uint8_t> &data, int returnedCount)
{
    if (returnedCount < 0) {
        throw std::invalid_argument("returned read count must be non-negative");
    }
    if (static_cast<std::size_t>(returnedCount) > data.size()) {
        throw std::invalid_argument("returned read count cannot exceed queued bytes");
    }
    scriptedReads_.push_back(ReadScriptItem{data, returnedCount});
}

void TCP1819ScriptedBus::queueWriteCount(int returnedCount)
{
    if (returnedCount < 0) {
        throw std::invalid_argument("returned write count must be non-negative");
    }
    scriptedWriteCounts_.push_back(returnedCount);
}

std::size_t TCP1819ScriptedBus::initCallCount() const
{
    return std::count_if(
        operations_.begin(), operations_.end(), [](const TCP1819ScriptedBusOperation &operation) {
            return operation.kind == TCP1819ScriptedBusOpKind::Init;
        });
}

std::size_t TCP1819ScriptedBus::testCallCount() const
{
    return std::count_if(
        operations_.begin(), operations_.end(), [](const TCP1819ScriptedBusOperation &operation) {
            return operation.kind == TCP1819ScriptedBusOpKind::Test;
        });
}

std::size_t TCP1819ScriptedBus::readCallCount() const
{
    return std::count_if(
        operations_.begin(), operations_.end(), [](const TCP1819ScriptedBusOperation &operation) {
            return operation.kind == TCP1819ScriptedBusOpKind::Read;
        });
}

std::size_t TCP1819ScriptedBus::writeCallCount() const
{
    return std::count_if(
        operations_.begin(), operations_.end(), [](const TCP1819ScriptedBusOperation &operation) {
            return operation.kind == TCP1819ScriptedBusOpKind::Write;
        });
}

std::size_t TCP1819ScriptedBus::operationCount() const
{
    return operations_.size();
}

const TCP1819ScriptedBusOperation &TCP1819ScriptedBus::operationAt(std::size_t index) const
{
    return operations_.at(index);
}

std::size_t TCP1819ScriptedBus::pendingTestCount() const
{
    return scriptedTestResults_.size();
}

std::size_t TCP1819ScriptedBus::pendingReadCount() const
{
    return scriptedReads_.size();
}

std::size_t TCP1819ScriptedBus::pendingWriteCount() const
{
    return scriptedWriteCounts_.size();
}

bool TCP1819ScriptedBus::hasPendingScript() const
{
    return !scriptedTestResults_.empty() || !scriptedReads_.empty() || !scriptedWriteCounts_.empty();
}

bool TCP1819ScriptedBus::scriptFullyConsumed() const
{
    return !hasPendingScript();
}

void TCP1819ScriptedBus::bind(BBI2C &bus, TCP1819ScriptedBus &scriptedBus)
{
    bindings()[&bus] = &scriptedBus;
}

void TCP1819ScriptedBus::unbind(BBI2C &bus)
{
    bindings().erase(&bus);
}

void TCP1819ScriptedBus::unbindAll()
{
    bindings().clear();
}

TCP1819ScriptedBus *TCP1819ScriptedBus::lookup(BBI2C *bus)
{
    const auto found = bindings().find(bus);
    if (found == bindings().end()) {
        return nullptr;
    }
    return found->second;
}

void TCP1819ScriptedBus::handleInit(BBI2C *bus, uint32_t clockHz)
{
    recordOperation(TCP1819ScriptedBusOperation{
        TCP1819ScriptedBusOpKind::Init,
        bus,
        0,
        clockHz,
        0,
        0,
        {},
    });
}

int TCP1819ScriptedBus::handleTest(BBI2C *bus, uint8_t address)
{
    const int result = scriptedTestResults_.empty() ? defaultTestResult_ : scriptedTestResults_.front();
    if (!scriptedTestResults_.empty()) {
        scriptedTestResults_.pop_front();
    }
    recordOperation(TCP1819ScriptedBusOperation{
        TCP1819ScriptedBusOpKind::Test,
        bus,
        address,
        0,
        0,
        result,
        {},
    });
    return result;
}

int TCP1819ScriptedBus::handleRead(BBI2C *bus, uint8_t address, uint8_t *data, int requestedLength)
{
    if (requestedLength < 0) {
        throw std::invalid_argument("requested read length must be non-negative");
    }

    std::vector<uint8_t> returnedBytes;
    int returnedCount = defaultReadCount_;

    if (!scriptedReads_.empty()) {
        const ReadScriptItem scriptItem = scriptedReads_.front();
        scriptedReads_.pop_front();
        returnedBytes.assign(scriptItem.bytes.begin(), scriptItem.bytes.begin() + scriptItem.returnedCount);
        returnedCount = scriptItem.returnedCount;
    } else {
        returnedBytes.assign(static_cast<std::size_t>(returnedCount), 0U);
    }

    returnedCount = std::min(returnedCount, requestedLength);
    if (returnedCount > 0 && data != nullptr && !returnedBytes.empty()) {
        std::copy(returnedBytes.begin(), returnedBytes.begin() + returnedCount, data);
    }
    if (returnedCount == 0) {
        returnedBytes.clear();
    } else if (static_cast<int>(returnedBytes.size()) > returnedCount) {
        returnedBytes.resize(static_cast<std::size_t>(returnedCount));
    }

    recordOperation(TCP1819ScriptedBusOperation{
        TCP1819ScriptedBusOpKind::Read,
        bus,
        address,
        0,
        requestedLength,
        returnedCount,
        returnedBytes,
    });
    return returnedCount;
}

int TCP1819ScriptedBus::handleWrite(BBI2C *bus, uint8_t address, const uint8_t *data, int requestedLength)
{
    if (requestedLength < 0) {
        throw std::invalid_argument("requested write length must be non-negative");
    }

    const int configuredCount = scriptedWriteCounts_.empty()
                                    ? defaultWriteCount_
                                    : scriptedWriteCounts_.front();
    if (!scriptedWriteCounts_.empty()) {
        scriptedWriteCounts_.pop_front();
    }

    const int returnedCount = (configuredCount == kDefaultRequestedLength)
                                  ? requestedLength
                                  : std::min(configuredCount, requestedLength);
    std::vector<uint8_t> writtenBytes;
    if (requestedLength > 0 && data != nullptr) {
        writtenBytes.assign(data, data + requestedLength);
    }

    recordOperation(TCP1819ScriptedBusOperation{
        TCP1819ScriptedBusOpKind::Write,
        bus,
        address,
        0,
        requestedLength,
        returnedCount,
        writtenBytes,
    });
    return returnedCount;
}

void TCP1819ScriptedBus::recordOperation(const TCP1819ScriptedBusOperation &operation)
{
    operations_.push_back(operation);
}

std::map<BBI2C *, TCP1819ScriptedBus *> &TCP1819ScriptedBus::bindings()
{
    static std::map<BBI2C *, TCP1819ScriptedBus *> registry;
    return registry;
}