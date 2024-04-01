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

#ifndef BLUENRG_IT_H
#define BLUENRG_IT_H

// Includes --------------------------------------------------------------------
// #include "tick.h"
// #include "uart.h"
// #include "bluenrg1_stack.h"
// #include "board.h"

// Implemented functions ---------------------------------------------------------
// void SysTick_Handler()
// {
//     _tick_isr_handler();
// }

// void UART_Handler()
// {
//     _uart_isr_handler();
// }

// void GPIO_Handler(void)
// {
//     if (GPIO_GetITPendingBit(BOND_PIN) == SET)
//     {
//         GPIO_ClearITPendingBit(BOND_PIN);

//         if (GPIO_ReadBit(BOND_PIN) == Bit_SET)
//         {
//             GPIO_ToggleBits(LED_PIN);

//             GPIO_InitType GPIO_InitStruct;
//             GPIO_StructInit(&GPIO_InitStruct);
//             GPIO_InitStruct.GPIO_Pin = LOCK_PIN;
//             GPIO_InitStruct.GPIO_Mode = (GPIO_ReadBit(LED_PIN) == Bit_SET) ? GPIO_Input : GPIO_Output;
//             GPIO_Init(&GPIO_InitStruct);
//         }
//     }
// }

// void Blue_Handler()
// {
//     RAL_Isr();
// }

void empru(){}

#endif // BLUENRG_IT_H
