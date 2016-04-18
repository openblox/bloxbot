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

#include <plugin.h>
#include <internal.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>

#include <glib.h>

#include <json.h>

//Default handling for SIGPIPE is to exit. I don't know why. Sounds silly to me.
//There's probably some historic stuff to it.
void catch_sigpipe_do_nothing(int signum){}

struct bb_plugin_ob_ud{
	int incoming_fd;
	int outgoing_fd;

	pthread_t serverThread;
	pthread_t incomingThread;

	GList* outgoingConns;
};

void bloxbot_plugin_ob_broadcast(struct bb_plugin_ob_ud* plug_ud, char* msg, int len){
	GList* toRemove = NULL;

	GList* iterator = NULL;
	for(iterator = plug_ud->outgoingConns; iterator; iterator = iterator->next){
		int ret = send(*(int*)iterator->data, msg, len, 0);
		if(ret == -1){
			if(errno == EACCES || errno == EBADF || errno == ECONNRESET || errno == ENOTCONN || errno == EPIPE){
				close(*(int*)iterator->data);
				toRemove = g_list_prepend(toRemove, iterator->data);
			}
		}
	}

	iterator = NULL;
	for(iterator = toRemove; iterator; iterator = iterator->next){
	    plug_ud->outgoingConns = g_list_remove(plug_ud->outgoingConns, iterator->data);
		free(iterator->data);
	}
	g_list_free(toRemove);
}

size_t _readFullLine(int sfd, char* buf, int count){
    if(!buf){
        return -1;
    }

    int numRead = 0;
    for(int i = 0; i < count; i++){
        int ret = recv(sfd, &buf[i], 1, 0);

        if(ret == 0){
            return 0;
        }

        if(ret == -1){
            //TODO: Handle errors
			return 0;//Make it look like it's getting EOF'd for now
        }

        numRead++;
        if(buf[i] == '\n'){
            buf[i] = '\0';
            break;
        }
    }

    return numRead;
}

void* incomingThreadFnc(void* vud){
	struct bb_plugin_ob_ud* plug_ud = (struct bb_plugin_ob_ud*)vud;

	struct sockaddr_un addr;

	int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sfd < 0){
		puts("Error creating incoming socket");
		exit(EXIT_FAILURE);
	}

	bzero(&addr, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, _bbinPath);

	unlink(_bbinPath);

	if(bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
		puts("Failed to bind incoming socket");
		exit(EXIT_FAILURE);
	}

	chmod(_bbinPath, 0777);

	if(listen(sfd, 5) == -1){
		puts("Listen error");
		exit(EXIT_FAILURE);
	}
	int clilen = sizeof(addr);

	while(1){
		int newsfd = accept(sfd, (struct sockaddr*)&addr, &clilen);

		if(newsfd > -1){
			int mBufLen = 1024;
			char incomingBuf[mBufLen];

			while(1){
				size_t rt = _readFullLine(newsfd, incomingBuf, mBufLen);
				if(rt == 0){
					break;
				}

				json_object* jobj = json_tokener_parse(incomingBuf);
				if(json_object_is_type(jobj, json_type_object)){
					json_object* typeObj;
					if(json_object_object_get_ex(jobj, "type", &typeObj)){
						if(!json_object_is_type(typeObj, json_type_string)){
							json_object_put(typeObj);
						}
					}else{
						json_object_put(typeObj);
					}

					if(typeObj){
						if(strcmp(json_object_get_string(typeObj), "irc") == 0){
							json_object* chanObj;
							if(json_object_object_get_ex(jobj, "channel", &chanObj)){
								if(!json_object_is_type(chanObj, json_type_string)){
									json_object_put(chanObj);
									chanObj = NULL;
								}
							}else{
								json_object_put(chanObj);
								chanObj = NULL;
							}

							if(chanObj){
								json_object* msgObj;
								if(json_object_object_get_ex(jobj, "msg", &msgObj)){
									if(!json_object_is_type(chanObj, json_type_string)){
										json_object_put(msgObj);
										msgObj = NULL;
									}
								}else{
									json_object_put(msgObj);
									msgObj = NULL;
								}

								if(msgObj){
								   	blox_sendMsg((char*)json_object_get_string(chanObj), (char*)json_object_get_string(msgObj));
									json_object_put(msgObj);
								}

								json_object_put(chanObj);
							}
						}
					}

					if(typeObj){
						json_object_put(typeObj);
					}
				}
				if(jobj){
					json_object_put(jobj);
				}

				incomingBuf[rt] = '\n';
				bloxbot_plugin_ob_broadcast(plug_ud, incomingBuf, rt + 1);
			}

			close(newsfd);
		}
	}

	return NULL;//Makes compilers stop complaining
}

void* serverThreadFnc(void* vud){
	struct bb_plugin_ob_ud* plug_ud = (struct bb_plugin_ob_ud*)vud;

	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sfd < 0){
		puts("Error creating outgoing socket");
		exit(EXIT_FAILURE);
	}

	int enable = 1;
	if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
		puts("setsockopt(SO_REUSEADDR) failed");
		exit(EXIT_FAILURE);
	}

	plug_ud->outgoing_fd = sfd;

	struct sockaddr_in serv_addr;
	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(_bboutPort);

	if(bind(sfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		puts("Failed to bind outgoing socket");
		exit(EXIT_FAILURE);
	}

	if(listen(sfd, 5) == -1){
		puts("Listen error");
		exit(EXIT_FAILURE);
	}
	int clilen = sizeof(serv_addr);

	while(1){
		int newsfd = accept(sfd, (struct sockaddr*)&serv_addr, &clilen);

		if(newsfd > -1){
		    int* fdp = malloc(sizeof(int));
			if(!fdp){
				puts("Out of memory");
				exit(EXIT_FAILURE);
			}
			*fdp = newsfd;

			plug_ud->outgoingConns = g_list_prepend(plug_ud->outgoingConns, fdp);
		}
	}

	return NULL;//Makes compilers stop complaining
}

void bloxbot_plugin_ob_init(bloxbot_Plugin* plug){
	signal(SIGPIPE, catch_sigpipe_do_nothing);

	struct bb_plugin_ob_ud* plug_ud = malloc(sizeof(struct bb_plugin_ob_ud));

	if(!plug_ud){
		puts("Out of memory");
		exit(EXIT_FAILURE);
	}
	bzero(plug_ud, sizeof(struct bb_plugin_ob_ud));

	plug->ud = plug_ud;

	plug_ud->outgoingConns = NULL;

	int ret = pthread_create(&plug_ud->incomingThread, NULL, incomingThreadFnc, plug_ud);
	if(ret){
		puts("Error: Failed to create incoming thread");
		exit(EXIT_FAILURE);
	}
	ret = pthread_create(&plug_ud->serverThread, NULL, serverThreadFnc, plug_ud);
	if(ret){
		puts("Error: Failed to create outgoing thread");
		exit(EXIT_FAILURE);
	}
}

void bloxbot_plugin_ob_deinit(){}

int bloxbot_plugin_ob_on_servercode(bloxbot_Plugin* plug, char* src, int code, char* msg){
	return BB_RET_OK;
}
