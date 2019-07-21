/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Damien P. George
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
#ifndef MICROPY_INCLUDED_STM32_POWERCTRL_H
#define MICROPY_INCLUDED_STM32_POWERCTRL_H

#include <stdint.h>

// 软件复位mcu
NORETURN void powerctrl_mcu_reset(void);

// 进入bootloader
NORETURN void powerctrl_enter_bootloader(uint32_t r0, uint32_t bl_addr);

// 检查是否通过 powerctl_mcu_reset 复位，是的话进入bootloader
void powerctrl_check_enter_bootloader(void);

// 配置系统时钟
int powerctrl_rcc_clock_config_pll(RCC_ClkInitTypeDef *rcc_init);

// 复位系统时钟
int powerctrl_set_sysclk(uint32_t sysclk, uint32_t ahb, uint32_t apb1, uint32_t apb2);

// 进入停止模式
void powerctrl_enter_stop_mode(void);

// 进入待机模式
void powerctrl_enter_standby_mode(void);

#endif // MICROPY_INCLUDED_STM32_POWERCTRL_H
