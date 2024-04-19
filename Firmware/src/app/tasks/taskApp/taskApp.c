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
 * @file taskApp.h
 * @author antoine163
 * @date 03-04-2024
 * @brief Task of the application
 */

// Include ---------------------------------------------------------------------
#include "taskApp.h"
#include "board.h"
#include "tasks/taskLight/taskLight.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

// Define ----------------------------------------------------------------------
#define _MSG_QUEUE_LENGTH 8

// Enum ------------------------------------------------------------------------
typedef enum
{
    TASK_APP_EVENT_BUTTON_BOND
} taskAppEventType_t;

// Struct ----------------------------------------------------------------------
typedef struct
{
    taskAppEventType_t event;
} taskAppMsg_t;

typedef struct
{
    QueueHandle_t msgQueue;
    StaticQueue_t msgStaticQueue;
    uint8_t msgQueueStorageArea[sizeof(taskAppMsg_t) * _MSG_QUEUE_LENGTH];
} taskApp_t;

// Global variables ------------------------------------------------------------
static taskApp_t _taskApp = {
    .msgQueue = NULL
};

// Implemented functions -------------------------------------------------------
void taskAppCode(__attribute__((unused)) void *parameters)
{
    taskAppMsg_t msg;

    _taskApp.msgQueue = xQueueCreateStatic(_MSG_QUEUE_LENGTH,
                                           sizeof(taskAppMsg_t),
                                           _taskApp.msgQueueStorageArea,
                                           &_taskApp.msgStaticQueue);

    boardEnableIo(true);

    while (1)
    {
        if (xQueueReceive(_taskApp.msgQueue, &msg, 400 / portTICK_PERIOD_MS) == pdTRUE)
        {

            boardOpen();
            static int leddc = 0;
            leddc += 10;

            if (leddc > 100)
            {
                leddc = 0;
                taskLightSetColor(COLOR_OFF, 0);
            }
            else
            {
                boardSetLightDc( leddc );
                taskLightSetColor(COLOR_BLUE, 0);
            }
        }
        else
        {
            boardLedToggel();
            // boardSetLightDc(boardGetBrightness());
        }
    }

    // while(1)
    // {
    boardLedOn();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    boardLedOff();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    // }

    // taskLightSetColor(COLOR_GREEN, 0);

    boardUnlock();

    // boardOpen();

    // boardLock();

    // taskLightSetColor(COLOR_WHITE, 0);

    bool ledon = false;
    while (1)
    {
        // taskLightSetColor(COLOR_RED, 0);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
        // taskLightSetColor(COLOR_GREEN, 0);
        // vTaskDelay(1000  / portTICK_PERIOD_MS);
        // taskLightSetColor(COLOR_BLUE, 0);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
        // taskLightSetColor(COLOR_WHITE, 0);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);

        // taskLightSetColor(COLOR_OFF, 0 );
        // vTaskDelay(1000 / portTICK_PERIOD_MS);

        boardLedToggel();

        int val = boardGetBrightness();
        boardPrintf("Brightness:%i\r\n", val);
        boardPrintf("Is open:%s\r\n", boardIsOpen() ? "yes" : "no");

        vTaskDelay(500 / portTICK_PERIOD_MS);

        if (GPIO_ReadBit(BOND_PIN) == Bit_SET)
        {
            vTaskDelay(500 / portTICK_PERIOD_MS);

            if (ledon == true)
            {
                ledon = false;
                taskLightSetColor(COLOR_WHITE, 0);
            }
            else
            {
                ledon = true;
                taskLightSetColor(COLOR_OFF, 0);
            }

            // boardOpen();
        }
    }
}

void taskAppSendMsgBond(BaseType_t* pxHigherPriorityTaskWoken)
{
    if (_taskApp.msgQueue != NULL)
    {
        taskAppMsg_t msg = {
            .event = TASK_APP_EVENT_BUTTON_BOND};
        xQueueSendFromISR(_taskApp.msgQueue, &msg, pxHigherPriorityTaskWoken);
    }
}