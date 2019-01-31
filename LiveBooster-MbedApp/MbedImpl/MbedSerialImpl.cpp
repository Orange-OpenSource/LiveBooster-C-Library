#include "MbedSerialImpl.h"

//Serial serialPort(SerialPortTX,SerialPortRX,38400); // used for NUCLEO_L476RG
Serial serialPort(SerialPortTX,SerialPortRX,115200); // used for NUCLEO_F746ZG

void mbedSerialOpen () {
	;
}

int mbedSerialAvailable () {
	return serialPort.readable();
}

char mbedSerialGet () {
	return serialPort.getc();
}

void mbedSerialWrite (const char *buffer, int size) {
	for (int i = 0; i < size; i++) {
		serialPort.putc (buffer[i]);
	}
}

SerialInterface mbedSerialImpl =
{
        mbedSerialOpen,
		mbedSerialAvailable,
		mbedSerialGet,
		mbedSerialWrite,
};
