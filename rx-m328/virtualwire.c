#include "VirtualWire.h"

static uint8_t vw_rx_pin;
static uint8_t vw_rx_sample = 0;
static uint8_t vw_rx_last_sample = 0;
static uint8_t vw_rx_pll_ramp = 0;
static uint8_t vw_rx_integrator = 0;
static uint8_t vw_rx_active = 0;
static volatile uint8_t vw_rx_done = 0;
static uint8_t vw_rx_enabled = 0;
static uint16_t vw_rx_bits = 0;
static uint8_t vw_rx_bit_count = 0;
static uint8_t vw_rx_buf[VW_MAX_MESSAGE_LEN];
static uint8_t vw_rx_count = 0;
static volatile uint8_t vw_rx_len = 0;
static uint8_t vw_rx_bad = 0;
static uint8_t vw_rx_good = 0;
static uint8_t symbols[] =
{
    0xd,  0xe,  0x13, 0x15, 0x16, 0x19, 0x1a, 0x1c,
    0x23, 0x25, 0x26, 0x29, 0x2a, 0x2c, 0x32, 0x34
};

uint16_t vw_crc(uint8_t *ptr, uint8_t count)
{
    uint16_t crc = 0xffff;

    while (count-- > 0)
    crc = _crc_ccitt_update(crc, *ptr++);
    return crc;
}

uint8_t vw_symbol_6to4(uint8_t symbol)
{
    uint8_t i;
    
    // Linear search :-( Could have a 64 byte reverse lookup table?
    for (i = 0; i < 16; i++)
    if (symbol == symbols[i]) return i;
    return 0; // Not found
}

void vw_set_rx_pin(uint8_t pin)
{
    vw_rx_pin = pin;
}

void vw_pll()
{
    // Integrate each sample
    if (vw_rx_sample)
    vw_rx_integrator++;

    if (vw_rx_sample != vw_rx_last_sample)
    {
        // Transition, advance if ramp > 80, retard if < 80
        vw_rx_pll_ramp += ((vw_rx_pll_ramp < VW_RAMP_TRANSITION)
        ? VW_RAMP_INC_RETARD
        : VW_RAMP_INC_ADVANCE);
        vw_rx_last_sample = vw_rx_sample;
    }
    else
    {
        // No transition
        // Advance ramp by standard 20 (== 160/8 samples)
        vw_rx_pll_ramp += VW_RAMP_INC;
    }
    if (vw_rx_pll_ramp >= VW_RX_RAMP_LEN)
    {
        // Add this to the 12th bit of vw_rx_bits, LSB first
        // The last 12 bits are kept
        vw_rx_bits >>= 1;

        // Check the integrator to see how many samples in this cycle were high.
        // If < 5 out of 8, then its declared a 0 bit, else a 1;
        if (vw_rx_integrator >= 5)
        vw_rx_bits |= 0x800;

        vw_rx_pll_ramp -= VW_RX_RAMP_LEN;
        vw_rx_integrator = 0; // Clear the integral for the next cycle

        if (vw_rx_active)
        {
            // We have the start symbol and now we are collecting message bits,
            // 6 per symbol, each which has to be decoded to 4 bits
            if (++vw_rx_bit_count >= 12)
            {
                // Have 12 bits of encoded message == 1 byte encoded
                // Decode as 2 lots of 6 bits into 2 lots of 4 bits
                // The 6 lsbits are the high nybble
                uint8_t this_byte =
                (vw_symbol_6to4(vw_rx_bits & 0x3f)) << 4
                | vw_symbol_6to4(vw_rx_bits >> 6);

                // The first decoded byte is the byte count of the following message
                // the count includes the byte count and the 2 trailing FCS bytes
                // REVISIT: may also include the ACK flag at 0x40
                if (vw_rx_len == 0)
                {
                    // The first byte is the byte count
                    // Check it for sensibility. It cant be less than 4, since it
                    // includes the bytes count itself and the 2 byte FCS
                    vw_rx_count = this_byte;
                    if (vw_rx_count < 4 || vw_rx_count > VW_MAX_MESSAGE_LEN)
                    {
                        // Stupid message length, drop the whole thing
                        vw_rx_active = false;
                        vw_rx_bad++;
                        return;
                    }
                }
                vw_rx_buf[vw_rx_len++] = this_byte;

                if (vw_rx_len >= vw_rx_count)
                {
                    // Got all the bytes now
                    vw_rx_active = false;
                    vw_rx_good++;
                    vw_rx_done = true; // Better come get it before the next one starts
                }
                vw_rx_bit_count = 0;
            }
        }
        // Not in a message, see if we have a start symbol
        else if (vw_rx_bits == 0xb38)
        {
            // Have start symbol, start collecting message
            vw_rx_active = true;
            vw_rx_bit_count = 0;
            vw_rx_len = 0;
            vw_rx_done = false; // Too bad if you missed the last message
        }
    }
}

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

void vw_setup(uint16_t speed)
{
    uint16_t nticks; // number of prescaled ticks needed
    uint8_t prescaler; // Bit values for CS0[2:0]

    prescaler = _timer_calc(speed, (uint16_t)-1, &nticks);

    if (!prescaler)
    {
        return; // fault
    }

    TCCR1A = 0; // Output Compare pins disconnected
    TCCR1B = _BV(WGM12); // Turn on CTC mode

    // convert prescaler index to TCCRnB prescaler bits CS10, CS11, CS12
    TCCR1B |= prescaler;

    // Caution: special procedures for setting 16 bit regs
    // is handled by the compiler
    OCR1A = nticks;
    // Enable interrupt

    TIMSK1 |= _BV(OCIE1A);

    vw_ddr &= ~(1<<vw_rx_pin);
    vw_port &= ~(1<<vw_rx_pin);
    
}

void vw_rx_start()
{
    if (!vw_rx_enabled)
    {
    vw_rx_enabled = true;
    vw_rx_active = false; // Never restart a partial message
    }
}

void vw_rx_stop()
{
    vw_rx_enabled = false;
}

void vw_wait_rx()
{
    while (!vw_rx_done)
    ;
}

uint8_t vw_have_message()
{
    return vw_rx_done;
}

uint8_t vw_get_message(uint8_t* buf, uint8_t* len)
{
    uint8_t rxlen;
    
    // Message available?
    if (!vw_rx_done)
    return false;
    
    // Wait until vw_rx_done is set before reading vw_rx_len
    // then remove bytecount and FCS
    rxlen = vw_rx_len - 3;
    
    // Copy message (good or bad)
    if (*len > rxlen)
    *len = rxlen;
    memcpy(buf, vw_rx_buf + 1, *len);
    
    vw_rx_done = false; // OK, got that message thanks
    
    // Check the FCS, return goodness
    return (vw_crc(vw_rx_buf, vw_rx_len) == 0xf0b8); // FCS OK?
}

ISR (TIMER1_COMPA_vect)
{
    if (vw_rx_enabled)
    {
    if(bit_is_set(vw_pin,vw_rx_pin))
    {
        vw_rx_sample = 1;
    }
    else
    {
        vw_rx_sample = 0;
    }
    }
    if (vw_rx_enabled)
    vw_pll();
}
