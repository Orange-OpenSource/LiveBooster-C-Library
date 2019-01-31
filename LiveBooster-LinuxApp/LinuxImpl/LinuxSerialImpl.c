#include "LinuxSerialImpl.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>


/* define PORTNAME   for heracles serial line)  */
//#define PORTNAME "/dev/ttymxc5"  /* for UDOO Neo board       */
#define PORTNAME "/dev/ttyAMA0"  /* for raspberry pi 3 board */

int fd = -1;
int IsReceived = 0;
char rcvd = 0;


void linuxSerialOpen () {

    if (fd < 0) {
        fd = open (PORTNAME , O_RDWR | O_NOCTTY | O_SYNC);

        if (fd < 0) {
                printf ("error %d opening %s: %s", errno, PORTNAME , strerror (errno));
                return;
        }

         struct termios tty;
         memset (&tty, 0, sizeof tty);
         if (tcgetattr (fd, &tty) != 0)
         {
             printf ("error %d from tcgetattr", errno);
         }

         cfsetospeed (&tty, B115200);
         cfsetispeed (&tty, B115200);

         tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP     // disable break processing
                          | INLCR | IGNCR | ICRNL               // disable conversion CR <-> NL
                          | IXON | IXOFF | IXANY);              // shut off xon/xoff ctrl

         tty.c_oflag = 0;                // no remapping, no delays

         tty.c_lflag = 0;                // no signaling chars, no echo,
                                         // no canonical processing

         tty.c_cc[VMIN]  = 0;            // read doesn't block
         tty.c_cc[VTIME] = 0;            // no read timeout : this is a completely non-blocking read

         tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;  // 8-bit chars
         tty.c_cflag |= (CLOCAL | CREAD);             // ignore modem controls, enable reading
         tty.c_cflag &= ~(PARENB | PARODD);           // shut off parity
         tty.c_cflag &= ~CSTOPB;
         tty.c_cflag &= ~CRTSCTS;

         if (tcsetattr (fd, TCSANOW, &tty) != 0)
         {
             printf ("error %d from tcsetattr", errno);
         }
    }

}

int linuxSerialAvailable () {

    IsReceived = read (fd, &rcvd, 1);

    if (IsReceived == 1) {
        return IsReceived;
    }
    return 0;
}

char linuxSerialGet () {

    if (IsReceived == 1) {
         IsReceived = 0;
       return rcvd;
    }
    IsReceived = read (fd, &rcvd, 1);

    if (IsReceived == 1) {
         IsReceived = 0;
       return rcvd;
    }
    return 0;
}

void linuxSerialWrite (const char *buffer, int size) {
    write (fd, buffer, size);
}

SerialInterface linuxSerialImpl =
{
		linuxSerialOpen,
		linuxSerialAvailable,
		linuxSerialGet,
		linuxSerialWrite
};
