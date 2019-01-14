/**
 * Copyright (c) 2018, Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 *
 * This is OneWire library for tinyAVR family.
 *
 * References:
 * - library: https://github.com/lpodkalicki/attiny-onewire-library
 * - documentation: https://github.com/lpodkalicki/attiny-onewire-library/README.md
 */

#ifndef _ATTINY_ONEWIRE_H_
#define _ATTINY_ONEWIRE_H_

#include <stdint.h>

#define	ONEWIRE_SEARCH_ROM	0xF0
#define	ONEWIRE_READ_ROM	0x33
#define	ONEWIRE_MATCH_ROM	0x55
#define	ONEWIRE_SKIP_ROM	0xCC
#define	ONEWIRE_ALARM_SEARCH	0xEC

#define	DS18B20_CONVERT_T	0x44
#define	DS18B20_READ		0xBE

/* Initialization.
 * pin: GPIO pin of PORTB
 */
//void DS18B20_init(uint8_t pin);

/* Read temperature (Celcius).
 * Returns real number rounded to two decimal places as integer.
 * For example, to get decimal value:
 *  temp = DS18B20_read() / 100;
 */
uint16_t DS18B20_read(void);

//void onewire_init(uint8_t pin);
static uint8_t onewire_reset(void);
static uint8_t onewire_write(uint8_t value);
static uint8_t onewire_read(void);

#endif	/* !_ATTINY_ONEWIRE_H_ */
