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
#define _TASK_APP_EVENT_QUEUE_LENGTH 8

// Enum ------------------------------------------------------------------------
typedef enum
{
    APP_STATUS_BLE_ERROR,    //!< Indicates a ble radio error.
    APP_STATUS_BONDING,      //!< Indicates that the device is bonding.
    APP_STATUS_DISCONNECTED, //!< Indicates that the device is disconnected.
    APP_STATUS_CONNECTED,    //!< Indicates that the device is connected.
    APP_STATUS_UNLOCKED      //!< Indicates that the device is unlocked.
} taskAppStatus_t;

typedef enum
{
    APP_EVENT_BLE_ERR,
    APP_EVENT_BUTTON_BOND
} taskAppEventType_t;

// Struct ----------------------------------------------------------------------
typedef struct
{
    taskAppEventType_t type;
} taskAppEvent_t;

typedef struct
{
    // Event queue
    QueueHandle_t eventQueue;
    StaticQueue_t eventStaticQueue;
    uint8_t eventQueueStorageArea[sizeof(taskAppEvent_t) * _TASK_APP_EVENT_QUEUE_LENGTH];

    // App status
    taskAppStatus_t status;
} taskApp_t;

// Global variables ------------------------------------------------------------
static taskApp_t _taskApp = {0};

// Private prototype functions -------------------------------------------------
void _taskAppEventBleErrHandle();

// Implemented functions -------------------------------------------------------
void taskAppCodeInit()
{
    _taskApp.eventQueue = xQueueCreateStatic(_TASK_APP_EVENT_QUEUE_LENGTH,
                                             sizeof(taskAppEvent_t),
                                             _taskApp.eventQueueStorageArea,
                                             &_taskApp.eventStaticQueue);

    _taskApp.status = APP_STATUS_DISCONNECTED;
}

void taskAppCode(__attribute__((unused)) void *parameters)
{
    taskAppEvent_t event;

    boardEnableIo(true);

    while (1)
    {
        xQueueReceive(_taskApp.eventQueue, &event, portMAX_DELAY);

        switch (event.type)
        {
        case APP_EVENT_BLE_ERR:
            _taskAppEventBleErrHandle();
            break;

        default:
            break;
        }
    }
}

// Handle event implemented fonction
void _taskAppEventBleErrHandle()
{
    _taskApp.status = APP_STATUS_BLE_ERROR;

    boardEnableIo(true);
    boardLedOn();
    taskLightSetColor(COLOR_RED, 0);
    
}

// Send event implemented fonction
void taskAppSendEventBondFromISR(BaseType_t *pxHigherPriorityTaskWoken)
{
    if (_taskApp.eventQueue != NULL)
    {
        taskAppEvent_t event = {
            .type = APP_EVENT_BUTTON_BOND};
        xQueueSendFromISR(_taskApp.eventQueue, &event, pxHigherPriorityTaskWoken);
    }
}

void taskAppBleErr()
{
    taskAppEvent_t event = {
        .type = APP_EVENT_BLE_ERR};
    xQueueSend(_taskApp.eventQueue, &event, portMAX_DELAY);
}
