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
#include <stdbool.h>

#include "BlueNRG1_gpio.h"
#include "map_hard.h"
#include "tick.h"


// Define ----------------------------------------------------------------------
#define SW_VERSION_MAJOR          0
#define SW_VERSION_MINOR          0
#define SW_VERSION_BUILD          1

#define STRINGIFY(x) #x
#define SW_VERSION_BUILD_STR(major, minor, build)   "v" STRINGIFY(major) "." STRINGIFY(minor) "." STRINGIFY(build)
#define SW_VERSION_STR      SW_VERSION_BUILD_STR(SW_VERSION_MAJOR, SW_VERSION_MINOR, SW_VERSION_BUILD)


// Prototype functions ---------------------------------------------------------
void boardInit();

static inline void boardWait(unsigned int timer_ms)
{
    tick_delay(timer_ms / TICK_PERIOD_MS);
}

static inline void boardLedOn()
{
    GPIO_SetBits(LED_PIN);
}

static inline void boardLedOff()
{
    GPIO_ResetBits(LED_PIN);
}

static inline void boardLedToggle(uint32_t )
{
    // GPIO_ToggleBits(led);

    // if (led == LED1)
    // {
    //     GPIO_ToggleBits(led);
    // }
}
static inline bool boardLedIsOn(uint32_t )
{
    // return (GPIO_ReadBit(led) == Bit_SET);
    return false;
}

static inline bool boardReadButton(uint32_t )
{
    // return GPIO_ReadBit(button) == Bit_SET;
    return false;
}


typedef enum light_e
{
    LIGHT_RED   = 0x01,
    LIGHT_GREEN = 0x02,
    LIGHT_BLUE  = 0x04,
    LIGHT_WHITE = LIGHT_RED | LIGHT_GREEN | LIGHT_BLUE
}light_t;

void boardLight(light_t light, unsigned int duty_cycle);

#endif // BOARD_H
