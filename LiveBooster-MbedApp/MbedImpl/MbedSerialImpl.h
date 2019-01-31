
#ifndef __MbedSerialImpl_h
#define __MbedSerialImpl_h

#include "SerialInterface.h"
#include "mbed.h"

extern SerialInterface mbedSerialImpl;

/* define pin value TX/RX used by modem */
extern PinName SerialPortTX;
extern PinName SerialPortRX;

#endif
