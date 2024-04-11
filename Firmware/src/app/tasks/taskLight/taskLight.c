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
#include <queue.h>

// Define ----------------------------------------------------------------------
#define _MSG_QUEUE_LENGTH 8

// Struct ----------------------------------------------------------------------
typedef struct
{
    color_t color;
    unsigned int transTime;
} taskLightMsg_t;

typedef struct
{
    QueueHandle_t msgQueue;
    StaticQueue_t msgStaticQueue;
    uint8_t msgQueueStorageArea[sizeof(taskLightMsg_t) * _MSG_QUEUE_LENGTH];
} taskLight_t;

// Global variables ------------------------------------------------------------
static taskLight_t _taskLight;

// Implemented functions -------------------------------------------------------
void taskLightCode(__attribute__((unused)) void *parameters)
{
    taskLightMsg_t msg;
    TickType_t tickToWait = portMAX_DELAY;

    _taskLight.msgQueue = xQueueCreateStatic(_MSG_QUEUE_LENGTH,
                                             sizeof(taskLightMsg_t),
                                             _taskLight.msgQueueStorageArea,
                                             &_taskLight.msgStaticQueue);

    while (xQueueReceive(_taskLight.msgQueue, &msg, portMAX_DELAY))
    {
        boardSetLightColor(msg.color);
        boardSetLightDc(100);
    }
}

void taskLightSetColor(color_t color, unsigned int transTime)
{
    taskLightMsg_t msg = {
        color = color,
        transTime = transTime};
    xQueueSend(_taskLight.msgQueue, &msg, 0);
}