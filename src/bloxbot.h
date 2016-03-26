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

int blox_sendDirectlyl(char* line, int len);
int blox_sendDirectly(char* line);
int blox_send(char* line);
int blox_join(char* chan);
//TODO: Support channels with passwords?
int blox_part(char* chan);
int blox_partr(char* chan, char* reason);
int blox_pong(char* servName);

#endif
