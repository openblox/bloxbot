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

#include "plugin.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <getopt.h>

#include <gnutls/gnutls.h>

static unsigned char stillConnected = 1;
unsigned char bb_verifyTLS = 1;
unsigned int bb_isVerbose = 1;
unsigned char bb_useClientCert = 1;
bloxbot_Conn* irc_conn = NULL;
long int irc_last_msg = 0;

char* irc_user = NULL;
char* irc_nick = NULL;
char* irc_gecos = NULL;

unsigned char doneReg = 0;
unsigned char doneInit = 0;
unsigned char capStage = 0;
unsigned char readyForInit = 0;

int handleLine(char* inBuffer, int lineLen){
    //SASL EXTERNAL
    if(bb_isVerbose){
        fwrite(inBuffer, sizeof(char), lineLen, stdout);
        puts("");
    }

    if(bb_useClientCert && capStage != 3 && (capStage == 2 || inBuffer[0] == ':')){
        if(capStage == 0){
            char* bufAfterServ = strchr(inBuffer, ' ');
            if(bufAfterServ && strlen(bufAfterServ) > 4){
                if(strncmp(bufAfterServ, " CAP ", 5) == 0){
                    bufAfterServ = &bufAfterServ[5];

                    bufAfterServ = strchr(bufAfterServ, ' ');
                    if(bufAfterServ){
                        bufAfterServ = &bufAfterServ[1];
                        if(strncmp(bufAfterServ, "LS :", 4) == 0){
                            if(strstr(bufAfterServ, "sasl")){
                                blox_sendDirectlyl("CAP REQ :sasl\r\n", 15);
                                capStage = 1;
                            }
                        }
                    }
                }
            }
        }else if(capStage == 1){
            char* bufAfterServ = strchr(inBuffer, ' ');
            if(bufAfterServ && strlen(bufAfterServ) > 4){
                if(strncmp(bufAfterServ, " CAP ", 5) == 0){
                    if(strstr(bufAfterServ, "ACK :sasl")){
                        blox_sendDirectlyl("AUTHENTICATE EXTERNAL\r\n", 23);
                        capStage = 2;
                    }
                }
            }
        }else if(capStage == 2){
            if(strstr(inBuffer, "AUTHENTICATE +")){
                blox_sendDirectlyl("AUTHENTICATE +\r\n", 16);
                blox_sendDirectlyl("CAP END\r\n", 9);
                capStage = 3;
            }
        }
    }

    if(!doneReg && capStage < 1){
        if(bb_useClientCert){
            blox_sendDirectlyl("CAP LS\r\n", 8);
        }

        int nickLineLen = 7 + strlen(irc_nick);
        char nickLine[nickLineLen];
        strcpy(nickLine, "NICK ");
        strcat(nickLine, irc_nick);
        strcat(nickLine, "\r\n");

        blox_sendDirectlyl(nickLine, nickLineLen);

        int userLineLen = 13 + strlen(irc_user) + strlen(irc_gecos);

        char userLine[userLineLen];
        strcpy(userLine, "USER ");
        strcat(userLine, irc_user);
        strcat(userLine, " * * :");
        strcat(userLine, irc_gecos);
        strcat(userLine, "\r\n");

        blox_sendDirectlyl(userLine, userLineLen);

        doneReg = 1;
    }

    if(strncmp(inBuffer, "PING :", 6) == 0){
        blox_pong(&inBuffer[6]);
    }

    if(inBuffer[0] == ':'){
        char* excla = strchr(inBuffer, '!');
        char* atSign = strchr(inBuffer, '@');
        if(excla > 0){

        }

        char* strCode_ = strchr(inBuffer, ' ');
        if(strlen(strCode_) > 3){
            char* strCode = malloc(4);
            if(!strCode){
                return 1;
            }
            memcpy(strCode, &strCode_[1], 3);
            strCode[3] = '\0';

            int ircCode = strtol(strCode, NULL, 0);

            free(strCode);

            if(strncmp(&strCode_[1], "NOTICE", 6) == 0){
                char* noticeFrom = &strCode_[8];
                char* noticeFromEnd = strchr(noticeFrom, ' ');
                int noticeFromLen = (int)(noticeFromEnd - noticeFrom);

                char* noticeMsg = &noticeFromEnd[2];

                if(memcmp(noticeFrom, "Auth", 4) == 0){
                    if(strstr(noticeMsg, "Welcome to")){
                        if(!doneInit){
                            //TODO: Init hooks
                            blox_join("#OpenBlox");
                            doneInit = 1;
                        }
                    }
                }

                return 0;
            }

            if(ircCode == 900){
            }
            printf("Code: %i\n", ircCode);
        }
    }

    return 0;
}

int main(int argc, char* argv[]){
    //TODO: Configuration
    char* server = "irc.openblox.org";
    int port = 6697;
    unsigned char useTLS = 1;

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

    _bb_init_bb();
    _bb_init_internal();

    bb_loadPlugin("ob");

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

    char inBufferl[MAX_BUFFER_LEN+1];

    int ret = 1;
    while(ret != -1 && stillConnected){
        _bb_run_queue();
        bzero(inBufferl, MAX_BUFFER_LEN);
        ret = irc_conn->read(irc_conn, inBufferl, MAX_BUFFER_LEN);

        if(ret < 0){
            exit(EXIT_FAILURE);
            break;//Obviously this isn't necessary
        }

        if(ret == 0){
            stillConnected = 0;
            break;
        }

        if(handleLine(inBufferl, ret)){
            return EXIT_FAILURE;
        }
    }

    stillConnected = 0;

    return EXIT_SUCCESS;
}
