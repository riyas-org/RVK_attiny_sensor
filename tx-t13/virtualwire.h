#ifndef _VIRTUALWIRE_H
#define _VIRTUALWIRE_H

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include <avr/io.h>
#include <util/crc16.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>

#define vw_ddr DDRB
#define vw_port PORTB
#define vw_pin PINB
#define LED 2

#define true 1
#define false 0

#define VW_MAX_MESSAGE_LEN 9
#define VW_MAX_PAYLOAD VW_MAX_MESSAGE_LEN-3
#define VW_RX_RAMP_LEN 160
#define VW_RX_SAMPLES_PER_BIT 8
#define VW_RAMP_INC (VW_RX_RAMP_LEN/VW_RX_SAMPLES_PER_BIT)
#define VW_RAMP_TRANSITION VW_RX_RAMP_LEN/2
#define VW_RAMP_ADJUST 9
#define VW_RAMP_INC_RETARD (VW_RAMP_INC-VW_RAMP_ADJUST)
#define VW_RAMP_INC_ADVANCE (VW_RAMP_INC+VW_RAMP_ADJUST)
#define VW_HEADER_LEN 8

#define vw_tx_pin (uint8_t)0

//void vw_set_tx_pin(uint8_t pin);
//void vw_setup(uint16_t speed);
void vw_setup();
//uint8_t vx_tx_active();
void vw_wait_tx();
uint8_t vw_send(uint8_t* buf, uint8_t len);

#endif