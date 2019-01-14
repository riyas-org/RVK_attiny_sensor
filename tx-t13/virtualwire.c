#include "virtualwire.h"
#include <avr/pgmspace.h>

static uint8_t vw_tx_buf[(VW_MAX_MESSAGE_LEN * 2) + VW_HEADER_LEN]
     = {0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x38, 0x2c};

static uint8_t vw_tx_len = 0;
static uint8_t vw_tx_index = 0;
static uint8_t vw_tx_bit = 0;
static uint8_t vw_tx_sample = 0;
static volatile uint8_t vw_tx_enabled = 0;
static uint16_t vw_tx_msg_count = 0;
//static uint8_t vw_tx_pin;
const static uint8_t symbols[] PROGMEM =
{
    0xd,  0xe,  0x13, 0x15, 0x16, 0x19, 0x1a, 0x1c,
    0x23, 0x25, 0x26, 0x29, 0x2a, 0x2c, 0x32, 0x34
};
/*
static uint16_t vw_crc(uint8_t *ptr, uint8_t count)
{
    uint16_t crc = 0xffff;

    while (count-- > 0)
    crc = _crc_ccitt_update(crc, *ptr++);
    return crc;
}

static uint8_t vw_symbol_6to4(uint8_t symbol)
{
    uint8_t i;
    
    // Linear search :-( Could have a 64 byte reverse lookup table?
    for (i = 0; i < 16; i++)
    if (symbol == pgm_read_byte(&(symbols[i]))) return i;
    return 0; // Not found
}
*/
/*
void vw_set_tx_pin(uint8_t pin)
{
    vw_tx_pin = pin;
}
*/

/*
static uint8_t _timer_calc(uint16_t speed, uint16_t max_ticks, uint16_t *nticks)
{
    // Clock divider (prescaler) values - 0/3333: error flag
    uint16_t prescalers[] = {0, 1, 8, 64, 256, 1024, 3333};
    uint8_t prescaler=0; // index into array & return bit value
    unsigned long ulticks; // calculate by ntick overflow

    // Div-by-zero protection
    if (speed == 0)
    {
        // signal fault
        *nticks = 0;
        return 0;
    }

    // test increasing prescaler (divisor), decreasing ulticks until no overflow
    for (prescaler=1; prescaler < 7; prescaler += 1)
    {
        // Amount of time per CPU clock tick (in seconds)
        float clock_time = (1.0 / ((float)(F_CPU) / (float)(prescalers[prescaler])));
        // Fraction of second needed to xmit one bit
        float bit_time = ((1.0 / (float)(speed)) / 8.0);
        // number of prescaled ticks needed to handle bit time @ speed
        ulticks = (long)(bit_time / clock_time);
        // Test if ulticks fits in nticks bitwidth (with 1-tick safety margin)
        if ((ulticks > 1) && (ulticks < max_ticks))
        {
            break; // found prescaler
        }
        // Won't fit, check with next prescaler value
    }

    // Check for error
    if ((prescaler == 6) || (ulticks < 2) || (ulticks > max_ticks))
    {
        // signal fault
        *nticks = 0;
        return 0;
    }

    *nticks = ulticks;
    return prescaler;
}
*/

static void vw_tx_start()
{
    vw_tx_index = 0;
    vw_tx_bit = 0;
    vw_tx_sample = 0;
    vw_tx_enabled = true;
}

static void vw_tx_stop()
{
    vw_port &= ~(1<<vw_tx_pin);
    vw_tx_enabled = false;
}

static uint8_t vx_tx_active()
{
    return vw_tx_enabled;
}

void vw_wait_tx()
{
    while (vw_tx_enabled);
    
}


uint8_t vw_send(uint8_t* buf, uint8_t len)
{
    uint8_t i;
    uint8_t index = 0;
    uint16_t crc = 0xffff;
    uint8_t *p = vw_tx_buf + VW_HEADER_LEN; // start of the message area
    uint8_t count = len + 3; // Added byte count and FCS to get total number of bytes

    if (len > VW_MAX_PAYLOAD)
    return false;

    // Wait for transmitter to become available
    vw_wait_tx();

    // Encode the message length
    crc = _crc_ccitt_update(crc, count);
    p[index++] = pgm_read_byte(&(symbols[count >> 4]));
    p[index++] = pgm_read_byte(&(symbols[count & 0xf]));

    // Encode the message into 6 bit symbols. Each byte is converted into
    // 2 6-bit symbols, high nybble first, low nybble second
    for (i = 0; i < len; i++)
    {
    crc = _crc_ccitt_update(crc, buf[i]);
    p[index++] = pgm_read_byte(&(symbols[buf[i] >> 4]));
    p[index++] = pgm_read_byte(&(symbols[buf[i] & 0xf]));
    }

    // Append the fcs, 16 bits before encoding (4 6-bit symbols after encoding)
    // Caution: VW expects the _ones_complement_ of the CCITT CRC-16 as the FCS
    // VW sends FCS as low byte then hi byte
    crc = ~crc;
    p[index++] = pgm_read_byte(&(symbols[(crc >> 4)  & 0xf]));
    p[index++] = pgm_read_byte(&(symbols[crc & 0xf]));
    p[index++] = pgm_read_byte(&(symbols[(crc >> 12) & 0xf]));
    p[index++] = pgm_read_byte(&(symbols[(crc >> 8)  & 0xf]));

    // Total number of 6-bit symbols to send
    vw_tx_len = index + VW_HEADER_LEN;

    // Start the low level interrupt handler sending symbols
    vw_tx_start();

    return true;
}

ISR (TIM0_COMPA_vect){
    if (vw_tx_enabled && vw_tx_sample++ == 0)
    {
    // Send next bit
    // Symbols are sent LSB first
    // Finished sending the whole message? (after waiting one bit period
    // since the last bit)
    if (vw_tx_index >= vw_tx_len)
    {
        vw_tx_stop();
        vw_tx_msg_count++;
    }
    else
    {
       
       if(vw_tx_buf[vw_tx_index] & (1 << vw_tx_bit++))
       {
           vw_port |= (1<<vw_tx_pin);
       }
       else
       {
           vw_port &= ~(1<<vw_tx_pin);
       }
       
        if (vw_tx_bit >= 6)
        {
        vw_tx_bit = 0;
        vw_tx_index++;
        }
		
    }
    }
    if (vw_tx_sample > 7)
    vw_tx_sample = 0;
}



//ISR (TIMER0_COMPA_vect) in Mega
/*
ISR (TIM0_COMPA_vect)
{
    if (vw_tx_enabled && vw_tx_sample++ == 0)
    {
    // Send next bit
    // Symbols are sent LSB first
    // Finished sending the whole message? (after waiting one bit period
    // since the last bit)
    if (vw_tx_index >= vw_tx_len)
    {
        vw_tx_stop();
        vw_tx_msg_count++;
    }
    else
    {
       // digitalWrite(vw_tx_pin, vw_tx_buf[vw_tx_index] & (1 << vw_tx_bit++));
       if(vw_tx_buf[vw_tx_index] & (1 << vw_tx_bit++))
       {
           vw_port |= (1<<vw_tx_pin);
       }
       else
       {
           vw_port &= ~(1<<vw_tx_pin);
       }
       
        if (vw_tx_bit >= 6)
        {
        vw_tx_bit = 0;
        vw_tx_index++;
        }
    }
    }
    if (vw_tx_sample > 7)
    vw_tx_sample = 0;
   }
*/ 