// extras/host_test/TCP1819HostShim.cpp v1
#include "TCP1819ScriptedBus.h"

#include <cstddef>
#include <cstring>

void I2CInit(BBI2C *pI2C, uint32_t iClock)
{
    if (pI2C == nullptr) {
        return;
    }
    if (TCP1819ScriptedBus *const scriptedBus = TCP1819ScriptedBus::lookup(pI2C)) {
        scriptedBus->handleInit(pI2C, iClock);
    }
}

uint8_t I2CTest(BBI2C *pI2C, unsigned char addr)
{
    if (pI2C == nullptr) {
        return 0;
    }
    if (TCP1819ScriptedBus *const scriptedBus = TCP1819ScriptedBus::lookup(pI2C)) {
        return static_cast<uint8_t>(scriptedBus->handleTest(pI2C, addr));
    }
    return 0;
}

int I2CRead(BBI2C *pI2C, uint8_t iAddr, uint8_t *pData, int iLen)
{
    if (pI2C == nullptr) {
        return 0;
    }
    if (TCP1819ScriptedBus *const scriptedBus = TCP1819ScriptedBus::lookup(pI2C)) {
        return scriptedBus->handleRead(pI2C, iAddr, pData, iLen);
    }
    if (pData != nullptr && iLen > 0) {
        std::memset(pData, 0, static_cast<std::size_t>(iLen));
    }
    return 0;
}

int I2CWrite(BBI2C *pI2C, unsigned char iAddr, unsigned char *pData, int iLen)
{
    if (pI2C == nullptr) {
        return 0;
    }
    if (TCP1819ScriptedBus *const scriptedBus = TCP1819ScriptedBus::lookup(pI2C)) {
        return scriptedBus->handleWrite(pI2C, iAddr, pData, iLen);
    }
    return 0;
}

int I2CReadRegister(BBI2C *pI2C,
                    unsigned char iAddr,
                    unsigned char u8Register,
                    unsigned char *pData,
                    int iLen)
{
    if (pI2C == nullptr) {
        return 0;
    }

    unsigned char registerByte = u8Register;
    if (I2CWrite(pI2C, iAddr, &registerByte, 1) != 1) {
        return 0;
    }
    return I2CRead(pI2C, iAddr, pData, iLen);
}

void I2CScan(BBI2C *pI2C, unsigned char *pMap)
{
    if (pMap == nullptr) {
        return;
    }

    std::memset(pMap, 0, 16U);
    for (unsigned int i = 1; i < 128; ++i) {
        if (I2CTest(pI2C, static_cast<unsigned char>(i)) != 0U) {
            pMap[i >> 3] |= static_cast<unsigned char>(1U << (i & 7U));
        }
    }
}

int I2CDiscoverDevice(BBI2C *, unsigned char, uint32_t *)
{
    return DEVICE_UNKNOWN;
}

void I2CGetDeviceName(int, char *szName)
{
    if (szName != nullptr) {
        szName[0] = '\0';
    }
}
// extras/host_test/TCP1819HostShim.cpp v1