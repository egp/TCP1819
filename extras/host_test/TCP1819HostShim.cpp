#include "TCP1819ScriptedBus.h"

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

int I2CReadRegister(BBI2C *, unsigned char, unsigned char, unsigned char *, int)
{
    return 0;
}

void I2CScan(BBI2C *, unsigned char *pMap)
{
    if (pMap != nullptr) {
        std::memset(pMap, 0, 16U);
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