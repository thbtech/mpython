/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Chester Tseng
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


#include "device.h"
#include "serial_api.h"

#define UART_TX    PA_7
#define UART_RX    PA_6

static serial_t sobj;

void mp_hal_uart_init(void)
{
    serial_init(&sobj, UART_TX, UART_RX);
    serial_baud(&sobj, 9600);
    serial_format(&sobj, 8, ParityNone, 1);
}

int mp_hal_stdin_rx_chr(void) {
    for (;;) {
        int c = serial_getc(&sobj);
        if (c != -1) {
            return c;
        }
    }
}

void mp_hal_stdout_tx_str(const char *str) {
    while (*str) {
        serial_putc(&sobj, *str++);
    }
}

void mp_hal_stdout_tx_strn(const char *str, uint32_t len) {
    while (len--) {
        serial_putc(&sobj, *str++);
    }
}

void mp_hal_stdout_tx_strn_cooked(const char *str, uint32_t len) {
    while (len--) {
        if (*str == '\n') {
            serial_putc(&sobj, '\r');
        }
        serial_putc(&sobj, *str++);
    }
}

void mp_hal_set_interrupt_char(int c) {
    // TODO
}
