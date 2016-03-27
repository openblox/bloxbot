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

#include "internal.h"

#include "bloxbot.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <sys/time.h>

struct bb_QueueItem* curQueue = NULL;

void _bb_run_queue(){
    long int cT = _bb_curtime();
    if((cT - irc_last_msg) > BB_MSG_DEBUF){
        if(curQueue){
            puts("Running item");
            struct bb_QueueItem* oldItem = curQueue;
            curQueue = oldItem->next;

            blox_sendDirectly(oldItem->line);
            free(oldItem->line);
            free(oldItem);

            irc_last_msg = cT;
        }
    }
}

void _bb_push_queue(char* line){
    if(!line){
        return;
    }
    struct bb_QueueItem* qi = malloc(sizeof(struct bb_QueueItem));
    if(!qi){
        puts("Out of memory");
        exit(EXIT_FAILURE);
    }
    bzero(qi, sizeof(struct bb_QueueItem));
    qi->next = NULL;
    qi->line = strdup(line);

    if(!curQueue){
        curQueue = qi;
        return;
    }

    struct bb_QueueItem* qii = curQueue;
    while(qii->next != NULL){
        qii = qii->next;
    }

    if(qii){
        qii->next = qi;
    }
}

long int _bb_curtime(){
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
