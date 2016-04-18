/*
 * Copyright (C) 2016 John M. Harris, Jr. <johnmh@openblox.org>
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

#ifndef BB_BLOXBOT_H_
#define BB_BLOXBOT_H_

extern unsigned int bb_isVerbose;

#include "config.h"

#include "conn.h"

void _bb_init_bb();

//cppcheck-suppress unusedFunction
int blox_sendDirectlyl(char* line, int len);
//cppcheck-suppress unusedFunction
int blox_sendDirectly(char* line);
//cppcheck-suppress unusedFunction
int blox_send(char* line);
//cppcheck-suppress unusedFunction
int blox_sendMsg(char* target, char* msg);
//cppcheck-suppress unusedFunction
int blox_join(char* chan);
//TODO: Support channels with passwords?
//cppcheck-suppress unusedFunction
int blox_part(char* chan);
//cppcheck-suppress unusedFunction
int blox_partr(char* chan, char* reason);
//cppcheck-suppress unusedFunction
int blox_pong(char* servName);

#endif
