/*
    Copyright 2014 Alistair Buxton <a.j.buxton@gmail.com>

    avr-433 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    avr-teletext is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with avr-teletext. If not, see <http://www.gnu.org/licenses/>.
*/

#include "main.h"

void RadioOn(void) {
    DDRB |=  0x01;
    PORTB |=  0x01;
    PORTD &= ~0x40;
}

void RadioOff(void) {
    DDRB |=  0x01;
    PORTB &= ~0x01;
    PORTD |=  0x40;
}

#define H 0b11101110
#define L 0b10001000
#define F 0b10001110
#define S 0b10000000

volatile uint8_t packet[16] = {
    F, F, F, F,
    F, F, F, F,
    F, F, F, F,
    S, 0, 0, 0,
};

volatile uint8_t packet_len = 16;
volatile uint8_t packet_counter = 0;
volatile uint8_t packet_repeat = 10;
volatile uint8_t packet_repeat_counter = 0;

void Go(void) {

    cli();

    RadioOn();
    packet_counter = 0;
    packet_repeat_counter = 0;

    UBRR1H = 0x00;
    UBRR1L = 0x00;
    UCSR1C = 0xc0;
    UCSR1B = 0x28;
    UBRR1H = 0x0a;
    UBRR1L = 0x7f;

    sei();

}

ISR(USART1_UDRE_vect) {

    if(packet_counter < packet_len)
    {
        UDR1 = packet[packet_counter];
        packet_counter += 1;
    }
    else if (packet_repeat_counter < packet_repeat)
    {
        UDR1 = packet[0];
        packet_counter = 1;
        packet_repeat_counter += 1;
    }
    else
    {
        UCSR1B = 0;
        UCSR1C = 0;
        RadioOff();
    }

}

void ParseRequest(uint8_t req) {

    memset(packet, F, 12);
    packet[req&3] = L;
    packet[4+((req>>2)&3)] = L;
    if(req>>4)
        packet[11] = H;
    else
        packet[11] = L;

}

void SetupHardware(void)
{

//  +---+-----+-----+-----+-----+-----+-----+-----+-----+
//  |   |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |
//  +---+-----+-----+-----+-----+-----+-----+-----+-----+
//  | B | PWR |  -  |  -  |  -  |  -  |  -  |  -  |  -  |
//  +---+-----+-----+-----+-----+-----+-----+-----+-----+
//  | C |  -  |  -  |  -  |  -  |  -  |  -  |  -  |  -  |
//  +---+-----+-----+-----+-----+-----+-----+-----+-----+
//  | D |  -  |  -  |  -  | TXD |  -  |  B  |  A  |  -  |
//  +---+-----+-----+-----+-----+-----+-----+-----+-----+
 
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    clock_prescale_set(clock_div_1);

    /* Hardware Initialization */
    USB_Init();

    DDRD  = 0x68;
    PORTD = 0x60; /* LEDs off. */

    /* MSPIM Initialization */
    UBRR1H = 0;
    UBRR1L = 0;
    // USART0 MSPIM mode, MSB
    UCSR1C = 0x00;
    // enable Tx
    UCSR1B = 0x00;

}

int main(void)
{
    cli();

    SetupHardware();

    sei();

    for (;;) {
        USB_USBTask();
    }
}

void EVENT_USB_Device_Connect(void)
{
    ;
}

void EVENT_USB_Device_Disconnect(void)
{
    ;
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
    ;
}

void EVENT_USB_Device_ControlRequest(void)
{
    switch (USB_ControlRequest.bmRequestType) {
    case 0x40:
        ParseRequest(USB_ControlRequest.bRequest);
        Go();
        Endpoint_ClearSETUP();
        Endpoint_ClearIN();
        break;
    case 0xc0:
        Endpoint_ClearSETUP();
        Endpoint_ClearOUT();
        break;
    }
}
