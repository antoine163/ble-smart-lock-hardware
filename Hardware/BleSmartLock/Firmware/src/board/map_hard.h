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
 
#ifndef MAP_HARD_H
#define MAP_HARD_H

// LEDs
#define LED1_PIN             GPIO_Pin_6
#define LED1_MODE            GPIO_Output
#define LED2_PIN             GPIO_Pin_14
#define LED2_MODE            GPIO_Output

// BUTTONs
#define BUTTON1_PIN             GPIO_Pin_4
#define BUTTON1_MODE            GPIO_Input

// UART
#define UART_RX_PIN        GPIO_Pin_11
#define UART_RX_FIFO_SIZE   16


#define UART_TX_PIN        GPIO_Pin_8
#define UART_TX_FIFO_SIZE   16

#endif // MAP_HARD_H
