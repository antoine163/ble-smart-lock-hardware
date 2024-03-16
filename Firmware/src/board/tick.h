/***
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 antoine163
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
 * @file tick.h
 * @author antoine163
 * @date 15.07.2023
 * @brief It is a tick driver base on SysTick
 */

#ifndef TICK_H
#define TICK_H

// Include ---------------------------------------------------------------------
#include "bluenrg_x_device.h"

// Define macro ----------------------------------------------------------------
#if (HS_SPEED_XTAL == HS_SPEED_XTAL_32MHZ) && (FORCE_CORE_TO_16MHZ != 1)
#define SYSCLK_FREQ 	32000000UL
#elif (HS_SPEED_XTAL == HS_SPEED_XTAL_16MHZ) || (FORCE_CORE_TO_16MHZ == 1)
#define SYSCLK_FREQ 	16000000UL
#else
#error "No definition for SYSCLK_FREQ"
#endif

#define TICK_CLOCK_HZ       SYSCLK_FREQ
#define TICK_RATE_HZ        1000UL
#define TICK_PERIOD_MS      (1000UL / TICK_RATE_HZ)
#define TICK_MAX            (tick_t)(-1)
#define TICK_FROM_MS(ms)    ( ms * TICK_RATE_HZ / 1000UL )
#define TICK_TO_MS(tick)    ( tick * 1000UL / TICK_TICK_RATE_HZ )

// Typedef ---------------------------------------------------------------------
typedef uint32_t tick_t;

// Global variables ------------------------------------------------------------
extern volatile tick_t g_tick_counter;

// Prototype functions ---------------------------------------------------------
void tick_delay(tick_t ticks);

// Static inline functions -----------------------------------------------------
static inline void tick_init()
{
    SysTick_Config( TICK_CLOCK_HZ / TICK_RATE_HZ );
}

static inline void tick_deinit()
{
    SysTick->CTRL = 0;
}

static inline tick_t tick_get()
{
    return g_tick_counter;
}

// ISR handler -----------------------------------------------------------------
static inline void _tick_isr_handler()
{
    g_tick_counter++;
}

#endif // TICK_H
