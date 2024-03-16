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
 * @file uart.h
 * @author antoine163
 * @date 15.07.2023
 * @brief This is a uart driver
 */

#ifndef UART_H
#define UART_H

// Include ---------------------------------------------------------------------
#include "map_hard.h"
#include "fifo.h"

#include "BlueNRG1_uart.h"

// Enum ------------------------------------------------------------------------
typedef enum
{
   UART_BAUDRATE_300     = 600,     //!< 600 bps.
   UART_BAUDRATE_1200    = 1200,    //!< 1200 bps.
   UART_BAUDRATE_2400    = 2400,    //!< 2400 bps.
   UART_BAUDRATE_4800    = 4800,    //!< 4800 bps.
   UART_BAUDRATE_9600    = 9600,    //!< 9600 bps.
   UART_BAUDRATE_19200   = 19200,   //!< 19200 bps.
   UART_BAUDRATE_38400   = 38400,   //!< 38400 bps.
   UART_BAUDRATE_57600   = 57600,   //!< 57600 bps.
   UART_BAUDRATE_115200  = 115200,  //!< 115200 bps.
   UART_BAUDRATE_230400  = 230400,  //!< 230400 bps.
   UART_BAUDRATE_460800  = 460800,  //!< 460800 bps.
   UART_BAUDRATE_921600  = 921600,  //!< 921600 bps.
   UART_BAUDRATE_1843200 = 1843200, //!< 1843200 bps.
   UART_BAUDRATE_3686400 = 3686400  //!< 3686400 bps.
}uart_baudrate_t;

typedef enum
{
    UART_DATA_5BITS = 5, //!< data of 5 bits.
    UART_DATA_6BITS = 6, //!< data of 6 bits.
    UART_DATA_7BITS = 7, //!< data of 7 bits.
    UART_DATA_8BITS = 8, //!< data of 8 bits.
}uart_databits_t;

typedef enum
{
    UART_PARITY_NO,      //!< No parity.
    UART_PARITY_ODD,     //!< ODD parity.
    UART_PARITY_EVEN     //!< Even parity.
}uart_parity_t;

typedef enum
{
    UART_STOPBIT_1,      //!< 1 stop bit.
    UART_STOPBIT_2       //!< 2 stop bit.
}uart_stopbit_t;

// Prototype functions ---------------------------------------------------------
int uart_init();
int uart_deinit();
int uart_config(uart_baudrate_t baudrate, uart_databits_t databit,
                uart_parity_t parity, uart_stopbit_t stopbit);

int uart_write(void const * buf, unsigned int nbyte);
int uart_read(void * buf, unsigned int nbyte);
int uart_printf(char const * format, ...);

// Exported global variables ---------------------------------------------------
#if defined(UART_TX_PIN)
extern fifo_t _uart_fifoTx;
#endif

#if defined(UART_RX_PIN)
extern fifo_t _uart_fifoRx;
#endif

// ISR handler -----------------------------------------------------------------
static inline void _uart_isr_handler()
{
    // -- Manage transmit data --
    // The UART Tx FiFo is empty and the interrupt is enable ?
    if ( (UART_GetITStatus(UART_IT_TXFE) == SET) &&
         (READ_BIT(UART->IMSC, UART_IT_TXFE) != 0) )
    {
        UART_ClearITPendingBit(UART_IT_TXFE);

        // Transfer data Fifo to Uart Fifo
        while( (UART_GetFlagStatus(UART_FLAG_TXFF) == RESET) &&
               !fifo_isEmpty(&_uart_fifoTx))
        {
            uint16_t byte = 0;
            FIFO_POP_BYTE((&_uart_fifoTx), (uint8_t*)&byte);
            UART_SendData(byte);
        }

        // Disable UART Tx FiFo empty interrupt if the data Tx FiFo is empty.
        if (fifo_isEmpty(&_uart_fifoTx))
            UART_ITConfig(UART_IT_TXFE, DISABLE);
    }

    // -- Manage receive data --
    // The UART Rx FiFo is full and the interrupt is enable ?
    if ( (UART_GetITStatus(UART_IT_RX) == SET) &&
         (READ_BIT(UART->IMSC, UART_IT_RX) != 0) )
    {
        UART_ClearITPendingBit(UART_IT_RX);

        // Transfer Uart Fifo to data Fifo
        while( (UART_GetFlagStatus(UART_FLAG_RXFE) == RESET) )
        {
            uint16_t byte = UART_ReceiveData();
            FIFO_PUSH_BYTE((&_uart_fifoRx), (uint8_t)byte);
        }
    }
}

#endif // UART_H
