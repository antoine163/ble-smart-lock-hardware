/***
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 antoine163
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

#include "BlueNRG1_sysCtrl.h"

// Implemented functions -------------------------------------------------------

/**
 * @brief Init clock, power and main gpio of the board.
 */
void boardInit()
{
    // Init tick
    tick_init();

    // Init Led Gpio
    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_GPIO, ENABLE);
    GPIO_InitType GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin       = LED1_PIN;
    GPIO_InitStruct.GPIO_Mode      = LED1_MODE;
    GPIO_InitStruct.GPIO_HighPwr   = DISABLE;
    GPIO_InitStruct.GPIO_Pull      = DISABLE;
    GPIO_Init(&GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin       = LED2_PIN;
    GPIO_InitStruct.GPIO_Mode      = LED2_MODE;
    GPIO_Init(&GPIO_InitStruct);

    // Init BPs
    GPIO_InitStruct.GPIO_Pin       = BUTTON1_PIN;
    GPIO_InitStruct.GPIO_Mode      = BUTTON1_MODE;
    GPIO_Init(&GPIO_InitStruct);
}
