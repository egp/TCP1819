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
    scriptedTestResults_.push_back(AddressedResult{false, 0U, result});
}

void TCP1819ScriptedBus::queueTestResultForAddress(uint8_t address, int result)
{
    scriptedTestResults_.push_back(AddressedResult{true, address, result});
}

void TCP1819ScriptedBus::queueReadVector(const std::vector<uint8_t> &data, int returnedCount)
{
    if (returnedCount < 0) {
        throw std::invalid_argument("returned read count must be non-negative");
    }
    if (static_cast<std::size_t>(returnedCount) > data.size()) {
        throw std::invalid_argument("returned read count cannot exceed queued bytes");
    }
    scriptedReads_.push_back(ReadScriptItem{false, 0U, data, returnedCount});
}

void TCP1819ScriptedBus::queueReadVectorForAddress(uint8_t address,
                                                   const std::vector<uint8_t> &data,
                                                   int returnedCount)
{
    if (returnedCount < 0) {
        throw std::invalid_argument("returned read count must be non-negative");
    }
    if (static_cast<std::size_t>(returnedCount) > data.size()) {
        throw std::invalid_argument("returned read count cannot exceed queued bytes");
    }
    scriptedReads_.push_back(ReadScriptItem{true, address, data, returnedCount});
}

void TCP1819ScriptedBus::queueWriteCount(int returnedCount)
{
    if (returnedCount < 0) {
        throw std::invalid_argument("returned write count must be non-negative");
    }
    scriptedWriteCounts_.push_back(AddressedResult{false, 0U, returnedCount});
}

void TCP1819ScriptedBus::queueWriteCountForAddress(uint8_t address, int returnedCount)
{
    if (returnedCount < 0) {
        throw std::invalid_argument("returned write count must be non-negative");
    }
    scriptedWriteCounts_.push_back(AddressedResult{true, address, returnedCount});
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
        0U,
        clockHz,
        0,
        0,
        {},
    });
}

int TCP1819ScriptedBus::handleTest(BBI2C *bus, uint8_t address)
{
    int result = defaultTestResult_;
    if (!scriptedTestResults_.empty()) {
        const AddressedResult &scriptItem = scriptedTestResults_.front();
        if (!scriptItem.hasAddress || scriptItem.address == address) {
            result = scriptItem.value;
            scriptedTestResults_.pop_front();
        }
    }

    recordOperation(TCP1819ScriptedBusOperation{
        TCP1819ScriptedBusOpKind::Test,
        bus,
        address,
        0U,
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

    int returnedCount = defaultReadCount_;
    std::vector<uint8_t> returnedBytes;

    if (!scriptedReads_.empty()) {
        const ReadScriptItem &scriptItem = scriptedReads_.front();
        if (!scriptItem.hasAddress || scriptItem.address == address) {
            returnedCount = scriptItem.returnedCount;
            returnedBytes.assign(scriptItem.bytes.begin(),
                                 scriptItem.bytes.begin() + scriptItem.returnedCount);
            scriptedReads_.pop_front();
        }
    }

    returnedCount = std::min(returnedCount, requestedLength);

    if (static_cast<int>(returnedBytes.size()) > returnedCount) {
        returnedBytes.resize(static_cast<std::size_t>(returnedCount));
    }

    if (returnedCount > 0) {
        if (returnedBytes.empty()) {
            returnedBytes.assign(static_cast<std::size_t>(returnedCount), 0U);
        }
        if (data != nullptr) {
            std::copy(returnedBytes.begin(), returnedBytes.end(), data);
        }
    } else {
        returnedBytes.clear();
    }

    recordOperation(TCP1819ScriptedBusOperation{
        TCP1819ScriptedBusOpKind::Read,
        bus,
        address,
        0U,
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

    int configuredCount = defaultWriteCount_;
    if (!scriptedWriteCounts_.empty()) {
        const AddressedResult &scriptItem = scriptedWriteCounts_.front();
        if (!scriptItem.hasAddress || scriptItem.address == address) {
            configuredCount = scriptItem.value;
            scriptedWriteCounts_.pop_front();
        }
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
        0U,
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