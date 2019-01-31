# ChipKIT uc32


## Board description

See https://reference.digilentinc.com/reference/microprocessor/uc32/reference-manual for characteristics.

## Heracles modem
An [Heracles shield](HeraclesShield.md) is used to connect the device to the Live Objects platform via a MQTT connection.


## Pins configuration

Pins configuration :
* Pins **SERIAL_TX**, **SERIAL_RX** used to be linked to the PC (**USB**).
* Pins **40** (U2TX),  **39** (U2RX) used to be linked to Heracles board.
* Pin **D4** set to HIGHT (**reset** of Heracles shield board).


*Example software to use serial line*

```c
#include "Arduino.h"

#define HeraclesSerial Serial1

const int  Rst = 4;

void setup() {
    /* initialize the Rst signal as an output: */
    pinMode(Rst, OUTPUT);
    digitalWrite(Rst, HIGH);

    /* Open serial communication and wait for port to open */
    Serial.begin(115200);
    while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
    }
    Serial.println("TEST AT COMMANDS WITH HERACLES MODEM !");

    /* Set the data rate for the HeraclesSerial port */
    HeraclesSerial.begin(115200);
    HeraclesSerial.println("AT\r\n");
}

void loop() {
       if (HeraclesSerial.available()) {
        Serial.write(HeraclesSerial.read());
    }
    if (Serial.available()) {
          HeraclesSerial.write(Serial.read());
    }
}
```
## Required hardware

* ChipKIT uc32 board.
* Heracles Modem shield.
* Battery Lithium-ion 3.7V for Heracles Modem shield.
* 2 cables to connect RX/TX between ChipKIT uc32 and Heracles modem.
* 1 mini-USB cable.

## Required software

* Eclipse IDE to develop application.
* Serial port monitor.
* [LiveObjects account](http://liveobjects.orange-business.com).

## Building the sample

To build the sample application:

1. Plug the Heracles modem shield on the ChipKIT uc32.
1. Connect the pin **TX** of the Heracles modem to **U2TX** (pin 40) of the ChipKIT uc32.
1. Connect the pin **RX** of the Heracles modem to**U2RX** (pin 39) of the ChipKIT uc32.

![ChipKITuc32Serial](Img/Arduino/ChipKITuc32_Serial.png) ![ChipKITuc32](Img/Arduino/ChipKITuc32.png)

4. Plug the mini-USB cable into the **programming USB** and PC.
1. Clone the LiveBooster repository from [gitHub]() *(TBD)* in a local directory.
1. Create a project on Eclipse as defined in [ArduinoApplication](ArduinoApplication.md)
1. Configure Eclipse to compile and upload the application onto ChipKIT uc32 board :

![ChipKITuc32ArduinoProperties](Img/Eclipse/ChipKITuc32ArduinoProperties.png)

8. Compile and Upload the Example application (button **Upload Sketch**)
1. The board is automatically programmed with the new binary. A flashing LED on it indicates that it is still working. When the LED stops blinking, the board is ready to work and the application start.
1. Start the serial port monitor and configure **COM***xx*, (115200,8,None,1).
1. Press the **RESET** button on the board to restart the program.  A trace of the connection to LiveObject is displayed to the terminal emulator.
1. Connect to the [LiveObjects](http://liveobjects.orange-business.com) platform. Select **parc**, select **Manage/MQTT**. Verify than the Auto-created device **urn:lo:nsid:LiveBooster:test** is connected.
