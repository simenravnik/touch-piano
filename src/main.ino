#include "src/VS/VS10XX.h"
#include "src/MPR/MPR121.h"
#include <Wire.h>

// MPR121
#define INTERUPT_PIN 33
#define numElectrodes 12

MPR121_type MPR121_1;
MPR121_type MPR121_2;

// VS1003 Module pin definitions
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

uint16_t MIDI_CHANNEL_FILTER = 0b1111111111111111;
byte preset_instruments[16] = {
    /* 01 */ 1,
    /* 02 */ 9,
    /* 03 */ 17,
    /* 04 */ 25,
    /* 05 */ 30,
    /* 06 */ 33,
    /* 07 */ 41,
    /* 08 */ 49,
    /* 09 */ 57,
    /* 10 */ 0, // Channel 10 will be ignored later as that is percussion anyway.
    /* 11 */ 65,
    /* 12 */ 73,
    /* 13 */ 81,
    /* 14 */ 89,
    /* 15 */ 113,
    /* 16 */ 48};

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

    vs10xx.initVS10xxChannels(preset_instruments, MIDI_CHANNEL_FILTER);

    // Set these invalid to trigger a read of the pots
    // (if being used) first time through.
    instrument = -1;
    volume = -1;
}

#define C 0
#define C_sharp 1
#define D 2
#define D_sharp 3
#define E 4
#define F 5
#define F_sharp 6
#define G 7
#define G_sharp 8
#define A 9
#define A_sharp 10
#define B 11

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
                vs10xx.noteOn(2, note, 127);
            }
            else if (MPR121.isNewRelease(i))
            {
                Serial.println(i);
                vs10xx.noteOff(2, note, 127);
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
