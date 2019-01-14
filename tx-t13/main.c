#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include "virtualwire.h"
#include <avr/sleep.h>
#include <avr/wdt.h>
#define	DS18B20_PIN	PB4
#define	BUTTON_PIN	PB2
// CHANGE IN TX LIBRARY FILE IF CHANGED
#define LDRPIN PB3
#define LEDPIN PB1



typedef struct messagedata
{
	unsigned char RX_ID;
	unsigned int temperature;
	unsigned int light;
	unsigned char button;	
} messagedata;
messagedata txdata={.RX_ID='A',.temperature=25,.light='L',.button='L'};

// watchdog interrupt
ISR(WDT_vect) 
  {
  wdt_disable();  // disable watchdog
  }

static  void myWatchdogEnable() 
{ 
  cli();  
  MCUSR = 0;                          // reset various flags
  WDTCR |= (1<<WDP3) | (0<<WDP2) | (0<<WDP1) | (1<<WDP0);
  WDTCR |= (1<<WDTIE);
  WDTCR |= (0<<WDE);
  sei();
  wdt_reset(); //call to this reset the timer
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_mode();            // now goes to Sleep and waits for the interrupt
} 


int main(void)
{
	DDRB &=~(1<<LDRPIN); //ADC0 is input PB5
    uint16_t nticks=37; // number of prescaled ticks needed TRY 150
    uint8_t prescaler=2;; // Bit values for CS0[2:0] TRY 1
    TCCR0A = 0;
    TCCR0A = _BV(WGM01); // Turn on CTC mode / Output Compare pins disconnected
    TCCR0B = 0;
    TCCR0B = prescaler; // set CS00, CS01, CS02 (other bits not needed)
	OCR0A = (uint8_t)(nticks);
	TIMSK0 |= _BV(OCIE0A);
	vw_ddr |= (1<<vw_tx_pin);
	vw_port &= ~(1<<vw_tx_pin);
	uint16_t t;
    uint8_t rcvdSize = sizeof(txdata);
	// Set the ADC input to PB3 or ADC3
    ADMUX |= (1 << MUX0);
	ADMUX |= (1 << MUX1);
    ADMUX |= (1 << ADLAR); 
    ADCSRA |= (1 << ADPS1) | (1 << ADPS0) | (1 << ADEN);
    sei();
        
    while (1)
    {
		t=DS18B20_read();
		txdata.temperature=t;
		ADCSRA |= (1 << ADSC);//start conversion
		while (ADCSRA & (1 << ADSC));	
		txdata.light=ADC;
		uint8_t counter=9;
		while(counter)
		{   
			vw_send((uint8_t *)&txdata, rcvdSize);
			vw_wait_tx();
			_delay_ms(500);
			counter--;
		}
		ADCSRA &= ~(1<<ADEN);     //turn off ADC
        ACSR |= _BV(ACD);         // disable analog comparator
		counter=120;
		while(counter)
		{   
			myWatchdogEnable ();  // 8 seconds
			counter--;
		}
		ADCSRA |= (1<<ADEN);    //turn on ADC
    }
}