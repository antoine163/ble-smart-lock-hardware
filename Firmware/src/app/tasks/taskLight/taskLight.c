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

#include <math.h>

// Define ----------------------------------------------------------------------
#define _TASK_LIGHT_EVENT_QUEUE_LENGTH 8

// Conf animation
#define ANIM_STEP_TIME 60 // ms

// Enum ------------------------------------------------------------------------
typedef enum
{
    LIGHT_EVENT_ANIM_TRANS,
    LIGHT_EVENT_ANIM_SIN,
    LIGHT_EVENT_ANIM_BLINK
} taskLightEventType_t;

// Struct ----------------------------------------------------------------------
typedef struct
{
    TickType_t (*func)();
    float currentDc;
    color_t color;
    TickType_t ticksToWait;
    TimeOut_t timeOut;

    // Specific for sin animation
    float sinInitAcos;
    float sinFreq;
    // Specific for blink animation
    bool blinkOn;
    TickType_t blinkTicksOn;
    TickType_t blinkTicksOff;
    // Specific for transient animation
    bool trendToOff;
    float trendDecDc;
    float trendIncDc;
} taskLightAnimation_t;

typedef struct
{
    taskLightEventType_t type;
    color_t color;

    // Specific for sin animation
    float sinFreq;
    // Specific for blink animation
    unsigned int blinkTimeOn;
    unsigned int blinkTimeOff;
    // Specific for transient animation
    unsigned int transTimeToOff;
    unsigned int transTimeToOn;
} taskLightEvent_t;

typedef struct
{
    // Event queue
    QueueHandle_t eventQueue;
    StaticQueue_t eventStaticQueue;
    uint8_t eventQueueStorageArea[sizeof(taskLightEvent_t) * _TASK_LIGHT_EVENT_QUEUE_LENGTH];

    // Light animation
    taskLightAnimation_t anim;
} taskLight_t;

// Global variables ------------------------------------------------------------
static taskLight_t _taskLight = {0};

// Prototype functions ---------------------------------------------------------
void taskLightInitAnimTrans(color_t color,
                            unsigned int timeToOff,
                            unsigned int timeToOn);
TickType_t taskLightFuncAnimFuncTrans();

void taskLightInitAnimSin(color_t color, float freq);
TickType_t taskLightFuncAnimSin();

void taskLightInitAnimBlink(color_t color,
                            unsigned int timeOn,
                            unsigned int timeOff);
TickType_t taskLightFuncAnimBlink();

// Implemented functions -------------------------------------------------------
void taskLightCodeInit()
{
    _taskLight.eventQueue = xQueueCreateStatic(_TASK_LIGHT_EVENT_QUEUE_LENGTH,
                                               sizeof(taskLightEvent_t),
                                               _taskLight.eventQueueStorageArea,
                                               &_taskLight.eventStaticQueue);
}

void taskLightCode(__attribute__((unused)) void *parameters)
{
    taskLightEvent_t event;
    TickType_t tickToWait = 0;

    while (1)
    {
        if (xQueueReceive(_taskLight.eventQueue, &event, tickToWait) == pdPASS)
        {
            switch (event.type)
            {
            case LIGHT_EVENT_ANIM_TRANS:
                taskLightInitAnimTrans(event.color,
                                       event.transTimeToOff,
                                       event.transTimeToOn);
                break;

            case LIGHT_EVENT_ANIM_SIN:
                taskLightInitAnimSin(event.color,
                                     event.sinFreq);
                break;

            case LIGHT_EVENT_ANIM_BLINK:
                taskLightInitAnimBlink(event.color,
                                       event.blinkTimeOn,
                                       event.blinkTimeOff);
                break;

            default:
                break;
            }
        }

        if (_taskLight.anim.func != NULL)
            tickToWait = _taskLight.anim.func();
        else
            tickToWait = portMAX_DELAY;
    }
}

void taskLightSetColor(color_t color, unsigned int transTime)
{
    taskLightAnimTrans(color, 0, transTime);
}

void taskLightAnimTrans(color_t color,
                        unsigned int timeToOff,
                        unsigned int timeToOn)
{
    taskLightEvent_t event = {0};
    event.color = color;
    event.transTimeToOff = timeToOff;
    event.transTimeToOn = timeToOn;

    xQueueSend(_taskLight.eventQueue, &event, portMAX_DELAY);
}

void taskLightAnimSin(color_t color, float freq)
{
    taskLightEvent_t event = {0};
    event.color = color;
    event.sinFreq = freq;

    xQueueSend(_taskLight.eventQueue, &event, portMAX_DELAY);
}

void taskLightAnimBlink(color_t color,
                        unsigned int timeOn,
                        unsigned int timeOff)
{
    taskLightEvent_t event = {0};
    event.color = color;
    event.blinkTimeOn = timeOn;
    event.blinkTimeOff = timeOff;

    xQueueSend(_taskLight.eventQueue, &event, portMAX_DELAY);
}

void taskLightInitAnimTrans(color_t color,
                            unsigned int timeToOff,
                            unsigned int timeToOn)
{
    _taskLight.anim.color = color;
    _taskLight.anim.trendDecDc = (float)timeToOff / (float)ANIM_STEP_TIME;
    _taskLight.anim.trendIncDc = (float)timeToOn / (float)ANIM_STEP_TIME;
    _taskLight.anim.trendToOff = true;

    // Init time
    _taskLight.anim.ticksToWait = ANIM_STEP_TIME / portTICK_PERIOD_MS;
    vTaskSetTimeOutState(&_taskLight.anim.timeOut);

    // Start transient
    _taskLight.anim.func = taskLightFuncAnimFuncTrans;
}

TickType_t taskLightFuncAnimFuncTrans()
{
    if (xTaskCheckForTimeOut(&_taskLight.anim.timeOut, &_taskLight.anim.ticksToWait) != pdFALSE)
    {
        // Init time
        _taskLight.anim.ticksToWait = ANIM_STEP_TIME / portTICK_PERIOD_MS;
        vTaskSetTimeOutState(&_taskLight.anim.timeOut);

        if (_taskLight.anim.trendToOff == true)
        {
            _taskLight.anim.currentDc -= _taskLight.anim.trendDecDc;
            boardSetLightDc(_taskLight.anim.currentDc);

            // Set finale color
            if (_taskLight.anim.currentDc <= 0.f)
            {
                _taskLight.anim.trendToOff = false;
                boardSetLightColor(_taskLight.anim.color);
            }
        }
        else
        {
            _taskLight.anim.currentDc += _taskLight.anim.trendIncDc;
            boardSetLightDc(_taskLight.anim.currentDc);

            // Stop transient
            if (_taskLight.anim.currentDc >= 100.f)
                _taskLight.anim.func = NULL;
        }
    }

    return _taskLight.anim.ticksToWait;
}

void taskLightInitAnimSin(color_t color, float freq)
{
    _taskLight.anim.color = color;
    _taskLight.anim.sinFreq = freq;
    boardSetLightColor(_taskLight.anim.color);

    // Init time
    _taskLight.anim.ticksToWait = ANIM_STEP_TIME / portTICK_PERIOD_MS;
    vTaskSetTimeOutState(&_taskLight.anim.timeOut);

    _taskLight.anim.sinInitAcos = acosf(_taskLight.anim.currentDc);
    _taskLight.anim.func = taskLightFuncAnimSin;
}

TickType_t taskLightFuncAnimSin()
{
    if (xTaskCheckForTimeOut(&_taskLight.anim.timeOut, &_taskLight.anim.ticksToWait) != pdFALSE)
    {
        // Init time
        _taskLight.anim.ticksToWait = ANIM_STEP_TIME / portTICK_PERIOD_MS;
        vTaskSetTimeOutState(&_taskLight.anim.timeOut);

        float tick = xTaskGetTickCount();
        _taskLight.anim.currentDc =
            50.f * (1.f - cosf(_taskLight.anim.sinInitAcos + tick * (M_TWOPI * _taskLight.anim.sinFreq * portTICK_PERIOD_MS / 1000.f)));
        boardSetLightDc(_taskLight.anim.currentDc);
    }

    return _taskLight.anim.ticksToWait;
}

void taskLightInitAnimBlink(color_t color,
                            unsigned int timeOn,
                            unsigned int timeOff)
{
    _taskLight.anim.color = color;
    _taskLight.anim.blinkTicksOn = timeOn / portTICK_PERIOD_MS;
    _taskLight.anim.blinkTicksOff = timeOff / portTICK_PERIOD_MS;
    boardSetLightColor(_taskLight.anim.color);
    _taskLight.anim.currentDc = 100.f;
    boardSetLightDc(_taskLight.anim.currentDc);

    // Init time
    _taskLight.anim.ticksToWait = _taskLight.anim.blinkTicksOn;
    vTaskSetTimeOutState(&_taskLight.anim.timeOut);

    _taskLight.anim.blinkOn = true;
    _taskLight.anim.func = taskLightFuncAnimBlink;
}

TickType_t taskLightFuncAnimBlink()
{
    if (xTaskCheckForTimeOut(&_taskLight.anim.timeOut, &_taskLight.anim.ticksToWait) != pdFALSE)
    {
        // Init time
        if (_taskLight.anim.blinkOn == true)
        {
            _taskLight.anim.blinkOn = false;
            _taskLight.anim.ticksToWait = _taskLight.anim.blinkTicksOff;
            boardSetLightColor(COLOR_OFF);
            _taskLight.anim.currentDc = 0.f;
        }
        else
        {
            _taskLight.anim.blinkOn = true;
            _taskLight.anim.ticksToWait = _taskLight.anim.blinkTicksOn;
            boardSetLightColor(_taskLight.anim.color);
            _taskLight.anim.currentDc = 100.f;
        }
        vTaskSetTimeOutState(&_taskLight.anim.timeOut);
    }

    return _taskLight.anim.ticksToWait;
}
