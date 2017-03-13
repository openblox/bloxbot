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

#ifndef BB_INTERNAL_H_
#define BB_INTERNAL_H_

#include "conn.h"

#define BB_MSG_DEBUF 500
#define MAX_BUFFER_LEN 512

extern bloxbot_Conn* irc_conn;
extern long int irc_last_msg;

struct bb_QueueItem{
    char* line;
    struct bb_QueueItem* next;
};

void _bb_init_internal();

void _bb_run_queue();
void _bb_push_queue(char* line);
long int _bb_curtime();

void _bb_shutdown();

#endif
