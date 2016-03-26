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

int blox_sendDirectly(char* line){
    if(!irc_conn){
        return -1;
    }
    if(!line){
        return 0;
    }
    int line_len = strlen(line);
    printf(">> %.*s", line_len, line);
    return irc_conn->write(irc_conn, line, line_len);
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
    char line[8 + lenServ];
    strcpy(line, "PONG :");
    strcat(line, servName);
    strcat(line, "\r\n");

    return blox_sendDirectly(line);//PONGs are considered important, so they're sent directly.. DO NOT send too many PONGs too quickly!
}
