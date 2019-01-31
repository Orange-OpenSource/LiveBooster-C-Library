#include "LinuxDebugImpl.h"
#include <stdio.h>

void linuxPrint (const char *log) {
   printf("%s\n",log);
}

DebugInterface linuxDebugImpl =
{
        linuxPrint
};
