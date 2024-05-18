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

#ifndef BOARD_H
#define BOARD_H

// Includes --------------------------------------------------------------------
#include "BlueNRG1_gpio.h"
#include "mapHard.h"

#include <FreeRTOS.h>

// Define ----------------------------------------------------------------------
// #define SW_VERSION_MAJOR          0
// #define SW_VERSION_MINOR          0
// #define SW_VERSION_BUILD          1

// #define STRINGIFY(x) #x
// #define SW_VERSION_BUILD_STR(major, minor, build)   "v" STRINGIFY(major) "." STRINGIFY(minor) "." STRINGIFY(build)
// #define SW_VERSION_STR      SW_VERSION_BUILD_STR(SW_VERSION_MAJOR, SW_VERSION_MINOR, SW_VERSION_BUILD)

// Enum ------------------------------------------------------------------------
typedef enum
{
    COLOR_OFF,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_CYAN,
    COLOR_MAGENTA,
    COLOR_WHITE,
    COLOR_WHITE_LIGHT
} color_t;

typedef enum
{
    BOARD_EVENT_BUTTON_BOND_STATE,
    BOARD_EVENT_DOOR_STATE
} boardEvent_t;

// Prototype functions ---------------------------------------------------------
void boardInit();
int boardPrintf(char const *format, ...);

void boardSetLightColor(color_t color);
void boardSetLightDc(float dc);

float boardGetBrightness();

void boardLock();
void boardUnlock();
bool boardIsLocked();

void boardOpen();
bool boardIsOpen();

// Function called by the ISR board to send an event to the App task
// This function is implemented in the App task
void boardSendEventFromISR(boardEvent_t event,
                           BaseType_t *pxHigherPriorityTaskWoken);

// Prototype static functions --------------------------------------------------
static inline void boardLedOn()
{
    GPIO_SetBits(LED_PIN);
}

static inline void boardLedOff()
{
    GPIO_ResetBits(LED_PIN);
}

static inline void boardLedToggel()
{
    GPIO_ToggleBits(LED_PIN);
}

static inline bool boardButtonBondState()
{
    return GPIO_ReadBit(BOND_PIN) == Bit_SET;
}

#endif // BOARD_H
