
#include "ArduinoSerialImpl.h"
#include "Arduino.h"

#define serialPort Serial1

void arduinoSerialOpen () {
	serialPort.begin(115200);
}

int arduinoSerialAvailable () {
	return serialPort.available();
}

char arduinoSerialGet () {
	return serialPort.read();
}

void arduinoSerialWrite (const char *buffer, int size) {
	for (int i = 0; i < size; i++) {
		serialPort.write (buffer[i]);
	}
}

SerialInterface arduinoSerialImpl =
{
        arduinoSerialOpen,
		arduinoSerialAvailable,
		arduinoSerialGet,
		arduinoSerialWrite,
};
