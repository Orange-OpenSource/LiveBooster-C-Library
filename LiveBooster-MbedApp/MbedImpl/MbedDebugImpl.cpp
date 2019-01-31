#include "MbedDebugImpl.h"
#include "Mbed.h"

Serial debugPort (SERIAL_TX,SERIAL_RX,115200);

void mbedPrint (const char *log) {
   debugPort.printf(log);
}

DebugInterface mbedDebugImpl =
{
        mbedPrint,
};
