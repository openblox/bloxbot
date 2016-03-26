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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <getopt.h>

#include <gnutls/gnutls.h>

static unsigned char stillConnected = 1;
unsigned char bb_verifyTLS = 1;
unsigned int bb_isVerbose = 1;
bloxbot_Conn* irc_conn = NULL;
long int irc_last_msg = 0;

int main(int argc, char* argv[]){
    //TODO: Configuration
    char* server = "irc.openblox.org";
    int port = 6697;
    unsigned char useTLS = 1;

    char* irc_user = NULL;
    char* irc_nick = NULL;
    char* irc_gecos = NULL;
    char* irc_pass = NULL;

    static struct option long_opts[] = {
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"no-tls-verification", no_argument, 0, 'N'},
        {"verbose", no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };

    int opt_idx = 0;

    int c;
    while(1){
        c = getopt_long(argc, argv, "vhNV", long_opts, &opt_idx);

        if(c == -1){
            break;
        }

        switch(c){
            case 'v': {
                puts(PACKAGE_STRING);
                puts("Copyright (C) 2016 John M. Harris, Jr.");
                puts("This is free software. It is licensed for use, modification and");
                puts("redistribution under the terms of the GNU General Public License,");
                puts("version 3 or later <https://gnu.org/licenses/gpl.html>");
                puts("");
                puts("Please send bug reports to: <" PACKAGE_BUGREPORT ">");
                exit(EXIT_SUCCESS);
                break;
            }
            case 'h': {
                puts(PACKAGE_NAME " - Advanced IRC bot");
                printf("Usage: %s [options]\n", argv[0]);
                puts("");
                puts("   -N, --no-tls-verification   Disables TLS server verification");
                puts("");
                puts("   -v, --version               Prints version information and exits");
                puts("   -h, --help                  Prints this help text and exits");
                puts("");
                puts("Options are specified by doubled hyphens and their name or by a single");
                puts("hyphen and the flag character.");
                puts("");
                puts("This IRC bot was originally created for the OpenBlox IRC network, where");
                puts("it handles commands in official channels and offers integration with");
                puts("with the OpenBlox internal network, from IRC.");
                puts("");
                puts("Please send bug reports to: <" PACKAGE_BUGREPORT ">");
                exit(EXIT_SUCCESS);
                break;
            }
            case 'N':{
                bb_verifyTLS = !bb_verifyTLS;
                break;
            }
            case 'V': {
                bb_isVerbose = !bb_isVerbose;
                break;
            }
            case '?': {
                //getopt already handled it
                exit(EXIT_FAILURE);
                break;
            }
            default: {
                exit(EXIT_FAILURE);
            }
        }
    }

    //TODO: Read config from configuration file

    if(!irc_user){
        irc_user = "bloxbot";
    }
    if(!irc_nick){
        irc_nick = "bloxbot";
    }
    if(!irc_gecos){
        irc_gecos = "OpenBlox Utility Bot v" PACKAGE_VERSION;
    }

    //Init
    gnutls_global_init();

    //Actually do the connection
    bloxbot_open_fnc connFunc = NULL;

    if(useTLS){
        connFunc = bloxbot_conn_tls;
    }else{
        connFunc = bloxbot_conn_plain;
    }

    irc_conn = connFunc(server, port);

    if(!irc_conn){
        return EXIT_FAILURE;
    }

    char inBuffer[MAX_BUFFER_LEN+1];
    size_t bufOff = 0;

    unsigned char doneReg = 0;

    int ret = 1;
    while(ret != -1 && stillConnected){
        _bb_run_queue();

        ret = irc_conn->read(irc_conn, inBuffer, MAX_BUFFER_LEN);

        if(ret < 0){
            exit(EXIT_FAILURE);
            break;//Obviously this isn't necessary
        }

        if(ret == BB_EOF){
            stillConnected = 0;
            break;
        }

        size_t realEnd = 0;

        unsigned char hadEnd = 0;
        for(size_t i = bufOff; i < (bufOff + ret); i++){
            if(inBuffer[i] == '\r' || inBuffer[i] == '\n'){
                inBuffer[i] = '\0';
                hadEnd = 1;
                realEnd = i;
                break;
            }
        }

        if(!hadEnd){
            bufOff = bufOff + ret;
            continue;
        }else{
            bufOff = 0;
        }

        if(!doneReg){
            int nickLineLen = 7 + strlen(irc_nick);
            char nickLine[nickLineLen];
            strcpy(nickLine, "NICK ");
            strcat(nickLine, irc_nick);
            strcat(nickLine, "\r\n");

            irc_conn->write(irc_conn, nickLine, nickLineLen);

            int userLineLen = 13 + strlen(irc_user) + strlen(irc_gecos);

            char userLine[userLineLen];
            strcpy(userLine, "USER ");
            strcat(userLine, irc_user);
            strcat(userLine, " * * :");
            strcat(userLine, irc_gecos);
            strcat(userLine, "\r\n");

            irc_conn->write(irc_conn, userLine, userLineLen);

            blox_join("#OpenBlox");

            doneReg = 1;
        }

        if(ret > 0){
            if(bb_isVerbose){
                fwrite(inBuffer, sizeof(char), realEnd, stdout);
                puts("");
            }

            if(strncmp(inBuffer, "PING :", 6) == 0){
                blox_pong(&inBuffer[6]);
            }
        }
    }

    stillConnected = 0;

    return EXIT_SUCCESS;
}
