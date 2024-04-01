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
#include "BlueNRG1_sysCtrl.h"

// Implemented functions -------------------------------------------------------

/**
 * @brief Init clock, power and main gpio of the board.
 */
void boardInit()
{
    SystemInit();


    // Init Gpio
    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_GPIO, ENABLE);

    GPIO_InitType GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);


    GPIO_ResetBits(LED_PIN);
    GPIO_InitStruct.GPIO_Pin = LED_PIN;
    GPIO_InitStruct.GPIO_Mode = LED_MODE;
    GPIO_Init(&GPIO_InitStruct);
}
