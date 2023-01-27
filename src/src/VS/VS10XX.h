#ifndef VS10XX_H
#define VS10XX_H
#include <Arduino.h>

class VS10XX
{

private:
    byte VS_XCS;
    byte VS_XDCS;
    byte VS_DREQ;
    byte VS_RESET;
    byte VS_SS;

public:
    VS10XX(byte xcs, byte xdcs, byte dreq, byte reset, byte ss);

    void noteOn(byte channel, byte note, byte attack_velocity);
    void noteOff(byte channel, byte note, byte release_velocity);
    void sendMIDI(byte data);
    void talkMIDI(byte cmd, byte data1, byte data2);
    void initialiseVS10xx();
    void VSStatus(void);
    void VSWriteRegister(unsigned char addressbyte, unsigned char highbyte, unsigned char lowbyte);
    void VSWriteRegister16(unsigned char addressbyte, uint16_t value);
    uint16_t VSReadRegister(unsigned char addressbyte);
    void VSLoadUserCode(void);
    void initVS10xxChannels(byte preset_instruments[16], uint16_t MIDI_CHANNEL_FILTER);
};

#endif