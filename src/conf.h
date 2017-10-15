/*
 * Copyright (C) 2017 John M. Harris, Jr. <johnmh@openblox.org>
 *
 * This file is part of bloxbot.
 *
 * bloxbot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bloxbot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bloxbot.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BB_CONF_H_
#define BB_CONF_H_

// We include bloxbot.h here so that plugins don't have to do the runaround
#include "bloxbot.h"

// int
#define BLOXBOT_CONF_ENT_TYPE_INT 1
// char*
#define BLOXBOT_CONF_ENT_TYPE_STR 2
// bloxbot_ConfigEntry**
#define BLOXBOT_CONF_ENT_TYPE_ARRAY 3

typedef struct bloxbot_ConfigEntry{
    unsigned char type;
    int refs;
    union{
        int integer;
        char cchar;
        unsigned char uchar;
        struct{
            int len;
            char* str;
        } str;
        struct{
            int len;
            struct bloxbot_ConfigEntry** array;
        } array;
    } data;
} bloxbot_ConfigEntry;

unsigned char bb_loadConfig(char* name);
unsigned char bb_reloadConfig(char* newConfig);
unsigned char bb_configLoaded();

unsigned char bb_hasConfigEntry(char* name);
bloxbot_ConfigEntry* bb_getConfigEntry(char* name);
void bb_releaseConfigEntry(bloxbot_ConfigEntry* ent);

#endif
