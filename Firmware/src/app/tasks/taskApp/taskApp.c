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
#include "tasks/taskLight/taskLight.h"
#include "tasks/taskBle/taskBle.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

// Define ----------------------------------------------------------------------
#define _TASK_APP_DEFAULT_BRIGHTNESS_THRESHOLD 50.f                          //!< Default threshold: 50%
#define _TASK_APP_RESTART_DELAY_TICK (1 * 60 * 1000 / portTICK_PERIOD_MS)    //!< Tick to wait before restarting following error detection: 1min
#define _TASK_APP_OFF_LIGHT_DELAY_TICK (15 * 60 * 1000 / portTICK_PERIOD_MS) //!< Tick to wait before torn off light following disconnection: 15min
#define _TASK_APP_CLEAR_BONDED_DELAY_TICK (3 * 1000 / portTICK_PERIOD_MS)    //!< Tick to wait before clear all bonded devices: 3s
#define _TASK_APP_EVENT_QUEUE_LENGTH 8

// Enum ------------------------------------------------------------------------
typedef enum
{
    _TASK_APP_STATUS_BLE_ERROR,    //!< Indicates a ble radio error.
    _TASK_APP_STATUS_BONDING,      //!< Indicates that the device is bonding.
    _TASK_APP_STATUS_DISCONNECTED, //!< Indicates that the device is disconnected.
    _TASK_APP_STATUS_CONNECTED,    //!< Indicates that the device is connected.
    _TASK_APP_STATUS_UNLOCKED      //!< Indicates that the device is unlocked.
} taskAppStatus_t;

// Struct ----------------------------------------------------------------------
typedef struct
{
    boardEvent_t boardEvent;
} taskAppEventItem_t;

typedef struct
{
    // Event queue
    QueueHandle_t eventQueue;
    StaticQueue_t eventQueueBuffer;
    uint8_t eventQueueStorageArea[sizeof(taskAppEventItem_t) * _TASK_APP_EVENT_QUEUE_LENGTH];

    // App status
    taskAppStatus_t status;

    // Timeout to manage reset follows an error
    TickType_t ticksToRestart;
    TimeOut_t timeOutRestart;

    // Timeout to manage light off follows an disconnection.
    TickType_t ticksToOffLight;
    TimeOut_t timeOutOffLight;

    // Timeout to manage clean all bonded device
    TickType_t ticksToClearBonded;
    TimeOut_t timeOutClearBonded;

    // App conf
    float brightnessTh;
} taskApp_t;

// Global variables ------------------------------------------------------------
static taskApp_t _taskApp = {0};

// Private prototype functions -------------------------------------------------
void _taskAppManageLightColor(taskAppStatus_t lastStatus);

void _taskAppSetStatus(taskAppStatus_t status);
void _taskAppSetLightOn();

void _taskAppBleEventErrHandle();
void _taskAppBleEventDisconnectedHandle();
void _taskAppBleEventConnectedHandle();

void _taskAppBleEventLockHandle();

void _taskAppBoardEventDoorStateHandle();
void _taskAppBoardEventButtonBondStateHandle();

// Implemented functions -------------------------------------------------------
void taskAppCodeInit()
{
    _taskApp.eventQueue = xQueueCreateStatic(_TASK_APP_EVENT_QUEUE_LENGTH,
                                             sizeof(taskAppEventItem_t),
                                             _taskApp.eventQueueStorageArea,
                                             &_taskApp.eventQueueBuffer);

    _taskApp.status = _TASK_APP_STATUS_DISCONNECTED;
    _taskApp.ticksToRestart = portMAX_DELAY;
    _taskApp.ticksToOffLight = portMAX_DELAY;
    _taskApp.ticksToClearBonded = portMAX_DELAY;

    // Todo: Lir la valeur à partie de la flash
    _taskApp.brightnessTh = _TASK_APP_DEFAULT_BRIGHTNESS_THRESHOLD;
}

void taskAppCode(__attribute__((unused)) void *parameters)
{
    taskAppEventItem_t eventItem;

    // Set brightens threshold BLE attribut
    taskBleUpdateAtt(BLE_ATT_BRIGHTNESS_TH,
                     &_taskApp.brightnessTh,
                     sizeof(_taskApp.brightnessTh));

    // /Door is open ?
    if (boardIsOpen() == true)
    {
        _taskAppSetLightOn();

        // Turn off the red light in 15 minutes if there are no new.
        // events.
        _taskApp.ticksToOffLight = _TASK_APP_OFF_LIGHT_DELAY_TICK;
        vTaskSetTimeOutState(&_taskApp.timeOutOffLight);
    }

    while (1)
    {
        TickType_t ticksToWait = _taskApp.ticksToRestart;
        if (_taskApp.ticksToOffLight < ticksToWait)
            ticksToWait = _taskApp.ticksToOffLight;

        if (xQueueReceive(_taskApp.eventQueue, &eventItem, ticksToWait) == pdPASS)
        {
            switch (eventItem.boardEvent)
            {
            case BOARD_EVENT_DOOR_STATE:
                _taskAppBoardEventDoorStateHandle();
                break;

            case BOARD_EVENT_BUTTON_BOND_STATE:
                _taskAppBoardEventButtonBondStateHandle();
                break;

            default:
                break;
            }
        }

        if ((_taskApp.ticksToRestart != portMAX_DELAY) &&
            (xTaskCheckForTimeOut(&_taskApp.timeOutRestart, &_taskApp.ticksToRestart) != pdFALSE))
        {
            NVIC_SystemReset();
        }

        if ((_taskApp.ticksToOffLight != portMAX_DELAY) &&
            (xTaskCheckForTimeOut(&_taskApp.timeOutOffLight, &_taskApp.ticksToOffLight) != pdFALSE))
        {
            _taskApp.ticksToOffLight = portMAX_DELAY;
            taskLightAnimTrans(0, COLOR_OFF, 0);
        }

        // Todo ...
        // if ((_taskApp.ticksToClearBonded != portMAX_DELAY) &&
        //     (xTaskCheckForTimeOut(&_taskApp.timeOutClearBonded, &_taskApp.ticksToClearBonded) != pdFALSE))
        // {
        //     // _taskApp.ticksToClearBonded = portMAX_DELAY;
        //     // if (boardButtonBondState() == true)
        //     //     taskBleClearAllPairing();
        // }
    }
}

float taskAppGetBrightnessTh()
{
    return _taskApp.brightnessTh;
}

void taskAppSetBrightnessTh(float th)
{
    _taskApp.brightnessTh = th;
    // Todo: notifier _taskApp pour sauvegarder la nouvelle valeur dans la flash.
    // Ou directement sovgarder ici
}

void taskAppUnlock()
{
    if (_taskApp.status == _TASK_APP_STATUS_CONNECTED)
    {
        boardPrintf("App: unlock the lock.\r\n");
        boardUnlock();
        _taskAppSetStatus(_TASK_APP_STATUS_UNLOCKED);
    }
    else
        boardPrintf("App: Can't unlock if unconnected device.\r\n");
}

void taskAppOpenDoor()
{
    if (boardIsLocked() == true)
    {
        boardPrintf("App: the lock is loked, can't open.\r\n");
        return;
    }

    boardPrintf("App: open the foor.\r\n");
    boardOpen();
}

void _taskAppManageLightColor(taskAppStatus_t lastStatus)
{
    switch (_taskApp.status)
    {
    case _TASK_APP_STATUS_BLE_ERROR:
        taskLightAnimTrans(0, COLOR_RED, 0);
        // Note: the device will be reset in 1min
        break;

    case _TASK_APP_STATUS_BONDING:
    {
        taskLightAnimTrans(200, COLOR_YELLOW, 1000);
        // Todo: set timeout to stop bond
        break;
    }
    case _TASK_APP_STATUS_DISCONNECTED:
    {
        if (boardIsOpen() == true)
        {
            if ((_TASK_APP_STATUS_DISCONNECTED == lastStatus))
            {
                // Here, the door was opened with the key.
                _taskAppSetLightOn();

                // Turn off the red light in 15 minutes if there are no new.
                // events.
                _taskApp.ticksToOffLight = _TASK_APP_OFF_LIGHT_DELAY_TICK;
                vTaskSetTimeOutState(&_taskApp.timeOutOffLight);
            }
            else
            {
                // Ble device is disconnected but the door is open.
                // Turns on the red light to try to warn the user.
                taskLightAnimBlink(0, COLOR_RED, 100, 500);

                // Turn off the red light in 15 minutes if there are no new.
                // events.
                _taskApp.ticksToOffLight = _TASK_APP_OFF_LIGHT_DELAY_TICK;
                vTaskSetTimeOutState(&_taskApp.timeOutOffLight);
            }
        }
        else if ((_TASK_APP_STATUS_CONNECTED == lastStatus) ||
                 (_TASK_APP_STATUS_UNLOCKED == lastStatus))
            taskLightAnimTrans(4000, COLOR_OFF, 0);
        else
            taskLightAnimTrans(0, COLOR_OFF, 0);

        break;
    }
    case _TASK_APP_STATUS_CONNECTED:
    {
        if (boardIsOpen() == true)
            _taskAppSetLightOn();
        else
            taskLightAnimSin(200, COLOR_BLUE, 0.2f);

        break;
    }
    case _TASK_APP_STATUS_UNLOCKED:
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
void _taskAppBleEventErrHandle()
{
    boardPrintf("App: ble radio error !\r\n");
    boardLedOn();

    // Init time to restart into 1min
    _taskApp.ticksToRestart = _TASK_APP_RESTART_DELAY_TICK;
    vTaskSetTimeOutState(&_taskApp.timeOutRestart);

    _taskAppSetStatus(_TASK_APP_STATUS_BLE_ERROR);
}

void _taskAppBleEventDisconnectedHandle()
{
    boardPrintf("App: device disconnected.\r\n");
    boardLock();
    _taskAppSetStatus(_TASK_APP_STATUS_DISCONNECTED);
}

void _taskAppBleEventConnectedHandle()
{
    boardPrintf("App: device connected.\r\n");
    _taskAppSetStatus(_TASK_APP_STATUS_CONNECTED);
}

void _taskAppBleEventLockHandle()
{
    boardPrintf("App: Lock the lock.\r\n");
    boardLock();

    _taskAppSetStatus(_TASK_APP_STATUS_CONNECTED);
}

void _taskAppBoardEventDoorStateHandle()
{
    if (boardIsOpen() == true)
    {
        boardPrintf("App: door is open.\r\n");

        // Update BLE characteristic
        uint8_t state = 1; 
        taskBleUpdateAtt(BLE_ATT_DOOR_STATE, &state, 1);
    }
    else
    {
        boardPrintf("App: door is close.\r\n");

        // Update BLE characteristic
        uint8_t state = 0; 
        taskBleUpdateAtt(BLE_ATT_DOOR_STATE, &state, 1);
    }

    _taskAppManageLightColor(_taskApp.status);
}

void _taskAppBoardEventButtonBondStateHandle()
{
    if (boardButtonBondState() == true)
    {
        // Enable clear bonded device timeout
        _taskApp.ticksToClearBonded = _TASK_APP_CLEAR_BONDED_DELAY_TICK;
        vTaskSetTimeOutState(&_taskApp.timeOutClearBonded);
    }
    else
    {
        if (xTaskCheckForTimeOut(&_taskApp.timeOutClearBonded, &_taskApp.ticksToClearBonded) == pdTRUE)
            taskBleClearAllPairing();
        else if (_taskApp.status != _TASK_APP_STATUS_BONDING)
        {
            _taskAppSetStatus(_TASK_APP_STATUS_BONDING);
            taskBleBonding(true);
        }

        // Disable clear bonded device timeout
        _taskApp.ticksToClearBonded = portMAX_DELAY;
    }
}

// Send event implemented fonction
void boardSendEventFromISR(boardEvent_t event,
                           BaseType_t *pxHigherPriorityTaskWoken)
{
    taskAppEventItem_t eventItem = {
        .boardEvent = event};
    xQueueSendFromISR(_taskApp.eventQueue, &eventItem, pxHigherPriorityTaskWoken);
}

// Warning: this function is run in task BLE
void taskBleSendEvent(bleEvent_t event)
{
    switch (event)
    {
    case BLE_EVENT_ERR:
        _taskAppBleEventErrHandle();
        break;
    case BLE_EVENT_CONNECTED:
        _taskAppBleEventConnectedHandle();
        break;
    case BLE_EVENT_DISCONNECTED:
        _taskAppBleEventDisconnectedHandle();
        break;
    }
}
