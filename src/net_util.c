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

#include "net_util.h"

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int net_util_connect(char* addr, int port){
    int ret;
    struct addrinfo hints;
    struct addrinfo* servinfo;

    // Max digits in a port: 5. Let's use 6 characters so we have some
    // padding
    char str_port[6];
    sprintf(str_port, "%d", port);

    bzero(&hints, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((ret = getaddrinfo(addr, str_port, &hints, &servinfo))){
        fprintf(stderr, "Failed to resolve: %s\n", gai_strerror(ret));
        return -1;
    }

    struct addrinfo* p;

    char ipstr[INET_ADDRSTRLEN];

    for(p = servinfo; p != NULL; p = p->ai_next){
        struct in_addr* addr;
        if(p->ai_family == AF_INET){
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
            addr = &(ipv4->sin_addr);
        }else{
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = (struct in_addr*)&(ipv6->sin6_addr);
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));

        int sockfd = socket(p->ai_family, SOCK_STREAM, 0);
        if(sockfd != -1){
            if(connect(sockfd, p->ai_addr, p->ai_addrlen) == 0){
                freeaddrinfo(servinfo);
                return sockfd;
            }
        }
    }

    freeaddrinfo(servinfo);

    puts("Failed to connect.");

    return -1;
}
