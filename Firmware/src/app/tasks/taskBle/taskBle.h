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
 * @file taskBle.h
 * @author antoine163
 * @date 02-04-2024
 * @brief Task of BLE radio
 */

#ifndef TASK_BLE_H
#define TASK_BLE_H

// Enum ------------------------------------------------------------------------

// typedef enum
// {
//     BLE_ATT_LOCK_STATE,
//     BLE_ATT_DOOR_STATE,
//     BLE_ATT_OPEN_DOOR,
//     BLE_ATT_BRIGHTNESS,
//     BLE_ATT_BRIGHTNESS_TH
// } bleAtt_t;

// Event send by the BLE task.
typedef enum
{
    BLE_EVENT_ERR,
    BLE_EVENT_CONNECTED,
    BLE_EVENT_DISCONNECTED,
} bleEvent_t;

// Prototype functions ---------------------------------------------------------
void taskBleCodeInit();
void taskBleCode(void *parameters);

void taskBleEnableBonding(bool enable);

// Function called by the BLE task to send an event to the App task
// This function is implemented in the App task
void taskBleSendEvent(bleEvent_t event);

// int taskBleReadAtt(taskBleAtt_t att, void *buf, size_t nbyte);
// int taskBleWritAtt(taskBleAtt_t att, const void *buf, size_t nbyte);

#endif // TASK_BLE_H
