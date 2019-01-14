#include <avr/io.h>
#include <util/delay.h>
#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include "uart.h"
#include "VirtualWire.h"

#define LED 5 //PIN13 on arduino

#ifndef F_CPU
#define F_CPU 16000000
#endif

typedef struct messagedata
{
	unsigned char RX_ID;
	int temperature;
	uint16_t  light;
    unsigned int battery;
	unsigned char button;
	
} messagedata;

int main(void)
{
    
    DDRB |= (1<<LED);
    sei();
    
   uart_init(UART_BAUD_SELECT(9600,F_CPU)); 
   _delay_ms(2000);
   uart0_puts("started!!\r\n"); 
   uart0_puts("Hellow!\r\n");
   vw_set_rx_pin(7); //PD 7 is digital pin 7
   vw_setup(500); //TRY 1000
   vw_rx_start();
    
    
    while (1)
    {
    
    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;
    
    if (vw_get_message(buf, &buflen)) // Non-blocking
    {
    PORTB ^= _BV(LED);
	messagedata *p = (messagedata *)buf;
	char buffer[30];
	sprintf( buffer, "T:%4d.%01dC\r\nADC:%4u\r\nID:%d\r\nBT:%d", (p->temperature) >> 4, (unsigned int)((p->temperature) << 12) / 6553,((p->light)>>6),(p->RX_ID),(p->button) );
    uart0_puts("\r\n");	
	uart0_puts(buffer);
	
    }
    
    }
}