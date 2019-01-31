
#include "ArduinoTimerImpl.h"
#include "Arduino.h"

void timerInit (){
	;
}

unsigned long arduinoMillis (){
	return millis();
}

void arduinoDelay (unsigned long waitTimeInMs) {
	delay(waitTimeInMs);
}

TimerInterface arduinoTimerImpl =
{
		timerInit,
		arduinoMillis,
		arduinoDelay
};
