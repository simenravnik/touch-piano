#include "VS10XX.h"
#include <SPI.h>
#include <MIDI.h>
#include "vlsi/vs10xx_uc.h" // http://www.vlsi.fi/en/support/software/microcontrollersoftware.html
#include "vlsi/rtmidi1003b.h"
#include "vlsi/vs1003inst.h"

#define TEST 1

// There are three selectable sound banks on the VS1053
// These can be selected using the MIDI command 0xBn 0x00 bank
#define DEFAULT_SOUND_BANK 0x00 // General MIDI 1 sound bank
#define DRUM_SOUND_BANK 0x78    // Drums
#define ISNTR_SOUND_BANK 0x79   // General MIDI 2 sound bank

VS10XX::VS10XX(byte xcs, byte xdcs, byte dreq, byte reset, byte ss)
{
    this->VS_XCS = xcs;
    this->VS_XDCS = xdcs;
    this->VS_DREQ = dreq;
    this->VS_RESET = reset;
    this->VS_SS = ss;

    // This is required to set up the MIDI library.
    // The default MIDI setup uses the built-in serial port.
    MIDI_CREATE_DEFAULT_INSTANCE();

// This listens to all MIDI channels
// They will be filtered out later...
#ifndef TEST
    MIDI.begin(MIDI_CHANNEL_OMNI);
#endif // TEST
}

/***
 * Methods to communicate with VS10XX over SPI bus.
 *
 * Influenced by diyelectromusic and MP3_Shield_RealtimeMIDI.ino
 */

// Send a MIDI note-on message.  Like pressing a piano key
// channel ranges from 0-15
void VS10XX::noteOn(byte channel, byte note, byte attack_velocity)
{
    talkMIDI((0x90 | channel), note, attack_velocity);
}

// Send a MIDI note-off message.  Like releasing a piano key
void VS10XX::noteOff(byte channel, byte note, byte release_velocity)
{
    talkMIDI((0x80 | channel), note, release_velocity);
}

void VS10XX::sendMIDI(byte data)
{
    SPI.transfer(0);
    SPI.transfer(data);
}

void VS10XX::talkMIDI(byte cmd, byte data1, byte data2)
{
    //
    // Wait for chip to be ready (Unlikely to be an issue with real time MIDI)
    //
    while (!digitalRead(VS_DREQ))
    {
    }
    digitalWrite(VS_XDCS, LOW);

    sendMIDI(cmd);

    // Some commands only have one data byte. All cmds less than 0xBn have 2 data bytes
    //(sort of: http://253.ccarh.org/handout/midiprotocol/)
    //  SysEx messages (0xF0 onwards) are variable length but not supported at present!
    if ((cmd & 0xF0) <= 0xB0 || (cmd & 0xF0) >= 0xE0)
    {
        sendMIDI(data1);
        sendMIDI(data2);
    }
    else
    {
        sendMIDI(data1);
    }

    digitalWrite(VS_XDCS, HIGH);
}

/***
 * Methods to initialize VS10XX and put it into real-time MIDI mode
 *
 * Based on VS1003b/VS1033c/VS1053b Real-Time MIDI Input Application
 * http://www.vlsi.fi/en/support/software/vs10xxapplications.html
 *
 * Influenced by diyelectromusic and MP3_Shield_RealtimeMIDI.ino
 */
void VS10XX::initialiseVS10xx()
{
    // Set up the pins controller the SPI link to the VS1053
    pinMode(VS_DREQ, INPUT);
    pinMode(VS_XCS, OUTPUT);
    pinMode(VS_XDCS, OUTPUT);
    pinMode(VS_RESET, OUTPUT);

    // Setup SPI
    // The Arduino's Slave Select pin is only required if the
    // Arduino is acting as an SPI slave device.
    // However, the internal circuitry for the ATmeta328 says
    // that if the SS pin is low, the MOSI/MISO lines are disabled.
    // This means that when acting as an SPI master (as in this case)
    // the SS pin must be set to an OUTPUT to prevent the SPI lines
    // being automatically disabled in hardware.
    // We can still use it as an OUTPUT IO pin however as the value
    // (HIGH or LOW) is not significant - it just needs to be an OUTPUT.
    // See: http://www.gammon.com.au/spi
    //
    pinMode(VS_SS, OUTPUT);

    // Now initialise the VS10xx
    digitalWrite(VS_XCS, HIGH);  // Deselect Control
    digitalWrite(VS_XDCS, HIGH); // Deselect Data
    digitalWrite(VS_RESET, LOW); // Put VS1053 into hardware reset

    // And then bring up SPI
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);

    // From page 12 of datasheet, max SCI reads are CLKI/7. Input clock is 12.288MHz.
    // Internal clock multiplier is 1.0x after power up.
    // Therefore, max SPI speed is 1.75MHz. We will use 1MHz to be safe.
    SPI.setClockDivider(SPI_CLOCK_DIV16); // Set SPI bus speed to 1MHz (16MHz / 16 = 1MHz)
    SPI.transfer(0xFF);                   // Throw a dummy byte at the bus

    delayMicroseconds(1);
    digitalWrite(VS_RESET, HIGH); // Bring up VS1053

    // Dummy read to ensure VS SPI bus in a known state
    VSReadRegister(SCI_MODE);

    // Perform software reset and initialise VS mode
    VSWriteRegister16(SCI_MODE, SM_SDINEW | SM_RESET);
    delay(200);
    VSStatus();

    // Enable real-time MIDI mode
    VSLoadUserCode();
    VSStatus();

    // Set the default volume
    // VSWriteRegister(SCI_VOL, 0x20, 0x20);  // 0 = Maximum; 0xFEFE = silence
    VSStatus();

    // Initialise each channel to contain one instrument
    initVS10xxChannels(default_preset_instruments, DEFAULT_MIDI_CHANNEL_FILTER);
}

// This will read key status and mode registers from the VS10xx device
// and dump them to the serial port.
//
void VS10XX::VSStatus(void)
{
#ifdef TEST
    // Print out some of the VS10xx registers
    uint16_t vsreg = VSReadRegister(SCI_MODE); // MODE Mode Register

    Serial.println(vsreg, BIN);
    vsreg = VSReadRegister(SCI_STATUS);
    Serial.print(vsreg, BIN);

    switch (vsreg & SS_VER_MASK)
    {
    case SS_VER_VS1001:
        Serial.println(" (VS1001)");
        break;
    case SS_VER_VS1011:
        Serial.println(" (VS1011)");
        break;
    case SS_VER_VS1002:
        Serial.println(" (VS1002)");
        break;
    case SS_VER_VS1003:
        Serial.println(" (VS1003)");
        break;
    case SS_VER_VS1053:
        Serial.println(" (VS1053)");
        break;
    case SS_VER_VS1033:
        Serial.println(" (VS1033)");
        break;
    case SS_VER_VS1063:
        Serial.println(" (VS1063)");
        break;
    case SS_VER_VS1103:
        Serial.println(" (VS1103)");
        break;
    default:
        Serial.println(" (Unknown)");
        break;
    }
    vsreg = VSReadRegister(SCI_VOL); // VOL Volume
    Serial.print("Volume: ");
    Serial.println(vsreg);
    vsreg = VSReadRegister(SCI_AUDATA); // AUDATA Misc Audio data
    Serial.println();
#endif
}

// Write to VS10xx register
// SCI: Data transfers are always 16bit. When a new SCI operation comes in
// DREQ goes low. We then have to wait for DREQ to go high again.
// XCS should be low for the full duration of operation.
//
void VS10XX::VSWriteRegister(unsigned char addressbyte, unsigned char highbyte, unsigned char lowbyte)
{
    while (!digitalRead(VS_DREQ))
        ;                      // Wait for DREQ to go high indicating IC is available
    digitalWrite(VS_XCS, LOW); // Select control

    // SCI consists of instruction byte, address byte, and 16-bit data word.
    SPI.transfer(0x02); // Write instruction
    SPI.transfer(addressbyte);
    SPI.transfer(highbyte);
    SPI.transfer(lowbyte);
    while (!digitalRead(VS_DREQ))
        ;                       // Wait for DREQ to go high indicating command is complete
    digitalWrite(VS_XCS, HIGH); // Deselect Control
}

// 16-bit interface to the above function.
//
void VS10XX::VSWriteRegister16(unsigned char addressbyte, uint16_t value)
{
    VSWriteRegister(addressbyte, value >> 8, value & 0xFF);
}

// Read a VS10xx register using the SCI (SPI command) bus.
//
uint16_t VS10XX::VSReadRegister(unsigned char addressbyte)
{
    while (!digitalRead(VS_DREQ))
        ;                      // Wait for DREQ to go high indicating IC is available
    digitalWrite(VS_XCS, LOW); // Select control

    SPI.transfer(0x03); // Read instruction
    SPI.transfer(addressbyte);
    delayMicroseconds(10);
    uint8_t d1 = SPI.transfer(0x00);
    uint8_t d2 = SPI.transfer(0x00);
    while (!digitalRead(VS_DREQ))
        ;                       // Wait for DREQ to go high indicating command is complete
    digitalWrite(VS_XCS, HIGH); // Deselect control

    return ((d1 << 8) | d2);
}

// Load a user plug-in over SPI.
//
// See the application and plug-in notes on the VLSI website for details.
//
void VS10XX::VSLoadUserCode(void)
{
#ifdef TEST
    Serial.print("Loading User Code");
#endif
    for (int i = 0; i < VS10xx_CODE_SIZE; i++)
    {
        uint8_t addr = pgm_read_byte_near(&vs10xx_atab[i]);
        uint16_t dat = pgm_read_word_near(&vs10xx_dtab[i]);
#ifdef TEST
        if (!(i % 8))
            Serial.print(".");
#endif
        VSWriteRegister16(addr, dat);
    }

    // Set the start address of the application code (see rtmidi.pdf application note)
    VSWriteRegister16(SCI_AIADDR, 0x30);

#ifdef TEST
    Serial.print("Done\n");
#endif
}

void VS10XX::initVS10xxChannels(byte preset_instruments[16], uint16_t MIDI_CHANNEL_FILTER)
{
    // Configure the instruments for all required MIDI channels.
    // Even though MIDI channels are 1 to 16, all the code here
    // is working on 0 to 15 (bitshifts, index into array, MIDI command).
    //
    // Also, instrument numbers in the MIDI spec are 1 to 128,
    // but need to be converted to 0 to 127 here.

    for (byte ch = 0; ch < 16; ch++)
    {
        if (ch != 9)
        { // Ignore channel 10 (drums)
            if (preset_instruments[ch] != 0)
            {
                uint16_t ch_filter = 1 << ch;
                if (MIDI_CHANNEL_FILTER & ch_filter)
                {
                    talkMIDI(0xC0 | ch, preset_instruments[ch] - 1, 0);
                }
            }
        }
    }
    // For some reason, the last program change isn't registered
    // without an extra "dummy" read here.
    talkMIDI(0x80, 0, 0);
}