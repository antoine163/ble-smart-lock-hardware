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
 * @file taskLight.h
 * @author antoine163
 * @date 07-04-2024
 * @brief Task to manage ambient light and lighting
 */

// Include ---------------------------------------------------------------------
#include "taskLight.h"
#include "board.h"

#include <FreeRTOS.h>
#include <task.h>

// Global variables ------------------------------------------------------------

// Implemented functions -------------------------------------------------------


// Implemented functions -------------------------------------------------------
void taskLightCode(__attribute__((unused)) void *parameters)
{
    boardEnableIo(true);
    // boardSetLightColor(COLOR_RED);
    boardSetLightDc(1);

    // vTaskDelay(200 / portTICK_PERIOD_MS);
    boardSetLightColor(COLOR_WHITE);
    
    
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        boardSetLightColor(COLOR_RED);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        boardSetLightColor(COLOR_GREEN);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        boardSetLightColor(COLOR_BLUE);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        boardSetLightColor(COLOR_WHITE);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        boardSetLightColor(COLOR_OFF);
    }
}

void taskLightSetColor(color_t color, unsigned int transTime)
{

}