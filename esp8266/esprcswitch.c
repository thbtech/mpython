/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>

#include "etshal.h"
#include "user_interface.h"
#include "modpyb.h"
#include "esprcswitch.h"

static uint32_t disable_irq(void) {
	ets_intr_lock();
	return 0;
}

static void enable_irq(uint32_t i) {
	ets_intr_unlock();
}

static void mp_hal_delay_us_no_irq(uint32_t us) {
	uint32_t start = system_get_time();
	while (system_get_time() - start < us) {
	}
}

#define DELAY_US mp_hal_delay_us_no_irq

int esp_rcswitch_send(uint pin, int val) {
	uint32_t i = disable_irq();
	int nRepeat=0;
	for (nRepeat=0; nRepeat<10; nRepeat++) {
		unsigned int mask = 16777216;
		while (mask >>= 1)
		{
			if (!!(mask & val)==0)
			{
				send0(pin);
			}
			if (!!(mask & val)==1)
			{
				send1(pin);
			}
		}
		sendsync(pin);
	}
	sendterm(pin);
	enable_irq(i);
	return 0;
}

/* possible better alternative:
int main()
{
        int val = 3847937;
        int i = 0;
        for(i = 0; i < 32; i++)
        {
                printf("%i, %i\n",i,val%2);
                val >>= 1;
        }
        return 0;
}
*/

int esp_rcswitch_readbit(uint pin) {
	pin_set(pin,1);
	return 0;
}

void esp_rcswitch_writebit(uint pin, int value) {
	pin_set(pin, 0);
}

void send_pulse(uint pin, int highs, int lows)
{
	pin_set(pin,1);
	DELAY_US(350*highs);
	pin_set(pin,0);
	DELAY_US(350*lows);
}
void send1(uint pin)
{
	send_pulse(pin,3,1);
}
void send0(uint pin)
{
	send_pulse(pin,1,3);
}
void sendsync(uint pin)
{
	send_pulse(pin,1,32);
}
void sendterm(uint pin)
{
	send_pulse(pin,1,256);
}
void send(const char* sCodeWord, uint pin) 
{
	int nRepeat=0;
	for (nRepeat=0; nRepeat<10; nRepeat++) {
		int i = 0;
		while (sCodeWord[i] != '\0') 
		{
			if (sCodeWord[i] == '0')
			{
				send0(pin);
			}
			if (sCodeWord[i] == '1')
			{
				send1(pin);
			}
			i++;
		}
		sendsync(pin);
	}
	sendterm(pin);
}
