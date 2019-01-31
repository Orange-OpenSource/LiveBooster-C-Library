
#include "ArduinoDebugImpl.h"
#include "Arduino.h"

#define debugPort Serial

void arduinoPrint (const char *log) {
    debugPort.print(log);
}

DebugInterface arduinoDebugImpl =
{
        arduinoPrint
};
