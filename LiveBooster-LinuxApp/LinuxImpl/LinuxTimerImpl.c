#include "LinuxTimerImpl.h"
#include <unistd.h>
#include <time.h>



void linuxTimerInit () {
   ;
}

unsigned long linuxMillis (){
    struct timespec ts;
    unsigned long theTick = 0U;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    theTick  = ts.tv_nsec / 1000000;
    theTick += ts.tv_sec * 1000;
    return theTick;
}

void linuxDelay (unsigned long waitTimeInMs) {
	   usleep(waitTimeInMs * 1000);
}

TimerInterface linuxTimerImpl =
{
		linuxTimerInit,
		linuxMillis,
		linuxDelay
};

