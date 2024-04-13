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
#include "drivers/pwm.h"
#include "drivers/adc.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <FreeRTOS.h>
#include <semphr.h>

// Struct ------------------------------------------------------------------------
typedef struct
{
    // Serial section
    uart_t serial;
    uint8_t serialBufTx[UART_TX_FIFO_SIZE];
    uint8_t serialBufRx[UART_RX_FIFO_SIZE];
    SemaphoreHandle_t serialSemaphore;
    StaticSemaphore_t serialMutexBuffer;

    // Light section
    pwm_t lightPwm;
    color_t lightColor;
    float lightDc;

    // Sensor section
    adc_t sensorAdc;
} board_t;

// Prototype static functions --------------------------------------------------
static void _boardInitGpio();
static void _boardInitUart();
static void _boardInitPwm();
static void _boardInitAdc();

// Global variables ------------------------------------------------------------
board_t _board = {
    .serialSemaphore = NULL,
    .lightColor = COLOR_OFF,
    .lightDc = 0.};

// Implemented functions -------------------------------------------------------

/**
 * @brief Init clock, power, gpio, uart, pwm, adc, of the board.
 */
void boardInit()
{
    // Init system
    SystemInit();

    // Make semaphores
    _board.serialSemaphore = xSemaphoreCreateRecursiveMutexStatic(&_board.serialMutexBuffer);

    // Init peripherals
    _boardInitGpio();
    _boardInitUart();
    _boardInitPwm();
    _boardInitAdc();
}

int boardPrintf(char const *format, ...)
{
    va_list ap;
    char str[128];

    va_start(ap, format);
    int n = vsnprintf(str, sizeof(str), format, ap);
    va_end(ap);

    xSemaphoreTakeRecursive(_board.serialSemaphore, portMAX_DELAY);
    n = uart_write(&_board.serial, str, n);
    xSemaphoreGiveRecursive(_board.serialSemaphore);

    return n;
}

void boardEnableIo(bool enable)
{
    GPIO_WriteBit(EN_IO_PIN, enable);
}

void boardSetLightColor(color_t color)
{
    color_t lastColor = _board.lightColor;
    _board.lightColor = color;

    // Off all color
    GPIO_ResetBits(LIGHT_RED_PIN |
                   LIGHT_BLUE_PIN |
                   LIGHT_GREEN_PIN |
                   LIGHT_WHITE_PIN);

    // Set PWM duty cycle
    boardSetLightDc(_board.lightDc);

    // On color
    switch (color)
    {
    case COLOR_OFF:
    {
        // Disable PWM Pin
        GPIO_InitType GPIO_InitStruct;
        GPIO_StructInit(&GPIO_InitStruct);
        GPIO_InitStruct.GPIO_Pin = LIGHT_PWM_PIN;
        GPIO_InitStruct.GPIO_Mode = LIGHT_PWM_MODE_OUT;
        GPIO_Init(&GPIO_InitStruct);
        break;
    }
    case COLOR_RED:
    {
        GPIO_SetBits(LIGHT_RED_PIN);
        break;
    }
    case COLOR_GREEN:
    {
        GPIO_SetBits(LIGHT_GREEN_PIN);
        break;
    }
    case COLOR_BLUE:
    {
        GPIO_SetBits(LIGHT_BLUE_PIN);
        break;
    }
    case COLOR_WHITE:
    {
        GPIO_SetBits(LIGHT_WHITE_PIN);
        break;
    }
    }

    if ((lastColor == COLOR_OFF) &&
        (color != COLOR_OFF))
    {
        // Enable PWM Pin
        GPIO_InitType GPIO_InitStruct;
        GPIO_StructInit(&GPIO_InitStruct);
        GPIO_InitStruct.GPIO_Pin = LIGHT_PWM_PIN;
        GPIO_InitStruct.GPIO_Mode = LIGHT_PWM_MODE_PWM;
        GPIO_Init(&GPIO_InitStruct);
    }
}

void boardSetLightDc(float dc)
{
    // Set duty cycle
    if (_board.lightColor == COLOR_WHITE)
        pwm_setDc(&_board.lightPwm, dc);
    else
        pwm_setDc(&_board.lightPwm, 100. - dc);

    _board.lightDc = dc;
}

float boardGetBrightness()
{
    adc_config(&_board.sensorAdc, ADC_CH_PIN1);
    float val = adc_convert_voltage(&_board.sensorAdc);
    return 100.0f - val * 100.0f / 3.3f;
}

// Implemented static functions ------------------------------------------------

void _boardInitGpio()
{
    // Init Gpio
    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_GPIO, ENABLE);

    GPIO_InitType GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);

    // ---- Pins section ----

    // Init bond button pin
    GPIO_InitStruct.GPIO_Pin = BOND_PIN;
    GPIO_InitStruct.GPIO_Mode = BOND_MODE;
    GPIO_Init(&GPIO_InitStruct);
    // Init opened feedback pin
    GPIO_InitStruct.GPIO_Pin = OPENED_PIN;
    GPIO_InitStruct.GPIO_Mode = OPENED_MODE;
    GPIO_Init(&GPIO_InitStruct);
    // Init lock pin
    GPIO_InitStruct.GPIO_Pin = LOCK_PIN;
    GPIO_InitStruct.GPIO_Mode = LOCK_MODE_OUT;
    GPIO_SetBits(GPIO_InitStruct.GPIO_Pin);
    GPIO_Init(&GPIO_InitStruct);
    // Init Led pin
    GPIO_InitStruct.GPIO_Pin = LED_PIN;
    GPIO_InitStruct.GPIO_Mode = LED_MODE;
    GPIO_ResetBits(GPIO_InitStruct.GPIO_Pin);
    GPIO_Init(&GPIO_InitStruct);
    // Init enable IO pin
    GPIO_InitStruct.GPIO_Pin = EN_IO_PIN;
    GPIO_InitStruct.GPIO_Mode = EN_IO_MODE;
    GPIO_ResetBits(GPIO_InitStruct.GPIO_Pin);
    GPIO_Init(&GPIO_InitStruct);

    // ---- Light section ----

    // Init light pins
    GPIO_InitStruct.GPIO_Pin = LIGHT_RED_PIN |
                               LIGHT_GREEN_PIN |
                               LIGHT_BLUE_PIN |
                               LIGHT_WHITE_PIN;
    GPIO_InitStruct.GPIO_Mode = LIGHT_MODE;
    GPIO_ResetBits(GPIO_InitStruct.GPIO_Pin);
    GPIO_Init(&GPIO_InitStruct);
    // Init light pwm pin
    GPIO_InitStruct.GPIO_Pin = LIGHT_PWM_PIN;
    GPIO_InitStruct.GPIO_Mode = LIGHT_PWM_MODE_OUT;
    GPIO_ResetBits(GPIO_InitStruct.GPIO_Pin);
    GPIO_Init(&GPIO_InitStruct);

    // ---- Uart section ----

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
    uart_init(&_board.serial, UART,
              _board.serialBufTx, sizeof(_board.serialBufTx),
              _board.serialBufRx, sizeof(_board.serialBufRx));

    uart_config(&_board.serial,
                UART_BAUDRATE_115200,
                UART_DATA_8BITS,
                UART_PARITY_NO,
                UART_STOPBIT_1);
}

void _boardInitPwm()
{
    // Init pwm
    pwm_init(&_board.lightPwm, MFT1);
    pwm_config(&_board.lightPwm, 60);
}

void _boardInitAdc()
{
    // Init adc
    adc_init(&_board.sensorAdc, ADC);
    adc_config(&_board.sensorAdc, ADC_CH_PIN1);
}