#include "src/VS/VS10XX.h"
#include "src/MPR/MPR121.h"
#include <Wire.h>

/**
    Wiring:

    1. VS1003
    ------------------------------------
    | VS1003  |  ESP32  | ESP32 (GPIO) |
    ------------------------------------
    |   SCLK  |   D18   |     18       |
    |   MISO  |   D19   |     19       |
    |   MOSI  |   D23   |     23       |
    |   XRST  |   D5    |     5        |
    |   CS    |   D4    |     4        |
    |   XDCS  |   RX2   |     16       |
    |   DREQ  |   TX2   |     17       |
    |   5V    |   3.3V  |     3.3V     |
    |   GND   |   GND   |     GND      |
    ------------------------------------

    2. MPR121 (2x)

    Since we are using 2 MPR121 modules, we connect them to the same SPI bus.
    The difference is in ADD pin, where one is empty (default address 0x5A), and the other
    is connected to 3.3V which changes the address to 0x5B.

    ------------------------------------
    | MPR121  |  ESP32  | ESP32 (GPIO) |
    ------------------------------------
    |   IRQ   |   D33   |     33       |
    |   SCL   |   D22   |     22       |
    |   SDA   |   D21   |     21       |--------------------------
    |   ADD   |   -     |     -        |    3.3V    |    3.3V    |
    |   3.3V  |   3.3V  |     3.3V     |--------------------------
    |   GND   |   GND   |     GND      |
    ------------------------------------
**/

// MPR121
#define INTERUPT_PIN 33
#define numElectrodes 12

MPR121_type MPR121_1;
MPR121_type MPR121_2;

// VS1003 pin definitions
#define VS_XCS 4   // Control Chip Select Pin (for accessing SPI Control/Status registers)
#define VS_XDCS 16 // Data Chip Select / BSYNC Pin
#define VS_DREQ 17 // Data Request Pin: Player asks for more data
#define VS_RESET 5 // Reset is active low

// VS10xx SPI pin connections
// Provided here for info only - not used in the sketch as the SPI library handles this
#define VS_MOSI 23
#define VS_MISO 19
#define VS_SCK 18
#define VS_SS 5

VS10XX vs10xx(VS_XCS, VS_XDCS, VS_DREQ, VS_RESET, VS_SS);

byte instrument;
byte volume;

void setup()
{

    Serial.begin(115200);
    Serial.println("System started");

    while (1)
    {
        if (!MPR121_2.begin(0x5A))
        {
            Serial.println("error setting up MPR121 2");
        }
        else
        {
            break;
        }
    }

    while (1)
    {
        if (!MPR121.begin(0x5B))
        {

            Serial.println("error setting up MPR121");
            switch (MPR121.getError())
            {
            case NO_ERROR:
                Serial.println("no error");
                break;
            case ADDRESS_UNKNOWN:
                Serial.println("incorrect address");
                break;
            case READBACK_FAIL:
                Serial.println("readback failure");
                break;
            case OVERCURRENT_FLAG:
                Serial.println("overcurrent on REXT pin");
                break;
            case OUT_OF_RANGE:
                Serial.println("electrode out of range");
                break;
            case NOT_INITED:
                Serial.println("not initialised");
                break;
            default:
                Serial.println("unknown error");
                break;
            }
        }
        else
        {
            break;
        }
    }

    MPR121.setInterruptPin(INTERUPT_PIN);
    MPR121.setTouchThreshold(40);
    MPR121.setReleaseThreshold(20);
    MPR121.updateTouchData();

    MPR121_2.setInterruptPin(INTERUPT_PIN);
    MPR121_2.setTouchThreshold(40);
    MPR121_2.setReleaseThreshold(20);
    MPR121_2.updateTouchData();

    Serial.println("Initialising VS10xx");
    // put your setup code here, to run once:
    vs10xx.initialiseVS10xx();
    delay(1000);

    // Set these invalid to trigger a read of the pots
    // (if being used) first time through.
    instrument = -1;
    volume = -1;
}

#define OCTAVE_1 48
#define OCTAVE_2 60

void loop()
{
    if (MPR121.touchStatusChanged())
    {
        MPR121.updateTouchData();
        for (int i = 0; i < numElectrodes; i++)
        {
            uint8_t note = i + OCTAVE_1;
            if (MPR121.isNewTouch(i))
            {
                Serial.println(i);
                vs10xx.noteOn(0, note, 127);
            }
            else if (MPR121.isNewRelease(i))
            {
                Serial.println(i);
                vs10xx.noteOff(0, note, 127);
            }
        }
    }

    if (MPR121_2.touchStatusChanged())
    {
        MPR121_2.updateTouchData();
        for (int i = 0; i < numElectrodes; i++)
        {
            uint8_t note = i + OCTAVE_2;
            if (MPR121_2.isNewTouch(i))
            {
                Serial.println(i);
                vs10xx.noteOn(0, note, 127);
            }
            else if (MPR121_2.isNewRelease(i))
            {
                Serial.println(i);
                vs10xx.noteOff(0, note, 127);
            }
        }
    }
}
