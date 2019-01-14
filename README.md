# RVK_attiny_sensor
A simple attiny13 wireless sensor with ds18b20 and virtualwire

Here is a simple project based on Attiny13a ( which is limited with 1kb flash and 64bytes of SRAM). In this project a simple sensor node is made out of this and collect data from a digital temperature sensor (ds18b20) using one wire protocol and the ambient light with an ldr and converted by the ADC(analogue to digital converter) inside the attiny. All collect data is send to the receiver via wireless link based on 433mhz OOK modules which needs a protocol to send the data (unlike the dedicated wireless chips with built in protocols and error corrections)

More details: http://www.riyas.org/2019/01/attiny13-wireless-temperature-433mhz-ds18b20-watchdog.html

