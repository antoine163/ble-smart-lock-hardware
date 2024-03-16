/***
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 antoine163
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file pwm.c
 * @author antoine163
 * @date 24.02.2024
 * @brief PWM driver
 */

// Include ---------------------------------------------------------------------
#include "adc.h"

#include "BlueNRG1_sysCtrl.h"
#include "BlueNRG1_mft.h"

// Implemented functions -------------------------------------------------------
int pwm_init()
{
    MFT_InitType timer_init;

    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_MTFX1 | CLOCK_PERIPH_MTFX2, ENABLE);

    MFT_StructInit(&timer_init);

    timer_init.MFT_Mode = MFT_MODE_1;

#if (HS_SPEED_XTAL == HS_SPEED_XTAL_32MHZ)
    timer_init.MFT_Prescaler = 160 - 1; /* 5 us clock */
#elif (HS_SPEED_XTAL == HS_SPEED_XTAL_16MHZ)
    timer_init.MFT_Prescaler = 80 - 1; /* 5 us clock */
#endif

    /* MFT2 configuration */
    timer_init.MFT_Clock1 = MFT_PRESCALED_CLK;
    timer_init.MFT_Clock2 = MFT_NO_CLK;
    timer_init.MFT_CRA = 1000 - 1; /* 5 ms positive duration */
    timer_init.MFT_CRB = 1000 - 1; /* 5 ms negative duration */
    MFT_Init(MFT2, &timer_init);

    /* Connect PWM output from MFT2 to TnA pin (PWM1) */
    MFT_TnXEN(MFT2, MFT_TnA, ENABLE);

    /* Start MFT timers */
    MFT_Cmd(MFT2, ENABLE);

    return 0;
}

int pwm_deinit()
{
    MFT_TnXEN(MFT2, MFT_TnA, DISABLE);
    MFT_DeInit(MFT2);
    return 0;
}

void pwm_setDutyCycle(float duty_cycle)
{
    uint32_t tmcra = (duty_cycle * 2000.) / 100.;
    uint32_t tmcrb = 2000UL - tmcra;


    MFT2->TNCRA = (uint16_t)tmcra;
    MFT2->TNCRB = (uint16_t)tmcrb;
}