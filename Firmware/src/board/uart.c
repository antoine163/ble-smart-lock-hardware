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

/**
 * @file uart.c
 * @author antoine163
 * @date 15.07.2023
 * @brief It is a UART driver
 */

// Include ---------------------------------------------------------------------
#include "uart.h"
#include "tick.h"

#include "BlueNRG1_sysCtrl.h"
#include "BlueNRG1_gpio.h"
#include "misc.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Global variables ------------------------------------------------------------
#if ( defined(UART_RX_GPIO) || defined(UART_TX_GPIO) ) && !defined(UART)
    #error "UART_RX_GPIO or/and UART_TX_GPIO is defined in mapHard.h but the UART is not available on your bluenrg !"
#endif

// Manage possessor errors -----------------------------------------------------
#if defined(UART_TX_PIN) && !defined(UART_TX_FIFO_SIZE)
    #error "UART_TX_PIN is defined in mapHard.h but UART_TX_FIFO_SIZE is not !"
#elif defined(UART_TX_FIFO_SIZE)
    static uint8_t _uart_fifoTxBuffer[UART_TX_FIFO_SIZE];
     fifo_t _uart_fifoTx =
        FIFO_INIT(   _uart_fifoTxBuffer,
                        UART_TX_FIFO_SIZE);
#endif

#if defined(UART_RX_PIN) && !defined(UART_RX_FIFO_SIZE)
#error "UART_RX_PIN is defined in mapHard.h but UART_RX_FIFO_SIZE is not !"
#elif defined(UART_RX_FIFO_SIZE)
    static uint8_t _uart_fifoRxBuffer[UART_RX_FIFO_SIZE];
    fifo_t _uart_fifoRx =
        FIFO_INIT(   _uart_fifoRxBuffer,
                  UART_RX_FIFO_SIZE);
#endif


// Implemented functions -------------------------------------------------------
int uart_init()
{
    GPIO_InitType gpioConf;
    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_UART | CLOCK_PERIPH_GPIO, ENABLE);

#ifdef UART_RX_PIN
    // Init Rx Pin
    gpioConf.GPIO_Pin       = UART_RX_PIN;
    gpioConf.GPIO_Mode      = Serial1_Mode;
    gpioConf.GPIO_HighPwr   = DISABLE;
    gpioConf.GPIO_Pull      = DISABLE;
    GPIO_Init(&gpioConf);

    // Init Rx FiFo
    fifo_clean(&_uart_fifoRx);
#endif

#ifdef UART_TX_PIN
    // Init Tx Pin
    gpioConf.GPIO_Pin       = UART_TX_PIN;
    gpioConf.GPIO_Mode      = Serial1_Mode;
    gpioConf.GPIO_HighPwr   = DISABLE;
    gpioConf.GPIO_Pull      = DISABLE;
    GPIO_Init(&gpioConf);

    // Init Tx FiFo
    fifo_clean(&_uart_fifoTx);
#endif

    // Enable UART Interrupt
    NVIC_InitType nvicConfig;
    nvicConfig.NVIC_IRQChannel = UART_IRQn;
    nvicConfig.NVIC_IRQChannelPreemptionPriority = LOW_PRIORITY;
    nvicConfig.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvicConfig);

    return 0;
}

int uart_deinit()
{
    // Disable UART Interrupt
    NVIC_InitType nvicConfig;
    nvicConfig.NVIC_IRQChannel = UART_IRQn;
    nvicConfig.NVIC_IRQChannelPreemptionPriority = LOW_PRIORITY;
    nvicConfig.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&nvicConfig);

    UART_DeInit();
    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_UART, DISABLE);
    return 0;
}

int uart_config(uart_baudrate_t baudrate, uart_databits_t databit,
                uart_parity_t parity, uart_stopbit_t stopbit)
{
    UART_InitType uartConf;
    uartConf.UART_BaudRate = (uint32_t)baudrate;

    switch(databit)
    {
        case UART_DATA_5BITS: uartConf.UART_WordLengthTransmit = UART_WordLength_5b; break;
        case UART_DATA_6BITS: uartConf.UART_WordLengthTransmit = UART_WordLength_6b; break;
        case UART_DATA_7BITS: uartConf.UART_WordLengthTransmit = UART_WordLength_7b; break;
        case UART_DATA_8BITS: uartConf.UART_WordLengthTransmit = UART_WordLength_8b; break;
        default: return -1;
    }
    uartConf.UART_WordLengthReceive = uartConf.UART_WordLengthTransmit;

    switch(parity)
    {
        case UART_PARITY_NO:   uartConf.UART_Parity = UART_Parity_No;    break;
        case UART_PARITY_ODD:  uartConf.UART_Parity = UART_Parity_Odd;   break;
        case UART_PARITY_EVEN: uartConf.UART_Parity = UART_Parity_Even;  break;
        default: return -1;
    }

    switch(stopbit)
    {
        case UART_STOPBIT_1: uartConf.UART_StopBits = UART_StopBits_1; break;
        case UART_STOPBIT_2: uartConf.UART_StopBits = UART_StopBits_2; break;
        default: return -1;
    }

#if defined(UART_RX_PIN) && defined(UART_TX_PIN)
    uartConf.UART_Mode = UART_Mode_Rx | UART_Mode_Tx;
#elif defined(UART_RX_PIN)
    uartConf.UART_Mode = UART_Mode_Rx;
#elif defined(UART_TX_PIN)
    uartConf.UART_Mode = UART_Mode_Tx;
#else
    return -1;
#endif

    uartConf.UART_HardwareFlowControl = UART_HardwareFlowControl_None;
    uartConf.UART_FifoEnable = ENABLE;

    // Disable UART prior modifying configuration registers
    UART_Cmd(DISABLE);

    // Set the uart configuration
    UART_Init(&uartConf);

    // Enable RX FIFO Full Interrupt
    UART_RxFifoIrqLevelConfig(FIFO_LEV_1_64);
    UART_ITConfig(UART_IT_RX, ENABLE);

    // Enable UART
    UART_Cmd(ENABLE);

    return 0;
}

int uart_write(void const * buf, unsigned int nbyte)
{
    int n = 0;

    // No byte to write ?
    if (nbyte == 0)
        return 0;

    UART_ITConfig(UART_IT_TXFE, DISABLE);
    UART_ClearITPendingBit(UART_IT_TXFE);

    // Transfer data of fifoTx to Uart Fifo
    while( (UART_GetFlagStatus(UART_FLAG_TXFF) == RESET) &&
           !fifo_isEmpty(&_uart_fifoTx) )
    {
        uint16_t byte = 0;
        FIFO_POP_BYTE((&_uart_fifoTx), (uint8_t*)&byte);
        UART_SendData(byte);
    }

    // Transfer data of buf to Uart Fifo
    while( (UART_GetFlagStatus(UART_FLAG_TXFF) == RESET) &&
           ((unsigned int)n < nbyte) )
    {
        uint16_t byte = *(uint8_t*)(buf+n);
        UART_SendData(byte);
        n++;
    }

    // Transfer remaining data of buf to fifoTx
    n += fifo_push(&_uart_fifoTx, buf+n, nbyte-n);

    // Don't enable UART Tx FiFo empty interrupt if no more byte to send.
    if (!fifo_isEmpty(&_uart_fifoTx))
        UART_ITConfig(UART_IT_TXFE, ENABLE);

    return n;
}

int uart_read(void * buf, unsigned int nbyte)
{
    size_t n = 0;
    
    // No byte to read ?
    if (nbyte == 0)
        return 0;

    UART_ITConfig(UART_IT_RX, DISABLE);

    // Transfer data of fifoRx to buf
    n += fifo_pop(&_uart_fifoRx, buf, nbyte);

    // Transfer data Uart Fifo to buf
    while( ((unsigned int)n < nbyte) &&
           (UART_GetFlagStatus(UART_FLAG_RXFE) == RESET) )
    {
        uint8_t byte = (uint8_t)UART_ReceiveData();
        *(uint8_t*)(buf+n) = byte;
        n++;
    }

    // Transfer data Uart Fifo to fifoRx
    while( (UART_GetFlagStatus(UART_FLAG_RXFE) == RESET)  &&
           !fifo_isFull(&_uart_fifoRx) )
    {
        uint16_t byte = UART_ReceiveData();
        FIFO_PUSH_BYTE((&_uart_fifoRx), (uint8_t)byte);
    }

    UART_ITConfig(UART_IT_RX, ENABLE);
    
    return n;
}

int uart_printf(char const * format, ...)
{
    va_list ap;
    char str[128];

    va_start(ap, format);
    int n = vsnprintf(str, sizeof(str), format, ap);
    va_end(ap);

    return uart_write(str, n);
}
