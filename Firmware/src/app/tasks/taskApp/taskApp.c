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
#include "BlueNRG1_flash.h"
#include "taskApp.h"
#include "tasks/taskLight/taskLight.h"
#include "tasks/taskBle/taskBle.h"

#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

// Define ----------------------------------------------------------------------
#define _TASK_APP_FLASH_ERASE_GUARD_TIME 25 // 25 ms
#define _TASK_APP_FLASH_WRITE_GUARD_TIME 1  // 1 ms

#define _TASK_APP_RESTART_DELAY_TICK (1 * 60 * 1000 / portTICK_PERIOD_MS)    //!< Tick to wait before restarting following error detection: 1min
#define _TASK_APP_OFF_LIGHT_DELAY_TICK (15 * 60 * 1000 / portTICK_PERIOD_MS) //!< Tick to wait before torn off light following disconnection: 15min
#define _TASK_APP_CLEAR_BONDED_DELAY_TICK (3 * 1000 / portTICK_PERIOD_MS)    //!< Tick to wait before clear all bonded devices from push bond button: 3s

#define _TASK_APP_EVENT_QUEUE_LENGTH 8
#define _TASK_APP_DATA_STORAGE_PAGE (N_PAGES - 3)

#define _TASK_APP_CHECK_VERBOSE(verbose) ((verbose == true) || (verbose == false))
#define _TASK_APP_CHECK_PIN(pin) (pin <= 999999)
#define _TASK_APP_CHECK_BRIGHTNESS_TH(th) ((th >= 0.) && (th <= 100.))

// Enum ------------------------------------------------------------------------
/**
 * @brief Enum representing the application status of the task.
 */
typedef enum
{
    _TASK_APP_STATUS_BLE_ERROR,    /**< Indicates a BLE radio error. */
    _TASK_APP_STATUS_BONDING,      /**< Indicates that the device is bonding. */
    _TASK_APP_STATUS_DISCONNECTED, /**< Indicates that the device is disconnected. */
    _TASK_APP_STATUS_CONNECTED,    /**< Indicates that the device is connected. */
    _TASK_APP_STATUS_UNLOCKED      /**< Indicates that the device is unlocked. */
} taskAppStatus_t;

/**
 * @brief Enum representing the different events for the application task.
 */
typedef enum
{
    _TASK_APP_EVENT_BOARD,        /**< Event related to the board. */
    _TASK_APP_EVENT_WRITE_NVM     /**< Event for writing to non-volatile memory. */
} taskAppEvent_t;


// Struct ----------------------------------------------------------------------

/**
 * @brief Struct representing the non-volatile memory data for the application task.
 */
typedef struct
{
    bool verbose;         /**< Verbose mode flag. */
    uint32_t pin;         /**< PIN number. */
    float brightnessTh;   /**< Brightness threshold value. */
} taskAppNvmData_t;

/**
 * @brief Struct representing an item of an application task event.
 */
typedef struct
{
    taskAppEvent_t event; /**< Type of the event. */
    union
    {
        boardEvent_t boardEvent;      /**< Event related to the board. */
        taskAppNvmData_t nvmNewData;  /**< New non-volatile memory data. */
    };
} taskAppEventItem_t;


typedef struct
{
    // Event queue
    QueueHandle_t eventQueue;
    StaticQueue_t eventQueueBuffer;
    uint8_t eventQueueStorageArea[sizeof(taskAppEventItem_t) * _TASK_APP_EVENT_QUEUE_LENGTH];

    // App status
    taskAppStatus_t status;

    // Timeout to manage reset follows an error.
    TickType_t ticksToRestart;
    TimeOut_t timeOutRestart;

    // Timeout to manage light off follows an disconnection.
    TickType_t ticksToOffLight;
    TimeOut_t timeOutOffLight;

    // Timeout to manage clean all bonded device follows pushed bond button.
    TickType_t ticksToClearBonded;
    TimeOut_t timeOutClearBonded;
} taskApp_t;

// Global variables ------------------------------------------------------------
NO_INIT_SECTION(const volatile taskAppNvmData_t _taskAppNvmData, ".noinit.app_flash_data");
static taskApp_t _taskApp = {0};
static const taskAppNvmData_t _taskAppNvmDefaultData = {
    .verbose = false,
    .pin = TASK_BLE_DEFAULT_FIX_PIN,
    .brightnessTh = 50.f};

// Private prototype functions -------------------------------------------------
void _taskAppManageLightColor(taskAppStatus_t lastStatus);

void _taskAppSetStatus(taskAppStatus_t status);
void _taskAppSetLightOn();

void _taskAppBleEventErrHandle();
void _taskAppBleEventDisconnectedHandle();
void _taskAppBleEventConnectedHandle();

void _taskAppBoardEventDoorStateHandle();
void _taskAppBoardEventButtonBondStateHandle();

int _taskAppNvmWrite(const taskAppNvmData_t *newData);
int _taskAppNvmInit();

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

    _taskAppNvmInit();
}

void taskAppCode(__attribute__((unused)) void *parameters)
{
    taskAppEventItem_t eventItem;

    // Set brightens threshold BLE attribut
    taskBleUpdateAtt(BLE_ATT_BRIGHTNESS_TH,
                     (const void *)&_taskAppNvmData.brightnessTh,
                     sizeof(_taskAppNvmData.brightnessTh));

    // Door is open ?
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
            switch (eventItem.event)
            {
            case _TASK_APP_EVENT_BOARD:
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
                break;
            }

            case _TASK_APP_EVENT_WRITE_NVM:
            {
                int n = _taskAppNvmWrite(&eventItem.nvmNewData);
                if (n == 0)
                    boardPrintf("App: NVM memory written!\r\n");
                else
                    boardPrintf("App: NVM memory writ fail!\r\n");

                break;
            }

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
    }
}

float taskAppGetBrightnessTh()
{
    return _taskAppNvmData.brightnessTh;
}

int taskAppSetBrightnessTh(float th)
{
    if (!_TASK_APP_CHECK_BRIGHTNESS_TH(th))
        return -1;

    if (th == _taskAppNvmData.brightnessTh)
        return 0;

    taskAppEventItem_t eventItem = {.event = _TASK_APP_EVENT_WRITE_NVM};
    memcpy((void *)&eventItem.nvmNewData,
           (const void *)&_taskAppNvmData,
           sizeof(taskAppNvmData_t));
    eventItem.nvmNewData.brightnessTh = th;

    xQueueSend(_taskApp.eventQueue, &eventItem, portMAX_DELAY);

    if (taskBleIsCurrent() == false)
        taskBleUpdateAtt(BLE_ATT_BRIGHTNESS_TH, &th, sizeof(th));

    return 0;
}

uint32_t taskAppGetPin()
{
    return _taskAppNvmData.pin;
}

int taskAppSetPin(uint32_t pin)
{
    if (!_TASK_APP_CHECK_PIN(pin))
        return -1;

    if (pin == _taskAppNvmData.pin)
        return 0;

    taskAppEventItem_t eventItem = {.event = _TASK_APP_EVENT_WRITE_NVM};
    memcpy((void *)&eventItem.nvmNewData,
           (const void *)&_taskAppNvmData,
           sizeof(taskAppNvmData_t));
    eventItem.nvmNewData.pin = pin;

    xQueueSend(_taskApp.eventQueue, &eventItem, portMAX_DELAY);
    return 0;
}

bool taskAppGetVerbose()
{
    return _taskAppNvmData.verbose;
}

int taskAppSetVerbose(bool verbose)
{
    if (!_TASK_APP_CHECK_VERBOSE(verbose))
        return -1;

    if (verbose == _taskAppNvmData.verbose)
        return 0;

    taskAppEventItem_t eventItem = {.event = _TASK_APP_EVENT_WRITE_NVM};
    memcpy((void *)&eventItem.nvmNewData,
           (const void *)&_taskAppNvmData,
           sizeof(taskAppNvmData_t));
    eventItem.nvmNewData.verbose = verbose;

    xQueueSend(_taskApp.eventQueue, &eventItem, portMAX_DELAY);
    return 0;
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
        taskLightAnimSin(200, COLOR_GREEN, 1);
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
    if (boardGetBrightness() <= _taskAppNvmData.brightnessTh)
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
        // Enable timeout to disable bond if the button is not release during 3s
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

// Attention, ensure that the radio is not currently executing and will not be
// during the write operation before calling this function.
int _taskAppNvmWrite(const taskAppNvmData_t *newData)
{
    if (newData == NULL)
        return 0;

    // Wait a sufficiently long time before the next radio activity to proceed
    // with the erase operation.
    while (taskBleNextRadioTime_ms() < _TASK_APP_FLASH_ERASE_GUARD_TIME)
        taskYIELD();
    taskBlePauseRadio();

    // Erase the DATA_STORAGE_PAGE before erase operation
    FLASH_ErasePage(_TASK_APP_DATA_STORAGE_PAGE);

    // Wait for the end of erase operation
    while (FLASH_GetFlagStatus(Flash_CMDDONE) != SET)
        taskYIELD();

    // Can resume the radio at the end of the erase operation.
    taskBleResumeRadio();

    // Program all words of taskAppNvmData_t
    for (size_t i = 0; i < sizeof(taskAppNvmData_t); i += N_BYTES_WORD)
    {
        // Wait a sufficiently long time before the next radio activity to proceed
        // with the write operation.
        while (taskBleNextRadioTime_ms() < _TASK_APP_FLASH_WRITE_GUARD_TIME)
            taskYIELD();
        taskBlePauseRadio();

        // Program the word
        uint32_t word = ((const uint32_t *)newData)[i / N_BYTES_WORD];
        FLASH_ProgramWord(((uint32_t)&_taskAppNvmData) + i, word);

        // Wait for the end of write operation
        while (FLASH_GetFlagStatus(Flash_CMDDONE) != SET)
            taskYIELD();

        // Can resume the radio at the end of the write operation.
        taskBleResumeRadio();
    }

    return 0;
}

int _taskAppNvmInit()
{
    // Check possible value
    if (!_TASK_APP_CHECK_VERBOSE(_taskAppNvmData.verbose) ||
        !_TASK_APP_CHECK_PIN(_taskAppNvmData.pin) ||
        !_TASK_APP_CHECK_BRIGHTNESS_TH(_taskAppNvmData.brightnessTh))
    {
        if (_taskAppNvmWrite(&_taskAppNvmDefaultData) == 0)
            boardPrintf("NVM memory written with default values!\r\n");
        else
            boardPrintf("NVM memory writ fail with default values!\r\n");
    }

    return 0;
}

// Send event implemented fonction
void boardSendEventFromISR(boardEvent_t event,
                           BaseType_t *pxHigherPriorityTaskWoken)
{
    taskAppEventItem_t eventItem = {
        .event = _TASK_APP_EVENT_BOARD,
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
