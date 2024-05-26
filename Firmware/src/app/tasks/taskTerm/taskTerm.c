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
 * @file taskTerm.h
 * @author antoine163
 * @date 25-05-2024
 * @brief Task to manage small terminale
 */

// Include ---------------------------------------------------------------------
#include "taskTerm.h"
#include "../taskApp/taskApp.h"
#include "board.h"

#include "vt100.h"

#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

// Define ----------------------------------------------------------------------
#define _TASK_TERM_PROMPT      VT100_TEXT_BOLD VT100_COLOR_GREEN "bsl>" VT100_COLOR_RESET
#define _TASK_TERM_MAX(v1, v2) (v1 > v2) ? v1 : v2
#define _TASK_TERM_MIN(v1, v2) (v1 < v2) ? v1 : v2

// Struct ----------------------------------------------------------------------
typedef struct
{
    char *name;
    int (*func)(int, char *[]);
    char *help;
} taskTermCmd_t;

// Private prototype functions -------------------------------------------------
int _taskTermReadline(char *str, size_t nbyte);

int _taskTermCmdVersion(int argc, char *argv[]);
int _taskTermCmdHelp(int argc, char *argv[]);
int _taskTermCmdVerbose(int argc, char *argv[]);
int _taskTermCmdPin(int argc, char *argv[]);
int _taskTermCmdBri(int argc, char *argv[]);
int _taskTermCmdBriTh(int argc, char *argv[]);
int _taskTermCmdBond(int argc, char *argv[]);
int _taskTermCmdBondClear(int argc, char *argv[]);
int _taskTermCmdReset(int argc, char *argv[]);

// Global variables ------------------------------------------------------------
static taskTermCmd_t _taskTermCmd[] = {
    {
        .name = "version",
        .help = "Display version informations.",
        .func = _taskTermCmdVersion,
    },
    {
        .name = "help",
        .help = "Display help.",
        .func = _taskTermCmdHelp,
    },
    {
        .name = "verbose",
        .help = "Read/write verbose mode. 1 to enable, 0 (default) to disable.",
        .func = _taskTermCmdVerbose,
    },
    {
        .name = "pin",
        .help = "Read/write PIN code (default is 215426).",
        .func = _taskTermCmdPin,
    },
    {
        .name = "bri",
        .help = "Read ambient brightness.",
        .func = _taskTermCmdBri,
    },
    {
        .name = "bri-th",
        .help = "Read/write day/night threshold (default is 50%).",
        .func = _taskTermCmdBriTh,
    },
    {
        .name = "bond",
        .help = "Display the list of paired devices.",
        .func = _taskTermCmdBond,
    },
    {
        .name = "bond-clear",
        .help = "Remove all paired devices (or hold the bond button for more than 3 seconds).",
        .func = _taskTermCmdBondClear,
    },
    {
        .name = "reset",
        .help = "Reset configuration to default.",
        .func = _taskTermCmdReset,
    }};

// Implemented functions -------------------------------------------------------
void taskTermCodeInit()
{
    boardDgbEnable(taskAppGetVerbose());
}

void taskTermCode(__attribute__((unused)) void *parameters)
{
    char buf[64];
    int argc;
    char *argv[4];

    while (1)
    {
        int n = _taskTermReadline(buf, sizeof(buf));
        if (n <= 0)
            continue;
        buf[n] = '\0';
        argc = 0;

        // Extra command name
        char *strChr = buf;
        argv[argc++] = strChr;

        // Extra others arguments
        while (1)
        {
            strChr = strchr(strChr, ' ');
            if (strChr == NULL)
                break;

            *strChr = '\0';
            argv[argc++] = ++strChr;
        }

        // Find and execute commande
        unsigned int i = 0;
        for (; i < sizeof(_taskTermCmd) / sizeof(taskTermCmd_t); i++)
        {
            if (strcasecmp(argv[0], _taskTermCmd[i].name) == 0)
            {
                _taskTermCmd[i].func(argc, argv);
                break;
            }
        }

        if (i >= sizeof(_taskTermCmd) / sizeof(taskTermCmd_t))
        {
            boardPrintf("'%s' unknown command! Please use 'help'.\r\n", argv[0]);
        }
    }
}

int _taskTermReadline(char *str, size_t nbyte)
{
    unsigned int str_n = 0;
    unsigned int str_i = 0;
    str[0] = '\0';

    bool vt100_escape = false;
    char vt100_cmd[8];
    unsigned int vt100_n;

    boardPrintf(_TASK_TERM_PROMPT);

    do
    {
        char c = boardReadChar();

        if (vt100_escape == true)
        {
            vt100_cmd[vt100_n++] = c;
            if (vt100_n > sizeof(vt100_cmd))
                vt100_escape = false; // vt100 cmd tool long

            if (strcmp(vt100_cmd, VT100_CURSOR_LEFT()) == 0) // <-
            {
                if (str_i <= 0)
                    continue;

                str_i--;
                boardPrintf(VT100_CURSOR_LEFT());
            }
            else if (strcmp(vt100_cmd, VT100_CURSOR_RIGHT()) == 0) // ->
            {
                if (str_i >= str_n)
                    continue;

                str_i++;
                boardPrintf(VT100_CURSOR_RIGHT());
            }
            else if (strcmp(vt100_cmd, VT100_ESC "[3~") == 0) // Suppr key
            {
                if (str_n <= str_i)
                    continue;

                str_n--;

                for (unsigned int cp_i = str_i + 1; cp_i < str_n; cp_i++)
                    str[cp_i] = str[cp_i + 1];

                str[str_n] = '\0';

                boardPrintf(VT100_HIDE_CURSOR VT100_SAVE_CURSOR);
                boardPrintf(VT100_CLEAR_LINE_FROM_CURSOR);
                boardPrintf("%s", str + str_i);
                boardPrintf(VT100_RESTORE_CURSOR VT100_SHOW_CURSOR);
            }
            else
                continue;

            // End of vt100 cmd
            vt100_escape = false;
        }
        else if (c == VT100_ESC[0])
        {
            // Start vt100 cmd
            vt100_escape = true;
            memset(vt100_cmd, 0, sizeof(vt100_cmd));
            vt100_n = 0;

            vt100_cmd[vt100_n++] = c;
        }
        else if (c == '\b') // backspace
        {
            if (str_i <= 0)
                continue;

            str_i--;
            str_n--;

            for (unsigned int cp_i = str_i; cp_i < str_n; cp_i++)
                str[cp_i] = str[cp_i + 1];

            str[str_n] = '\0';

            boardPrintf(VT100_HIDE_CURSOR VT100_CURSOR_LEFT());
            boardPrintf(VT100_SAVE_CURSOR VT100_CLEAR_LINE_FROM_CURSOR);
            boardPrintf("%s", str + str_i);
            boardPrintf(VT100_RESTORE_CURSOR VT100_SHOW_CURSOR);
        }
        else if (c == '\f') // clear ctrl + L
        {
            boardPrintf(VT100_CLEAR_SCREEN VT100_RESET);
            boardPrintf(_TASK_TERM_PROMPT "%s", str);
        }
        else if (c == '\t') // TAB
        {
            if (str_n <= 0)
                continue;

            for (unsigned int i = 0; i < sizeof(_taskTermCmd) / sizeof(taskTermCmd_t); i++)
            {
                if (strstr(_taskTermCmd[i].name, str) != NULL)
                    boardPrintf("\r\n%s", _taskTermCmd[i].name);
            }

            boardPrintf("\r\n" _TASK_TERM_PROMPT "%s", str);
            if ((str_n - str_i) > 0)
                boardPrintf(VT100_CURSOR_LEFT(% u), str_n - str_i);
        }
        else if (c == '\r')
        {
            boardPrintf("\r\n");
            break; // exit boardReadline() function
        }
        else
        {
            if (str_n >= nbyte)
                continue;

            for (unsigned int cp_i = str_n; cp_i > str_i; cp_i--)
                str[cp_i] = str[cp_i - 1];

            str[str_i] = c;

            str_i++;
            str_n++;
            str[str_n] = '\0';

            boardPrintf(VT100_HIDE_CURSOR VT100_SAVE_CURSOR);
            boardPrintf("%s", str + str_i - 1);
            boardPrintf(VT100_RESTORE_CURSOR);
            boardPrintf(VT100_CURSOR_RIGHT() VT100_SHOW_CURSOR);
        }
    } while (1);

    str[str_n] = '\0';
    return str_n;
}

int _taskTermCmdVersion(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    boardPrintf("%s - %s\r\n", PROJECT_VERSION, __DATE__);
    return 0;
}

int _taskTermCmdHelp(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    char addspac[32] = {'\0'};
    unsigned int maxCmdChar = 0;
    for (unsigned int i = 0; i < sizeof(_taskTermCmd) / sizeof(taskTermCmd_t); i++)
        maxCmdChar = _TASK_TERM_MAX(maxCmdChar, strlen(_taskTermCmd[i].name));
    maxCmdChar = _TASK_TERM_MIN(maxCmdChar, sizeof(addspac) - 1);

    for (unsigned int i = 0; i < sizeof(_taskTermCmd) / sizeof(taskTermCmd_t); i++)
    {
        size_t len = strlen(_taskTermCmd[i].name);
        memset(addspac, ' ', maxCmdChar - len);
        addspac[maxCmdChar - len] = '\0';

        boardPrintf(VT100_TEXT_BOLD "%s:" VT100_COLOR_RESET, _taskTermCmd[i].name);
        boardPrintf("%s %s\r\n", addspac, _taskTermCmd[i].help);
    }
    return EXIT_SUCCESS;
}

int _taskTermCmdVerbose(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    return EXIT_SUCCESS;
}

int _taskTermCmdPin(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    return EXIT_SUCCESS;
}

int _taskTermCmdBri(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    return EXIT_SUCCESS;
}

int _taskTermCmdBriTh(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    return EXIT_SUCCESS;
}

int _taskTermCmdBond(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    return EXIT_SUCCESS;
}

int _taskTermCmdBondClear(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    return EXIT_SUCCESS;
}

int _taskTermCmdReset(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    return EXIT_SUCCESS;
}
