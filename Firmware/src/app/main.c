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

#include "FreeRTOS.h"
#include "task.h"

// Implemented functions -------------------------------------------------------

#define STACK_SIZE 200
StaticTask_t xTaskBuffer;
StackType_t xStack[STACK_SIZE];

/* Function that implements the task being created. */
void vTaskCode(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        boardLedOn();
        vTaskDelay( 500 / portTICK_PERIOD_MS );
        boardLedOff();
        vTaskDelay( 500 / portTICK_PERIOD_MS );
    }
}


int main()
{
    boardInit();

    /* Create the task without using any dynamic memory allocation. */
    xTaskCreateStatic(
        vTaskCode,        /* Function that implements the task. */
        "NAME",           /* Text name for the task. */
        STACK_SIZE,       /* Number of indexes in the xStack array. */
        NULL,             /* Parameter passed into the task. */
        tskIDLE_PRIORITY, /* Priority at which the task is created. */
        xStack,           /* Array to use as the task's stack. */
        &xTaskBuffer);     /* Variable to hold the task's data structure. */

    vTaskStartScheduler();
    for (;;)
        ;
}
