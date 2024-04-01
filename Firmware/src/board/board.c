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

#include "drivers/uart.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Global variables ------------------------------------------------------------
uart_t _serial;
static uint8_t _serialBufTx[UART_TX_FIFO_SIZE];
static uint8_t _serialBufRx[UART_RX_FIFO_SIZE];

// Prototype functions ---------------------------------------------------------
void _boardInitGpio();
void _boardInitUart();

// Implemented functions -------------------------------------------------------

/**
 * @brief Init clock, power and main gpio of the board.
 */
void boardInit()
{
    // Ini system
    SystemInit();

    _boardInitGpio();
    _boardInitUart();
}

void _boardInitGpio()
{
    // Init Gpio
    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_GPIO, ENABLE);

    GPIO_InitType GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);

    // Init Led pin
    GPIO_ResetBits(LED_PIN);
    GPIO_InitStruct.GPIO_Pin = LED_PIN;
    GPIO_InitStruct.GPIO_Mode = LED_MODE;
    GPIO_Init(&GPIO_InitStruct);

    // Init Rx Pin
    GPIO_InitStruct.GPIO_Pin = UART_RX_PIN;
    GPIO_InitStruct.GPIO_Mode = UART_RX_MODE;
    GPIO_Init(&GPIO_InitStruct);

    // Init Tx Pin
    GPIO_InitStruct.GPIO_Pin = UART_TX_PIN;
    GPIO_InitStruct.GPIO_Mode = UART_TX_MODE;
    GPIO_Init(&GPIO_InitStruct);
}

void _boardInitUart()
{
    // Init uart
    uart_init(&_serial, UART,
              _serialBufTx, sizeof(_serialBufTx),
              _serialBufRx, sizeof(_serialBufRx));

    uart_config(&_serial,
                UART_BAUDRATE_115200,
                UART_DATA_8BITS,
                UART_PARITY_NO,
                UART_STOPBIT_1);
}

int boardPrintf(char const *format, ...)
{
    va_list ap;
    char str[128];

    va_start(ap, format);
    int n = vsnprintf(str, sizeof(str), format, ap);
    va_end(ap);

    return uart_write(&_serial, str, n);
}