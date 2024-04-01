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
 * @file adc.c
 * @author antoine163
 * @date 25-02-2024
 * @brief ADC driver
 */

// Include ---------------------------------------------------------------------
#include "adc.h"

#include "BlueNRG1_sysCtrl.h"
#include "BlueNRG1_adc.h"

// Implemented functions -------------------------------------------------------
ADC_InitType xADC_InitType;

int adc_init()
{

    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_ADC, ENABLE);

    /* Configure ADC */

    xADC_InitType.ADC_OSR = ADC_OSR_200;
    xADC_InitType.ADC_Input = ADC_Input_AdcPin1;
    xADC_InitType.ADC_ConversionMode = ADC_ConversionMode_Single;
    xADC_InitType.ADC_ReferenceVoltage = ADC_ReferenceVoltage_0V6;
    xADC_InitType.ADC_Attenuation = ADC_Attenuation_9dB54;

    ADC_Init(&xADC_InitType);

    /* Enable auto offset correction */
    ADC_AutoOffsetUpdate(ENABLE);
    ADC_Calibration(ENABLE);

    return 0;
}

int adc_deinit()
{
    return 0;
}

float adcGet()
{
    ADC_Cmd(ENABLE);

    while (ADC_GetFlagStatus(ADC_FLAG_EOC) == RESET)
    {
    }

    float adcValue = ADC_GetConvertedData(xADC_InitType.ADC_Input, xADC_InitType.ADC_ReferenceVoltage);
    adcValue = ADC_CompensateOutputValue(adcValue) * 1000.0;
    return adcValue;
}