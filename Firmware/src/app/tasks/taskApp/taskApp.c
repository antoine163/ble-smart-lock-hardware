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
#include "tasks/taskBle/taskBle.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

// Define ----------------------------------------------------------------------
#define _TASK_APP_EVENT_QUEUE_LENGTH 8
#define _TASK_APP_DEFAULT_BRIGHTNESS_THRESHOLD 50.f           //!< Default threshold: 50%
#define RESTART_DELAY_TICK (1 * 60 * 1000 / portTICK_PERIOD_MS)   //!< Tick to wait before restarting following error detection: 1min
#define OFF_LIGHT_DELAY_TICK (15 * 60 * 1000 / portTICK_PERIOD_MS) //!< Tick to wait before torn off light following disconnection: 15min

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
    APP_EVENT_BLE_CONNECTED,
    APP_EVENT_BLE_DISCONNECTED,
    APP_EVENT_BLE_UNLOCK,
    APP_EVENT_BLE_LOCK,
    APP_EVENT_BLE_OPEN,
    APP_EVENT_BUTTON_BOND,
    APP_EVENT_DOOR_STATE
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

    // Timeout to manage reset follows an error
    TickType_t ticksToRestart;
    TimeOut_t timeOutRestart;

    // Timeout to manage light off follows an disconnection.
    TickType_t ticksToOffLight;
    TimeOut_t timeOutOffLight;

    // App conf
    float brightnessTh;
} taskApp_t;

// Global variables ------------------------------------------------------------
static taskApp_t _taskApp = {0};

// Private prototype functions -------------------------------------------------
void _taskAppManageLightColor(taskAppStatus_t lastStatus);
void _taskAppSetStatus(taskAppStatus_t status);
void _taskAppSetLightOn();
void _taskAppEventBleErrHandle();
void _taskAppEventBleDisconnectedHandle();
void _taskAppEventBleConnectedHandle();
void _taskAppEventBleUnlockHandle();
void _taskAppEventBleLockHandle();
void _taskAppEventBleOpenHandle();
void _taskAppEventDoorStateHandle();

// Implemented functions -------------------------------------------------------
void taskAppCodeInit()
{
    _taskApp.eventQueue = xQueueCreateStatic(_TASK_APP_EVENT_QUEUE_LENGTH,
                                             sizeof(taskAppEvent_t),
                                             _taskApp.eventQueueStorageArea,
                                             &_taskApp.eventStaticQueue);

    _taskApp.status = APP_STATUS_DISCONNECTED;
    _taskApp.ticksToRestart = portMAX_DELAY;
    _taskApp.ticksToOffLight = portMAX_DELAY;

    // Todo: Lir la valeur à partie de la flash
    _taskApp.brightnessTh = _TASK_APP_DEFAULT_BRIGHTNESS_THRESHOLD;
}

void taskAppCode(__attribute__((unused)) void *parameters)
{
    taskAppEvent_t event;

    // Make discoverable Ble if door is closed
    if (boardIsOpen() == false)
        taskBleEventDiscoverable();
    else // Or the door is opened
    {
        _taskAppSetLightOn();

        // Turn off the red light in 15 minutes if there are no new.
        // events.
        _taskApp.ticksToOffLight = OFF_LIGHT_DELAY_TICK;
        vTaskSetTimeOutState(&_taskApp.timeOutOffLight);
    }

    while (1)
    {
        TickType_t ticksToWait = _taskApp.ticksToRestart;
        if (_taskApp.ticksToOffLight < ticksToWait)
            ticksToWait = _taskApp.ticksToOffLight;

        if (xQueueReceive(_taskApp.eventQueue, &event, ticksToWait) == pdPASS)
        {
            switch (event.type)
            {
            case APP_EVENT_BLE_ERR:
                _taskAppEventBleErrHandle();
                break;

            case APP_EVENT_BLE_CONNECTED:
                _taskAppEventBleConnectedHandle();
                break;

            case APP_EVENT_BLE_DISCONNECTED:
                _taskAppEventBleDisconnectedHandle();
                break;

            case APP_EVENT_BLE_UNLOCK:
                _taskAppEventBleUnlockHandle();
                break;

            case APP_EVENT_BLE_LOCK:
                _taskAppEventBleLockHandle();
                break;

            case APP_EVENT_BLE_OPEN:
                _taskAppEventBleOpenHandle();
                break;

            case APP_EVENT_DOOR_STATE:
                _taskAppEventDoorStateHandle();
                break;

            default:
                break;
            }
        }

        if (xTaskCheckForTimeOut(&_taskApp.timeOutRestart, &_taskApp.ticksToRestart) != pdFALSE)
        {
            NVIC_SystemReset();
        }

        if (xTaskCheckForTimeOut(&_taskApp.timeOutOffLight, &_taskApp.ticksToOffLight) != pdFALSE)
        {
            _taskApp.ticksToOffLight = portMAX_DELAY;
            taskLightAnimTrans(0, COLOR_OFF, 0);
        }
    }
}

float taskAppGetBrightnessTh()
{
    return _taskApp.brightnessTh;
}

void taskAppSetBrightnessTh(float th)
{
    _taskApp.brightnessTh = th;
    // Todo: notifier tashApp pour sauvegarder la nouvelle valeur dans la flash.
}

void _taskAppManageLightColor(taskAppStatus_t lastStatus)
{
    switch (_taskApp.status)
    {
    case APP_STATUS_BLE_ERROR:
        taskLightAnimTrans(0, COLOR_RED, 0);
        // Note: the device will be reset in 1min
        break;

    case APP_STATUS_BONDING:
    {
        taskLightAnimTrans(200, COLOR_YELLOW, 1000);
        // Todo: set timeout to stop bond
        break;
    }
    case APP_STATUS_DISCONNECTED:
    {
        if (boardIsOpen() == true)
        {
            if ((APP_STATUS_DISCONNECTED == lastStatus))
            {
                // Here, the door was opened with the key.
                _taskAppSetLightOn();

                // Turn off the red light in 15 minutes if there are no new.
                // events.
                _taskApp.ticksToOffLight = OFF_LIGHT_DELAY_TICK;
                vTaskSetTimeOutState(&_taskApp.timeOutOffLight);
            }
            else
            {
                // Ble device is disconnected but the door is open.
                // Turns on the red light to try to warn the user.
                taskLightAnimBlink(0, COLOR_RED, 100, 500);

                // Turn off the red light in 15 minutes if there are no new.
                // events.
                _taskApp.ticksToOffLight = OFF_LIGHT_DELAY_TICK;
                vTaskSetTimeOutState(&_taskApp.timeOutOffLight);
            }
        }
        else if ((APP_STATUS_CONNECTED == lastStatus) ||
                 (APP_STATUS_UNLOCKED == lastStatus))
            taskLightAnimTrans(4000, COLOR_OFF, 0);
        else
            taskLightAnimTrans(0, COLOR_OFF, 0);

        break;
    }
    case APP_STATUS_CONNECTED:
    {
        if (boardIsOpen() == true)
            _taskAppSetLightOn();
        else
            taskLightAnimSin(200, COLOR_BLUE, 0.2f);

        break;
    }
    case APP_STATUS_UNLOCKED:
    {
        if (boardIsOpen() == true)
            _taskAppSetLightOn();
        else
            taskLightAnimTrans(200, COLOR_BLUE, 500);

        break;
    }

    default:
        break;
    }
}

void _taskAppSetStatus(taskAppStatus_t status)
{
    taskAppStatus_t lastStatus = _taskApp.status;
    _taskApp.status = status;
    _taskAppManageLightColor(lastStatus);
}

void _taskAppSetLightOn()
{
    if (boardGetBrightness() <= _taskApp.brightnessTh)
        taskLightAnimTrans(200, COLOR_WHITE_LIGHT, 200);
    else
        taskLightAnimTrans(200, COLOR_YELLOW, 200);
}

// Handle event implemented fonction
void _taskAppEventBleErrHandle()
{
    boardPrintf("App: ble radio error !\r\n");
    boardLedOn();

    // Init time to restart into 1min
    _taskApp.ticksToRestart = RESTART_DELAY_TICK;
    vTaskSetTimeOutState(&_taskApp.timeOutRestart);

    _taskAppSetStatus(APP_STATUS_BLE_ERROR);
}

void _taskAppEventBleDisconnectedHandle()
{
    boardPrintf("App: device disconnected.\r\n");
    boardLock();

    if (boardIsOpen() == false)
        taskBleEventDiscoverable();

    _taskAppSetStatus(APP_STATUS_DISCONNECTED);
}

void _taskAppEventBleConnectedHandle()
{
    boardPrintf("App: device connected.\r\n");

    _taskAppSetStatus(APP_STATUS_CONNECTED);
}

void _taskAppEventBleUnlockHandle()
{
    boardPrintf("App: unlock the lock.\r\n");
    boardUnlock();

    _taskAppSetStatus(APP_STATUS_UNLOCKED);
}

void _taskAppEventBleLockHandle()
{
    boardPrintf("App: Lock the lock.\r\n");
    boardLock();

    _taskAppSetStatus(APP_STATUS_CONNECTED);
}

void _taskAppEventBleOpenHandle()
{
    if (boardIsLocked() == true)
    {
        boardPrintf("App: the lock is loked, can't open.\r\n");
        return;
    }

    boardPrintf("App: open the lock.\r\n");
    boardOpen();
}

void _taskAppEventDoorStateHandle()
{
    // Update BLE characteristic
    taskBleEventDoorState();

    if (boardIsOpen() == true)
    {
        boardPrintf("App: door is open.\r\n");
        taskBleEventUndiscoverable();
    }
    else
    {
        boardPrintf("App: door is close.\r\n");

        if (_taskApp.status == APP_STATUS_DISCONNECTED)
            taskBleEventDiscoverable();
    }

    _taskAppManageLightColor(_taskApp.status);
}

// Send event implemented fonction
void taskAppEventBondFromISR(BaseType_t *pxHigherPriorityTaskWoken)
{
    if (_taskApp.eventQueue != NULL)
    {
        taskAppEvent_t event = {
            .type = APP_EVENT_BUTTON_BOND};
        xQueueSendFromISR(_taskApp.eventQueue, &event, pxHigherPriorityTaskWoken);
    }
}

void taskAppEventDoorStateFromISR(BaseType_t *pxHigherPriorityTaskWoken)
{
    if (_taskApp.eventQueue != NULL)
    {
        taskAppEvent_t event = {
            .type = APP_EVENT_DOOR_STATE};
        xQueueSendFromISR(_taskApp.eventQueue, &event, pxHigherPriorityTaskWoken);
    }
}

void taskAppEventBleErr()
{
    taskAppEvent_t event = {
        .type = APP_EVENT_BLE_ERR};
    xQueueSend(_taskApp.eventQueue, &event, portMAX_DELAY);
}

void taskAppEventBleConnected()
{
    taskAppEvent_t event = {
        .type = APP_EVENT_BLE_CONNECTED};
    xQueueSend(_taskApp.eventQueue, &event, portMAX_DELAY);
}

void taskAppEventBleDisconnected()
{
    taskAppEvent_t event = {
        .type = APP_EVENT_BLE_DISCONNECTED};
    xQueueSend(_taskApp.eventQueue, &event, portMAX_DELAY);
}

void taskAppEventBleUnlock()
{
    taskAppEvent_t event = {
        .type = APP_EVENT_BLE_UNLOCK};
    xQueueSend(_taskApp.eventQueue, &event, portMAX_DELAY);
}

void taskAppEventBleOpen()
{
    taskAppEvent_t event = {
        .type = APP_EVENT_BLE_OPEN};
    xQueueSend(_taskApp.eventQueue, &event, portMAX_DELAY);
}
