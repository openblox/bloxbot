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

#include "bloxbot.h"

#include "internal.h"

#include <stdio.h>
#include <string.h>


#include <pthread.h>

pthread_mutex_t _sock_lock;

void _bb_init_bb(){
    if(pthread_mutex_init(&_sock_lock, NULL) != 0){
        puts("Failed to initialize mutex.");
        exit(EXIT_FAILURE);
    }
}

int blox_sendDirectlyl(char* line, int len){
    if(!irc_conn){
        return -1;
    }
    if(!line){
        return 0;
    }
    if(bb_isVerbose){
        printf(">> %.*s", len, line);
    }
    pthread_mutex_lock(&_sock_lock);
    int ret = irc_conn->write(irc_conn, line, len);
    pthread_mutex_unlock(&_sock_lock);
    return ret;
}

int blox_sendDirectly(char* line){
    if(!line){
        return 0;
    }
    int line_len = strlen(line);
    return blox_sendDirectlyl(line, line_len);
}

int blox_send(char* line){
    if(!line){
        return 0;
    }
    long int cT = _bb_curtime();
    if((cT - irc_last_msg) > BB_MSG_DEBUF){
        blox_sendDirectly(line);
        irc_last_msg = cT;
    }else{
        _bb_push_queue(line);
    }
    return 0;
}

int blox_sendMsg(char* target, char* msg){
	if(!target){
		return 0;
	}
	if(!msg){
		return 0;
	}

	size_t lenChan = strlen(target);
	size_t lenMsg = strlen(msg);
	char line[12 + lenChan + lenMsg];
	strcpy(line, "PRIVMSG ");
	strcat(line, target);
	strcat(line, " :");
	strcat(line, msg);
	strcat(line, "\r\n");

	return blox_send(line);
}

int blox_join(char* chan){
    if(!chan){
        return 0;
    }
    size_t lenChan = strlen(chan);
    char line[7 + lenChan];
    strcpy(line, "JOIN ");
    strcat(line, chan);
    strcat(line, "\r\n");

    return blox_send(line);
}

int blox_part(char* chan){
    if(!chan){
        return 0;
    }
    size_t lenChan = strlen(chan);
    char line[7 + lenChan];
    strcpy(line, "PART ");
    strcat(line, chan);
    strcat(line, "\r\n");

    return blox_send(line);
}

int blox_partr(char* chan, char* reason){
    if(!chan){
        return 0;
    }
    size_t lenChan = strlen(chan);
    size_t lenReason = strlen(reason);
    char line[9 + lenChan + lenReason];
    strcpy(line, "PART ");
    strcat(line, chan);
    strcat(line, " :");
    strcat(line, reason);
    strcat(line, "\r\n");

    return blox_send(line);
}

int blox_pong(char* servName){
    if(!servName){
        return 0;
    }
    size_t lenServ = strlen(servName);
	size_t lenWhole = 8 + lenServ;
    char line[lenWhole];
    strcpy(line, "PONG :");
    strcat(line, servName);
    strcat(line, "\r\n");

    return blox_sendDirectlyl(line, lenWhole);//PONGs are considered important, so they're sent directly.. DO NOT send too many PONGs too quickly!
}
