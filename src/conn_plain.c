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

#include "conn_plain.h"

#include <sys/socket.h>

struct _conn_plain_ud{
    int sockfd;
};

/* This function is slightly deceiving!
 * This function will only return after a complete line has been read.
 */
size_t _conn_plain_read(bloxbot_Conn* conn, char* buf, int count){
    if(!buf){
        return -1;
    }

    struct _conn_plain_ud* mode_ud = (struct _conn_plain_ud*)conn->ud;

    int numRead = 0;
    for(int i = 0; i < count; i++){
        int ret = recv(mode_ud->sockfd, &buf[i], 1, 0);

        if(ret == 0){
            return 0;
        }

        if(ret == -1){
            //TODO: Handle errors
        }

        numRead++;
        if(buf[i] == '\n'){
            buf[i] = '\0';
            break;
        }
    }

    return numRead;
}

size_t _conn_plain_write(bloxbot_Conn* conn, char* buf, size_t count){
    struct _conn_plain_ud* mode_ud = (struct _conn_plain_ud*)conn->ud;

    int ret = -1;

    ret = send(mode_ud->sockfd, buf, count, 0);

    if(ret == -1){
        //Handle error
    }

    return ret;
}

void _conn_plain_close(bloxbot_Conn* conn){
    struct _conn_plain_ud* mode_ud = (struct _conn_plain_ud*)conn->ud;
    shutdown(mode_ud->sockfd, SHUT_RDWR);
}

bloxbot_Conn* bloxbot_conn_plain(char* addr, int port){
    struct _conn_plain_ud* mode_ud = malloc(sizeof(struct _conn_plain_ud));
    if(!mode_ud){
        return NULL;
    }
    bzero(mode_ud, sizeof(struct _conn_plain_ud));

    mode_ud->sockfd = net_util_connect(addr, port);

    if(mode_ud->sockfd == -1){
        free(mode_ud);
        return NULL;
    }

    //Get everything ready to return
    bloxbot_Conn* conn = malloc(sizeof(bloxbot_Conn));
    if(!conn){
        return NULL;
    }
    bzero(conn, sizeof(bloxbot_Conn));

    conn->read = _conn_plain_read;
    conn->write = _conn_plain_write;
    conn->close = _conn_plain_close;

    conn->ud = mode_ud;

    return conn;
}
