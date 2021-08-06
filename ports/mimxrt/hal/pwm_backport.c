/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * The MIT License (MIT)
 * Copyright (c) 2021 Robert Hammelrath
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

// These are a few functions taken from the NXP-Lib
// for PWM, for
// - dealing with an u16 duty cycle setting,
// - setting the pulse center position, and
// - factoring out pure duty cycle change.

#include "py/runtime.h"
#include "fsl_pwm.h"
#include "fsl_qtmr.h"
#include "hal/pwm_backport.h"

void PWM_UpdatePwmDutycycle_u16(
    PWM_Type *base, pwm_submodule_t subModule, pwm_channels_t pwmSignal, uint16_t dutyCycle, uint16_t Center_u16) {
    assert((uint16_t)pwmSignal < 2U);
    uint16_t pulseCnt = 0, pwmHighPulse = 0;
    uint16_t center;

    // check and confine bounds for Center_u16
    if ((Center_u16 + dutyCycle / 2) >= PWM_FULL_SCALE) {
        Center_u16 = PWM_FULL_SCALE - dutyCycle / 2;
    } else if (Center_u16 < (dutyCycle / 2)) {
        Center_u16 = dutyCycle / 2;
    }
    pulseCnt = base->SM[subModule].VAL1;
    // Calculate pulse width and center position
    pwmHighPulse = ((pulseCnt + 1) * dutyCycle) / PWM_FULL_SCALE;
    center = ((pulseCnt + 1) * Center_u16) / PWM_FULL_SCALE;

    // Setup the PWM dutycycle of channel A or B
    if (pwmSignal == kPWM_PwmA) {
        base->SM[subModule].VAL2 = center - pwmHighPulse / 2;
        base->SM[subModule].VAL3 = center + pwmHighPulse / 2;
    } else {
        base->SM[subModule].VAL4 = center - pwmHighPulse / 2;
        base->SM[subModule].VAL5 = center + pwmHighPulse / 2;
    }
}

void PWM_SetupPwm_u16(PWM_Type *base, pwm_submodule_t subModule, pwm_signal_param_u16_t *chnlParams,
    uint32_t pwmFreq_Hz, uint32_t srcClock_Hz) {

    uint32_t pwmClock;
    uint16_t pulseCnt = 0;
    uint8_t polarityShift = 0, outputEnableShift = 0;

    // Divide the clock by the prescale value
    pwmClock = (srcClock_Hz / (1U << ((base->SM[subModule].CTRL & PWM_CTRL_PRSC_MASK) >> PWM_CTRL_PRSC_SHIFT)));
    pulseCnt = (pwmClock / pwmFreq_Hz);
    base->SM[subModule].INIT = 0;
    // base->SM[subModule].VAL0 = (pulseCnt / 2);
    base->SM[subModule].VAL1 = pulseCnt;

    // Set up the Registers VAL2..VAL5 controlling the duty cycle of channel A/B
    PWM_UpdatePwmDutycycle_u16(base, subModule, chnlParams->pwmChannel,
        chnlParams->dutyCycle_u16, chnlParams->Center_u16);

    // Setup register shift values based on the channel being configured.
    // Also setup the deadtime value
    if (chnlParams->pwmChannel == kPWM_PwmA) {
        polarityShift              = PWM_OCTRL_POLA_SHIFT;
        outputEnableShift          = PWM_OUTEN_PWMA_EN_SHIFT;
        base->SM[subModule].DTCNT0 = PWM_DTCNT0_DTCNT0(chnlParams->deadtimeValue);
    } else {
        polarityShift              = PWM_OCTRL_POLB_SHIFT;
        outputEnableShift          = PWM_OUTEN_PWMB_EN_SHIFT;
        base->SM[subModule].DTCNT1 = PWM_DTCNT1_DTCNT1(chnlParams->deadtimeValue);
    }

    // Setup signal active level
    if (chnlParams->level == kPWM_HighTrue) {
        base->SM[subModule].OCTRL &= ~(1U << polarityShift);
    } else {
        base->SM[subModule].OCTRL |= (1U << polarityShift);
    }
    // Enable PWM output
    base->OUTEN |= (1U << (outputEnableShift + subModule));
}

void PWM_SetupPwmx_u16(PWM_Type *base, pwm_submodule_t subModule,
    uint32_t pwmFreq_Hz, uint16_t duty_cycle, uint8_t invert, uint32_t srcClock_Hz) {

    uint32_t pulseCnt;
    uint32_t pwmClock;

    // Divide the clock by the prescale value
    pwmClock = (srcClock_Hz / (1U << ((base->SM[subModule].CTRL & PWM_CTRL_PRSC_MASK) >> PWM_CTRL_PRSC_SHIFT)));
    pulseCnt = pwmClock / pwmFreq_Hz;
    base->SM[subModule].INIT = 0;
    base->SM[subModule].VAL0 = ((uint32_t)duty_cycle * pulseCnt) / PWM_FULL_SCALE;
    base->SM[subModule].VAL1 = pulseCnt;

    base->SM[subModule].OCTRL = (base->SM[subModule].OCTRL & ~PWM_OCTRL_POLX_MASK) | PWM_OCTRL_POLX(!invert);

    base->OUTEN |= (1U << subModule);
}

void PWM_SetupFaultDisableMap(PWM_Type *base, pwm_submodule_t subModule,
    pwm_channels_t pwmChannel, pwm_fault_channels_t pwm_fault_channels, uint16_t value) {
    uint16_t reg = base->SM[subModule].DISMAP[pwm_fault_channels];
    switch (pwmChannel)  {
        case kPWM_PwmA:
            reg &= ~((uint16_t)PWM_DISMAP_DIS0A_MASK);
            reg |= (((uint16_t)(value) << (uint16_t)PWM_DISMAP_DIS0A_SHIFT) & (uint16_t)PWM_DISMAP_DIS0A_MASK);
            break;
        case kPWM_PwmB:
            reg &= ~((uint16_t)PWM_DISMAP_DIS0B_MASK);
            reg |= (((uint16_t)(value) << (uint16_t)PWM_DISMAP_DIS0B_SHIFT) & (uint16_t)PWM_DISMAP_DIS0B_MASK);
            break;
        case kPWM_PwmX:
            reg &= ~((uint16_t)PWM_DISMAP_DIS0X_MASK);
            reg |= (((uint16_t)(value) << (uint16_t)PWM_DISMAP_DIS0X_SHIFT) & (uint16_t)PWM_DISMAP_DIS0X_MASK);
            break;
        default:
            assert(false);
            break;
    }
    base->SM[subModule].DISMAP[pwm_fault_channels] = reg;
}

status_t QTMR_SetupPwm_u16(TMR_Type *base, qtmr_channel_selection_t channel, uint32_t pwmFreqHz,
    uint16_t dutyCycleU16, bool outputPolarity, uint32_t srcClock_Hz) {
    uint32_t periodCount, highCount, lowCount, reg;

    if (dutyCycleU16 >= PWM_FULL_SCALE) {
        // Invalid dutycycle
        return kStatus_Fail;
    }

    // Set OFLAG pin for output mode and force out a low on the pin
    base->CHANNEL[channel].SCTRL |= (TMR_SCTRL_FORCE_MASK | TMR_SCTRL_OEN_MASK);

    // Counter values to generate a PWM signal
    periodCount = (srcClock_Hz / pwmFreqHz);
    highCount   = (periodCount * dutyCycleU16) / PWM_FULL_SCALE;
    lowCount    = periodCount - highCount;

    // Setup the compare registers for PWM output
    base->CHANNEL[channel].COMP1 = lowCount;
    base->CHANNEL[channel].COMP2 = highCount;

    // Setup the pre-load registers for PWM output
    base->CHANNEL[channel].CMPLD1 = lowCount;
    base->CHANNEL[channel].CMPLD2 = highCount;

    reg = base->CHANNEL[channel].CSCTRL;
    // Setup the compare load control for COMP1 and COMP2.
    // Load COMP1 when CSCTRL[TCF2] is asserted, load COMP2 when CSCTRL[TCF1] is asserted
    reg &= ~(TMR_CSCTRL_CL1_MASK | TMR_CSCTRL_CL2_MASK);
    reg |= (TMR_CSCTRL_CL1(kQTMR_LoadOnComp2) | TMR_CSCTRL_CL2(kQTMR_LoadOnComp1));
    base->CHANNEL[channel].CSCTRL = reg;

    if (outputPolarity) {
        // Invert the polarity
        base->CHANNEL[channel].SCTRL |= TMR_SCTRL_OPS_MASK;
    } else {
        // True polarity, no inversion
        base->CHANNEL[channel].SCTRL &= ~TMR_SCTRL_OPS_MASK;
    }

    reg = base->CHANNEL[channel].CTRL;
    reg &= ~(TMR_CTRL_OUTMODE_MASK);
    // Count until compare value is  reached and re-initialize the counter, toggle OFLAG output
    // using alternating compare register
    reg |= (TMR_CTRL_LENGTH_MASK | TMR_CTRL_OUTMODE(kQTMR_ToggleOnAltCompareReg));
    base->CHANNEL[channel].CTRL = reg;

    return kStatus_Success;
}