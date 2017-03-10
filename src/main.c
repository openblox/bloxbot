/*
 * Copyright (C) 2016-2017 John M. Harris, Jr. <johnmh@openblox.org>
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
#include <pthread.h>
#include <unistd.h>

#include <glib.h>

#include <sys/wait.h>

#include <gnutls/gnutls.h>

static unsigned char stillConnected = 1;
unsigned char bb_verifyTLS = 1;
unsigned int bb_isVerbose = 1;
unsigned char bb_useClientCert = 1;
bloxbot_Conn* irc_conn = NULL;
long int irc_last_msg = 0;

char* _bbinPath = NULL;
int _bboutPort = 14425;

char* irc_user = NULL;
char* irc_nick = NULL;
char* irc_gecos = NULL;

char* ns_user = NULL;
char* ns_pass = NULL;

char* join_str = NULL;
int join_strl = 0;

unsigned char doneReg = 0;
unsigned char capStage = 0;

char* bbm_server;
int bbm_port;
unsigned char bbm_useTLS;

pthread_t queueThread;

void* queueThreadFnc(void* ud){
	while(stillConnected){
		_bb_run_queue();
		usleep(250 * 1000);//250 milliseconds * 100 to convert to microseconds
	}

	return ud;//This means NULL
}

int handleLine(char* inBuffer, int lineLen){
	char* firstR = strchr(inBuffer, '\r');
	if(firstR){
		firstR[0] = '\0';
	}
	lineLen = (int)(firstR - inBuffer);
	
    //SASL EXTERNAL
    if(bb_isVerbose){
        fwrite(inBuffer, sizeof(char), lineLen, stdout);
        putchar('\n');
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

    char* srcNick = NULL;
    char* srcLogin = NULL;
    char* srcHost = NULL;

    char* exclaP = strchr(inBuffer, '!');
    char* atSignP = strchr(inBuffer, '@');
    char* firstSPCP = strchr(inBuffer, ' ');
    int excla = (int)(exclaP - inBuffer);
    if(!exclaP){
        excla = 0;
    }
    int atSign = (int)(atSignP - inBuffer);
    if(!atSignP){
        atSign = 0;
    }
    int firstSPC = (int)(firstSPCP - inBuffer);
    if(!firstSPCP){
        firstSPC = 0;
    }
    if(inBuffer[0] == ':'){
        if(excla == 0 || firstSPC < excla){
            int srcNickLen = firstSPC - 1;
            srcNick = malloc(firstSPC);
            memcpy(srcNick, &inBuffer[1], srcNickLen);
            srcNick[srcNickLen] = '\0';

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

                char* startMsg = strchr(&strCode_[1], ':');
                if(startMsg){
                    startMsg = &startMsg[1];

					if(ircCode == 266){
						if(!bb_useClientCert && ns_user && ns_pass){
							char identLine[10 + strlen(ns_user) + strlen(ns_pass)];
							strcpy(identLine, "IDENTIFY ");
							strcat(identLine, ns_user);
							strcat(identLine, " ");
							strcat(identLine, ns_pass);
							blox_sendMsg("NickServ", identLine);
						}
						
						if(join_str){
							blox_sendDirectly(join_str);
							
							free(join_str);
							join_str = NULL;
						}
					}

					if(ircCode == 433){
						puts(startMsg);
						blox_sendDirectly("QUIT :Error detected\r\n");
						return 1;
					}

                    int ret = _bb_hook(_BB_HOOK_SERVERCODE, srcNick, ircCode, startMsg);
                    if(ret == BB_RET_STOP){
                        free(srcNick);
                        return 0;
                    }
                }
			}
		}else{
			if(excla > 0 && atSign > 0 && excla < atSign){
				int srcNickLen = excla - 1;
				srcNick = malloc(excla);
				memcpy(srcNick, &inBuffer[1], srcNickLen);
				srcNick[srcNickLen] = '\0';
				
				int srcLoginLen = atSign - excla - 1;
				srcLogin = malloc(srcLoginLen + 1);
				memcpy(srcLogin, &inBuffer[excla + 1], srcLoginLen);
				srcLogin[srcLoginLen] = '\0';
				
				int srcHostLen = firstSPC - atSign - 1;
				srcHost = malloc(srcHostLen + 1);
				memcpy(srcHost, &inBuffer[atSign + 1], srcHostLen);
				srcHost[srcHostLen] = '\0';
				
				char* secondSPCP = strchr(&firstSPCP[1], ' ');
				if(secondSPCP){
					int ircCommandLen = (int)(secondSPCP - firstSPCP) - 1;
					char* ircCommand = malloc(ircCommandLen + 1);
					memcpy(ircCommand, &firstSPCP[1], ircCommandLen);
					ircCommand[ircCommandLen] = '\0';
				    
					char* thirdSPCP = strchr(&secondSPCP[1], ' ');
					if(thirdSPCP){
						int targetLen = (int)(thirdSPCP - secondSPCP) - 1;
						char* target = malloc(targetLen + 1);
						memcpy(target, &secondSPCP[1], targetLen);
						target[targetLen] = '\0';
						
						if(strcmp(ircCommand, "PRIVMSG") == 0){
							//TODO: CTCP
							char* theMsg = strchr(&secondSPCP[1], ':');
							if(theMsg){
								theMsg = &theMsg[1];
								
								//Temporary "commands"
								if((strcmp(srcHost, "openblox/dev/JohnMH") == 0) ||
								   (strcmp(srcHost, "bloxbot/testing") == 0)){
									if(strcmp(theMsg, "!reload") == 0){
										bb_reloadPlugins();
									}
									if(strcmp(theMsg, "!quit") == 0){
										stillConnected = 0;
										blox_sendDirectly("QUIT :Shutting down.\r\n");
										return 1;
									}
								}
								
								if(target[0] == '#'){
									int ret = _bb_hook(_BB_HOOK_MSG, target, srcNick, srcLogin, srcHost, theMsg);
									if(ret == BB_RET_STOP){
										free(target);
										free(ircCommand);
										free(srcNick);
										free(srcLogin);
										free(srcHost);
										return 0;
									}
								}else{
									int ret = _bb_hook(_BB_HOOK_PRIVMSG, srcNick, srcLogin, srcHost, theMsg);
									if(ret == BB_RET_STOP){
										free(target);
										free(ircCommand);
										free(srcNick);
										free(srcLogin);
										free(srcHost);
										return 0;
									}
								}
							}
						}
						
						free(target);
					}
					free(ircCommand);
				}
            }

            if(srcNick){
                free(srcNick);
            }

            if(srcLogin){
                free(srcLogin);
            }

            if(srcHost){
                free(srcHost);
            }
        }
    }
    return 0;
}

void cleanup(){
	free(irc_user);
	free(irc_nick);
	free(irc_gecos);
		
	if(ns_pass){
		free(ns_pass);
		if(ns_user){
			free(ns_user);
		}
	}
}

void main_ircbot(){
	_bb_init_bb();
    _bb_init_internal();
    _bb_plugin_init();

    bb_loadPlugin("ob");

    //Actually do the connection
    bloxbot_open_fnc connFunc = NULL;

    if(bbm_useTLS){
        connFunc = bloxbot_conn_tls;
    }else{
        connFunc = bloxbot_conn_plain;
    }

    irc_conn = connFunc(bbm_server, bbm_port);

    if(!irc_conn){
	    cleanup();
		
        _exit(EXIT_FAILURE);
    }

    char inBufferl[MAX_BUFFER_LEN+1];

	int ret = pthread_create(&queueThread, NULL, queueThreadFnc, NULL);
	if(ret){
	    cleanup();
		
		puts("Error: Failed to create queue thread");
		_exit(EXIT_FAILURE);
	}

    ret = 1;
    while(ret != -1 && stillConnected){
        //bzero(inBufferl, MAX_BUFFER_LEN);
        ret = irc_conn->read(irc_conn, inBufferl, MAX_BUFFER_LEN);

        if(ret < 0){
		    cleanup();
			
            _exit(EXIT_FAILURE);
            break;//Obviously this isn't necessary
        }

        if(ret == 0){
            stillConnected = 0;
            break;
        }

        if(handleLine(inBufferl, ret)){
		    cleanup();
			
			if(stillConnected){
			    _exit(EXIT_FAILURE);
				return;
			}else{
			    _exit(EXIT_SUCCESS);
				return;
			}
        }
    }

    stillConnected = 0;

	cleanup();
    _exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]){
    //TODO: Configuration
    bbm_server = strdup("irc.openblox.org");
    bbm_port = 6697;
    bbm_useTLS = 1;

	unsigned char bb_oneshot = 0;

	_bbinPath = strdup("/public/bb-in");

    static struct option long_opts[] = {
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"no-tls-verification", no_argument, 0, 'N'},
        {"verbose", no_argument, 0, 'V'},
		{"oneshot", no_argument, 0, 'O'},
		{"user", required_argument, 0, 'u'},
		{"nick", required_argument, 0, 'n'},
		{"gecos", required_argument, 0, 'g'},
		{"ns-user", required_argument, 0, 'U'},
		{"ns-pass", required_argument, 0, 'P'},
		{"ns-pass-env", no_argument, 0, 0},
		{"join", required_argument, 0, 0},
		{"ob-in-path", required_argument, 0, 0},
		{"ob-out-port", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    int opt_idx = 0;

    int c;
    while(1){
        c = getopt_long(argc, argv, "vhNVOu:n:g:U:P:", long_opts, &opt_idx);

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
				puts("   -n, --nick                  Sets the nickname to be used");
				puts("   -u, --user                  Sets the username to be used");
				puts("   -g, --gecos                 Sets the gecos field to be used");
				puts("");
				puts("Authentication:");
				puts("   -U, --ns-user               Sets the nickserv user to identify as");
				puts("   -P, --ns-pass               Sets the nickserv password use");
				puts("   --ns-pass-env               Sets the nickserv pass to BB_PASSWD");
				puts("");
				puts("Misc:");
				puts("   --join                      Adds a channel to the join list.");
				puts("   -O, --oneshot               Only fork once.");
				puts("");
				puts("   -N, --no-tls-verification   Disables TLS server verification");
				puts("");
				puts("OB Module:");
				puts("   --ob-in-path                Sets the path to use as unix socket");
				puts("   --ob-out-port               Sets port to use for o");
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
			case 'O': {
				bb_oneshot = !bb_oneshot;
				break;
			}
			case 'n': {
				if(irc_nick){
					free(irc_nick);
				}
				irc_nick = strdup(optarg);
				break;
			}
			case 'u': {
				if(irc_user){
					free(irc_user);
				}
				irc_user = strdup(optarg);
				break;
			}
			case 'g' : {
				if(irc_gecos){
					free(irc_gecos);
				}
				irc_gecos = strdup(optarg);
				break;
			}
			case 'U': {
				if(ns_user){
					free(ns_user);
				}
				ns_user = strdup(optarg);
				break;
			}
			case 'P': {
			    if(ns_pass){
					free(ns_pass);
				}
				ns_pass = strdup(optarg);
				break;
			}
			case 0: {
				if(long_opts[opt_idx].flag != 0){
					break;
				}
				//ns-pass-en
				if(strcmp(long_opts[opt_idx].name, "ns-pass-env") == 0){
					if(ns_pass){
						free(ns_pass);
					}
					char* env_pass = getenv("BB_PASSWD");
					if(!env_pass){
						if(irc_nick){
							free(irc_nick);
						}
						if(irc_user){
							free(irc_user);
						}
						if(irc_gecos){
							free(irc_gecos);
						}
						if(ns_user){
							free(ns_user);
						}

						puts("The environmental variable BB_PASSWD was NULL.");
						exit(EXIT_FAILURE);
					}
					ns_pass = strdup(env_pass);
					break;
				}
				if(strcmp(long_opts[opt_idx].name, "join") == 0){
					int tjoinsl = 8 + strlen(optarg);
				    char tjoins[tjoinsl];
					strcpy(tjoins, "JOIN ");
					strcat(tjoins, optarg);
					strcat(tjoins, "\r\n");

					if(!join_str){
						join_str = malloc(tjoinsl);
						strcpy(join_str, tjoins);
					}else{
						join_str = realloc(join_str, join_strl + tjoinsl);
						strcat(join_str + join_strl, tjoins);
					}
					
					join_strl = join_strl + tjoinsl;
					break;
				}
				if(strcmp(long_opts[opt_idx].name, "ob-in-path") == 0){
				    free(_bbinPath);
					_bbinPath = strdup(optarg);
					break;
				}
				if(strcmp(long_opts[opt_idx].name, "ob-out-port") == 0){
					_bboutPort = atoi(optarg);
					break;
				}
				break;
			}
            case '?': {
				if(irc_nick){
					free(irc_nick);
				}
				if(irc_user){
					free(irc_user);
				}
				if(irc_gecos){
					free(irc_gecos);
				}
				if(ns_user){
					free(ns_user);
				}
				if(ns_pass){
					free(ns_pass);
				}
                exit(EXIT_FAILURE);
                break;
            }
            default: {
				if(irc_nick){
					free(irc_nick);
				}
				if(irc_user){
					free(irc_user);
				}
				if(irc_gecos){
					free(irc_gecos);
				}
				if(ns_user){
					free(ns_user);
				}
				if(ns_pass){
					free(ns_pass);
				}
                exit(EXIT_FAILURE);
            }
        }
    }

    //TODO: Read config from configuration file

	if(irc_nick && !irc_user){
		irc_user = strdup(irc_nick);
	}else if(irc_user && !irc_nick){
		irc_nick = strdup(irc_user);
	}
	
    if(!irc_user){
        irc_user = strdup("bloxbot");
    }
    if(!irc_nick){
        irc_nick = strdup("bloxbot");
    }
    if(!irc_gecos){
        irc_gecos = strdup("OpenBlox Utility Bot v" PACKAGE_VERSION);
    }

	if(ns_user && !ns_pass){
		free(ns_user);
		ns_user = NULL;
	}

	if(ns_pass){
		if(!ns_user){
			ns_user = strdup(irc_nick);
		}
		bb_useClientCert = 0;
	}

    //Init
    gnutls_global_init();

	while(1){
		pid_t cpid = fork();

		if(cpid == 0){
			main_ircbot();

			//Should theoretically never get here, but to be safe..
			_exit(EXIT_SUCCESS);
		}else{
			int status = 0;
			while(1){
				pid_t wpid = wait(&status);

				if(wpid == cpid){
					printf("Child died. Exit code: %d\n", WEXITSTATUS(status));
					if(bb_oneshot){
						puts("Shutting down.");
						return EXIT_SUCCESS;
					}
					puts("The cycle continues!");
					sleep(1);
					break;
				}
			}
		}
	}
	return EXIT_SUCCESS;
}
