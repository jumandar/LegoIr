//
// Lego Power Functions Infrared Control for Arduino
// see http://www.philohome.com/pf/LEGO_Power_Functions_RC_v120.pdf for more info
// Based on SuperCow's code (http://forum.arduino.cc/index.php?topic=38142.0)
// And Jurriaans library https://github.com/jurriaan/Arduino-PowerFunctions
//
// Modified and enhanced by TheDIYGuy999, June 2016:
// - Begin function added to allow channel switching during runtime
// - Modifications to make it also compatible with 8MHz Arduino versions

// Modified and enhanced by TheDIYGuy999, December 2017:
// - Library V1.11 is now compatible with CPU's faster than 16MHz. Tested on an STM32 ARM "Maple" board

#define toggle() _toggle ^= 0x8
#define CHECKSUM() (0xf ^ _nib1 ^ _nib2 ^ _nib3)

#include <stdlib.h>
#include "Arduino.h"
#include "LegoIr.h"

// Aliases
void LegoIr::red_pwm(uint8_t pwm) { single_pwm(RED, pwm); }
void LegoIr::blue_pwm(uint8_t pwm) { single_pwm(BLUE, pwm); }

// Constructor
LegoIr::LegoIr()
{
}

// Begin
// Allows channel switching during runtime. Added June 2016 by TheDIYGuy999
void LegoIr::begin(uint8_t pin, uint8_t channel)
{
    _channel = channel;
    _toggle = 0;
    _pin = pin;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

// Single output mode PWM
void LegoIr::single_pwm(uint8_t output, uint8_t pwm) {
    _nib1 = _toggle | _channel;
    _nib2 = SINGLE_OUTPUT | output;
    _nib3 = pwm;
    send();
    toggle();
}

void LegoIr::single_increment(uint8_t output){
    _nib1 = _toggle | _channel;
    _nib2 = SINGLE_EXT | output;
    _nib3 = 0x4;
    send();
    toggle();
}

void LegoIr::single_decrement(uint8_t output){
    _nib1 = _toggle | _channel;
    _nib2 = SINGLE_EXT | output;
    _nib3 = 0x5;
    send();
    toggle();
}

// Combo PWM mode
void LegoIr::combo_pwm(uint8_t blue_speed, uint8_t red_speed)
{
    _nib1 = ESCAPE | _channel;
    _nib2 = blue_speed;
    _nib3 = red_speed;
    send();
}

//
// Private methods
//

// Pause function see "Transmitting Messages" in Power Functions PDF
void LegoIr::pause(uint8_t count)
{
    uint8_t pause = 0;
    
    if(count == 0) { // Pause before first message
        pause = 4 - (_channel + 1);
    } else if(count < 3) { // Pause 1, 2
        pause = 5;
    } else {  // Pause 3, 4, 5
        pause = 6 + (_channel + 1) * 2; // 6 was 5, but this is wrong according to the PDF!
    }
    
    // The STM32 does not like delays with a duration of zero! So, only if > 0
    if (pause > 0) delayMicroseconds(pause * 77); //MAX_MESSAGE_LENGTH 77
}

// Send the start/stop bit
void LegoIr::start_stop_bit()
{
    send_bit();
    delayMicroseconds(START_STOP); // Extra pause for start_stop_bit
}

// Send a bit
// NOTE: delayMicroseconds() is only active on a >= 16MHz CPU, in order to achieve
// a correct timing on a 8MHz CPU. Added June 2016 by TheDIYGuy999
void LegoIr::send_bit() {
    for(uint8_t i = 0; i < 6; i++) {
        digitalWrite(_pin, HIGH);
#if F_CPU >= 16000000 // 16MHz or faster CPU
        delayMicroseconds(HALF_PERIOD);
#endif
        digitalWrite(_pin, LOW);
#if F_CPU >= 16000000 // 16MHz or faster CPU
        delayMicroseconds(HALF_PERIOD);
#endif
    }
}

void LegoIr::send()
{
    uint8_t i, j;
    uint16_t message = _nib1 << 12 | _nib2 << 8 | _nib3 << 4 | CHECKSUM();
    for(i = 0; i < 6; i++)
    {
        pause(i);
        start_stop_bit();
        for(j = 0; j < 16; j++) {
            send_bit();
            delayMicroseconds((0x8000 & (message << j)) != 0 ? HIGH_PAUSE : LOW_PAUSE);
        }
        start_stop_bit();
    }
}
