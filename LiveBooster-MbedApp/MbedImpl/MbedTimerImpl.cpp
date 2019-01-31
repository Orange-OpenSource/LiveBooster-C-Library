#include "MbedTimerImpl.h"
#include "mbed.h"


static volatile unsigned long millisValue = 0;
Ticker ticker;

void millisTicker () {
    millisValue ++;
}

void mbedTimerInit () {
    ticker.attach (millisTicker, 0.001);
}

unsigned long mbedMillis (){
	return (millisValue);
}

void mbedDelay (unsigned long waitTimeInMs) {
	wait_ms(waitTimeInMs);
}

TimerInterface mbedTimerImpl =
{
		mbedTimerInit,
		mbedMillis,
		mbedDelay
};
