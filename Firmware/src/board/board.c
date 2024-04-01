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

// Includes --------------------------------------------------------------------
#include "board.h"
#include "tick.h"
#include "pwm.h"

#include "BlueNRG1_sysCtrl.h"
#include "misc.h"

// Implemented functions -------------------------------------------------------

/**
 * @brief Init clock, power and main gpio of the board.
 */
void boardInit()
{
    // Init tick
    tick_init();

    // Init Gpio
    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_GPIO, ENABLE);

    GPIO_InitType GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);

    // IN
    GPIO_InitStruct.GPIO_Pin = BOND_PIN;
    GPIO_InitStruct.GPIO_Mode = BOND_MODE;
    GPIO_Init(&GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = OPENED_PIN;
    GPIO_InitStruct.GPIO_Mode = OPENED_MODE;
    GPIO_Init(&GPIO_InitStruct);

    // IN / OUT
    GPIO_ResetBits(LOCK_PIN);
    GPIO_InitStruct.GPIO_Pin = LOCK_PIN;
    GPIO_InitStruct.GPIO_Mode = LOCK_MODE_OUT;
    GPIO_Init(&GPIO_InitStruct);



//---

            GPIO_StructInit(&GPIO_InitStruct);
            GPIO_InitStruct.GPIO_Pin = LOCK_PIN;
            GPIO_InitStruct.GPIO_Mode = GPIO_Input;
            GPIO_Init(&GPIO_InitStruct);
//--






    // OUT
    GPIO_ResetBits(LED_PIN);
    GPIO_InitStruct.GPIO_Pin = LED_PIN;
    GPIO_InitStruct.GPIO_Mode = LED_MODE;
    GPIO_Init(&GPIO_InitStruct);

    GPIO_ResetBits(EN_IO_PIN);
    GPIO_InitStruct.GPIO_Pin = EN_IO_PIN;
    GPIO_InitStruct.GPIO_Mode = EN_IO_MODE;
    GPIO_Init(&GPIO_InitStruct);
#if 0
    // LIGHT
    GPIO_ResetBits(LIGHT_RED_PIN);
    GPIO_InitStruct.GPIO_Pin = LIGHT_RED_PIN;
    GPIO_InitStruct.GPIO_Mode = LIGHT_RED_MODE;
    GPIO_Init(&GPIO_InitStruct);

    GPIO_ResetBits(LIGHT_GREEN_PIN);
    GPIO_InitStruct.GPIO_Pin = LIGHT_GREEN_PIN;
    GPIO_InitStruct.GPIO_Mode = LIGHT_GREEN_MODE;
    GPIO_Init(&GPIO_InitStruct);

    GPIO_ResetBits(LIGHT_BLUE_PIN);
    GPIO_InitStruct.GPIO_Pin = LIGHT_BLUE_PIN;
    GPIO_InitStruct.GPIO_Mode = LIGHT_BLUE_MODE;
    GPIO_Init(&GPIO_InitStruct);

    GPIO_ResetBits(LIGHT_WHITE_PIN);
    GPIO_InitStruct.GPIO_Pin = LIGHT_WHITE_PIN;
    GPIO_InitStruct.GPIO_Mode = LIGHT_WHITE_MODE;
    GPIO_Init(&GPIO_InitStruct);

    GPIO_ResetBits(LIGHT_PWM_PIN);
    GPIO_InitStruct.GPIO_Pin = LIGHT_PWM_PIN;
    GPIO_InitStruct.GPIO_Mode = LIGHT_PWM_MODE_OUT;
    GPIO_Init(&GPIO_InitStruct);
#else
    // LIGHT
    GPIO_InitStruct.GPIO_Pin =
        LIGHT_RED_PIN | LIGHT_GREEN_PIN | LIGHT_BLUE_PIN | LIGHT_WHITE_PIN | LIGHT_PWM_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Output;
    GPIO_Init(&GPIO_InitStruct);
#endif

    // /* Set the GPIO interrupt priority and enable it */
    // NVIC_InitType NVIC_InitStructure;
    // NVIC_InitStructure.NVIC_IRQChannel = GPIO_IRQn;
    // NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = LOW_PRIORITY;
    // NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    // NVIC_Init(&NVIC_InitStructure);

    // /* Configures EXTI line for BOND_PIN */
    // GPIO_EXTIConfigType GPIO_EXTIStructure;
    // GPIO_EXTIStructure.GPIO_Pin = BOND_PIN;
    // GPIO_EXTIStructure.GPIO_IrqSense = GPIO_IrqSense_Edge;
    // GPIO_EXTIStructure.GPIO_Event = GPIO_Event_Both;
    // GPIO_EXTIConfig(&GPIO_EXTIStructure);

    // /* Clear pending interrupt */
    // GPIO_ClearITPendingBit(BOND_PIN);

    // /* Enable the interrupt */
    // GPIO_EXTICmd(BOND_PIN, ENABLE);
}

void boardLight(light_t light, unsigned int duty_cycle)
{

    if (duty_cycle != 0)
    {
        if (MFT2->TNMCTRL_b.TNEN == RESET)
        {
            GPIO_InitType GPIO_InitStruct;
            GPIO_StructInit(&GPIO_InitStruct);
            GPIO_InitStruct.GPIO_Pin = LIGHT_PWM_PIN;
            GPIO_InitStruct.GPIO_Mode = LIGHT_PWM_MODE_PWM;
            GPIO_Init(&GPIO_InitStruct);

            pwm_init();
        }

        //  MFT_Cmd(MFT2, DISABLE);

        GPIO_ResetBits(LIGHT_WHITE_PIN);
        GPIO_ResetBits(LIGHT_RED_PIN);
        GPIO_ResetBits(LIGHT_GREEN_PIN);
        GPIO_ResetBits(LIGHT_BLUE_PIN);

        switch (light)
        {
        case LIGHT_RED:
        {
            pwm_setDutyCycle(100 - duty_cycle);

            // GPIO_ResetBits(LIGHT_WHITE_PIN);

            GPIO_SetBits(LIGHT_RED_PIN);
            // GPIO_ResetBits(LIGHT_GREEN_PIN);
            // GPIO_ResetBits(LIGHT_BLUE_PIN);
            break;
        }

        case LIGHT_GREEN:
        {
            pwm_setDutyCycle(100 - duty_cycle);

            // GPIO_ResetBits(LIGHT_WHITE_PIN);

            // GPIO_ResetBits(LIGHT_RED_PIN);
            GPIO_SetBits(LIGHT_GREEN_PIN);
            // GPIO_ResetBits(LIGHT_BLUE_PIN);
            break;
        }

        case LIGHT_BLUE:
        {
            pwm_setDutyCycle(100 - duty_cycle);

            // GPIO_ResetBits(LIGHT_WHITE_PIN);

            // GPIO_ResetBits(LIGHT_RED_PIN);
            // GPIO_ResetBits(LIGHT_GREEN_PIN);
            GPIO_SetBits(LIGHT_BLUE_PIN);
            break;
        }

        default:
        case LIGHT_WHITE:
        {
            pwm_setDutyCycle(duty_cycle);
            MFT_SetCounter1(MFT2, 0);
            MFT_SetCounter2(MFT2, 0);

            // GPIO_ResetBits(LIGHT_RED_PIN);
            // GPIO_ResetBits(LIGHT_GREEN_PIN);
            // GPIO_ResetBits(LIGHT_BLUE_PIN);

            GPIO_SetBits(LIGHT_WHITE_PIN);
            break;
        }
        }

        //  MFT_Cmd(MFT2, ENABLE);
    }
    else
    {
        pwm_deinit();

        GPIO_InitType GPIO_InitStruct;
        GPIO_StructInit(&GPIO_InitStruct);
        GPIO_InitStruct.GPIO_Pin = LIGHT_PWM_PIN;
        GPIO_InitStruct.GPIO_Mode = LIGHT_PWM_MODE_OUT;
        GPIO_Init(&GPIO_InitStruct);
    }

    // Serial1_Mode
}
