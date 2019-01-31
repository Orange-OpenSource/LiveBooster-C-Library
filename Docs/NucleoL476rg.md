# NUCLEO L476RG


## Board description

See https://os.mbed.com/platforms/ST-Nucleo-L476RG/ for characteristics.

```c
The UART Serial1 use for Heracles communication do not support speed greater than 38400 bauds.
```

## Heracles modem
[Heracles shield](HeraclesShield.md) is used to connect the device to the Live Objects platform via a MQTT connection.

## Pins configuration

Pins configuration :
 * Pins **SERIAL_TX**, **SERIAL_RX** used to be linked to the PC (**USB PWR**).
 * Pins **D8** (Serial1_TX),  **D2** (Serial1_RX) used to be linked to Heracles board.
 * Pin **D4** set to HIGHT (**reset** of Heracles shield board).

 ![NucleoL476rgPin](Img/Mbed/NucleoL476rgPin.png)


*Example software to use the serial line*

```c
#include "mbed.h"

DigitalOut Pin4(D4);

Serial Heracles(D8,D2,38400); // do not support 115200 bauds
Serial Pc(SERIAL_TX,SERIAL_RX,115200);

int main()
{
   int cpt = 0;
   Pin4 = 1;
   led = 1;

    while (!Pc) {
      ; // wait for serial port to connect. Needed for native USB port only
    }
     Pc.printf("TEST AT COMMANDS WITH HERACLES MODEM !\n");
    Heracles.printf("AT\r\n");   
    while(1) {
         if (Heracles.readable()) {
          Pc.putc(Heracles.getc());
       }
       if (Pc.readable()) {
          Heracles.putc(Pc.getc());
       }
    }
}
```
## Required hardware

* Nucleo L476RG board.
* Heracles Modem shield.
* 3.7V Lithium-Ion battery for the Heracles Modem shield.
* 1 mini-USB cable.
* 1 cable to connect pin D8 Nucleo to Tx Heracles shield.
* 1 cable to connect pin D2 Nucleo to Rx Heracles shield.

## Required software

* Mbed environment (Mbed library, GCC_ARM).
* Serial port monitor.
* [LiveObjects account](http://liveobjects.orange-business.com).

## Building the sample

To build the sample application:

1. Plug the Heracles modem shield to the Nucleo L476RG board.

![NucleoL476rgBoard](Img/Mbed/NucleoL476rgBoard.png)

2. Connect pin **D8** Nucleo to **Tx** Heracles shield and **D2** to **Rx** (or use the strap on RX *Arduino*).
1. Plug the mini-USB cable into the **USB PWR** and PC.
1. Clone the LiveBooster repository from [gitHub]() *(TBD)* in a local directory.
1. Create the project environnement as defined in [MbedApplication](MbedApplication.md).
1. Configure the speed of the RS to **38400 bauds** in [MbedSerialImpl.cpp](..\LiveBooster-MbedApp\MbedImpl\MbedSerialImpl.cpp).
1. Make the application as defined in [MbedApplication](MbedApplication.md).
1. Drag and drop **LiveBooster-MbedApp.bin** to drive **NODE_L476RG (F:)**.  
1. The board is automatically programmed with the new binary. A flashing LED on it indicates that it is still working. When the LED stops blinking, the board is ready to work and application start.
1. Start the serial port monitor and configure **COM***xx*, (115200,8,None,1).
1. Press the **RESET** button on the board to restart the program.  A trace of the connection to LiveObject is displayed to the terminal emulator.
1. Connect to the [LiveObjects](http://liveobjects.orange-business.com) platform. Select **parc**, select **Manage/MQTT**. Verify than the Auto-created device **urn:lo:nsid:LiveBooster:test** is connected
