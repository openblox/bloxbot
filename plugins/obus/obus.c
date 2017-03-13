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

#include <plugin.h>
#include <internal.h>
#include <conf.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>

#include <zmq.h>
#include <json.h>

struct bb_plugin_ob_ud{
	void* zmq_ctx;

	char* pubServ;
	unsigned char keepRunning;

	pthread_t obusThread;
};

//obus defines the max length of a message as 1024
#define OBUS_MAX_MESSAGE_LEN 1024

//Copy-pasted from obus

struct json_object* obus_parseMessage(char* str, int len){
	struct json_tokener* tok = NULL;

	tok = json_tokener_new();
	if(!tok){
		return NULL;
	}
	
	struct json_object* jobj = NULL;
	enum json_tokener_error jerr;

	jobj = json_tokener_parse_ex(tok, str, len);
	jerr = json_tokener_get_error(tok);

	if(jerr != json_tokener_success){
		fprintf(stderr, "JSON parse error: %s\n", json_tokener_error_desc(jerr));
		if(jobj){
			json_object_put(jobj);
		}
		json_tokener_free(tok);
		return NULL;
	}

	json_tokener_free(tok);

	return jobj;
}

void* obusThreadFnc(void* vud){
	struct bb_plugin_ob_ud* plug_ud = (struct bb_plugin_ob_ud*)vud;

	void* zmq_ctx = plug_ud->zmq_ctx;

	void* zmq_sub = zmq_socket(zmq_ctx, ZMQ_SUB);

	int r = zmq_connect(zmq_sub, plug_ud->pubServ);
	if(r != 0){
		zmq_close(zmq_sub);
		puts("[obus] ERROR: Failed to connect to message bus.");
		return NULL;
	}

	r = zmq_setsockopt(zmq_sub, ZMQ_SUBSCRIBE, "bloxbot:", 8);
	if(r != 0){
		zmq_close(zmq_sub);
		puts("[obus] ERROR: Failed to subscribe.");
		return NULL;
	}

	int recv_timeout = 250;//milliseconds

	r = zmq_setsockopt(zmq_sub, ZMQ_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
	if(r != 0){
		zmq_close(zmq_sub);
		puts("[obus] ERROR: Failed to set recv timeout.");
		return NULL;
	}

	char buffer[OBUS_MAX_MESSAGE_LEN];

	while(plug_ud->keepRunning){
		r = zmq_recv(zmq_sub, buffer, OBUS_MAX_MESSAGE_LEN, 0);

		if(r < 0){
			if(errno == EAGAIN){
				continue;
			}
			if(errno == ENOTSUP || errno == ETERM || errno == ENOTSOCK){
				zmq_close(zmq_sub);
				puts("Failed to receive message.");
				return NULL;
			}else{
				if(errno == EFSM){
					puts("EFSM");
				}
			}
		}else if(r > 8){
			puts(&buffer[8]);

			json_object* jobj = obus_parseMessage(&buffer[8], r - 8);
			if(json_object_is_type(jobj, json_type_object)){
				json_object* typeObj;
				if(json_object_object_get_ex(jobj, "command", &typeObj)){
					if(!json_object_is_type(typeObj, json_type_string)){
						json_object_put(typeObj);
					}
				}else{
					json_object_put(typeObj);
				}

				if(typeObj){
					if(strcmp(json_object_get_string(typeObj), "send") == 0){
						json_object* chanObj;
						if(json_object_object_get_ex(jobj, "target", &chanObj)){
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
		}
	}

	zmq_close(zmq_sub);

	return NULL;//Makes compilers stop complaining
}

int bloxbot_plugin_obus_init(bloxbot_Plugin* plug){
	struct bb_plugin_ob_ud* plug_ud = malloc(sizeof(struct bb_plugin_ob_ud));

	if(!plug_ud){
		puts("Out of memory");
		exit(EXIT_FAILURE);
	}
    memset(plug_ud, '\0', sizeof(struct bb_plugin_ob_ud));

	plug->ud = plug_ud;

	plug_ud->zmq_ctx = zmq_ctx_new();
	plug_ud->keepRunning = 1;
	plug_ud->pubServ = NULL;

	char* obus_host = strdup("0.0.0.0");
	int obus_port = 14453;

	{
		bloxbot_ConfigEntry* ent = bb_getConfigEntry("obus-host");
		if(ent){
			if(ent->type == BLOXBOT_CONF_ENT_TYPE_STR){
				if(ent->data.str.len > 0){
					free(obus_host);
				    obus_host = strdup(ent->data.str.str);
				}
			}
			bb_releaseConfigEntry(ent);
			ent = NULL;
		}

		ent = bb_getConfigEntry("obus-port");
		if(ent){
			if(ent->type == BLOXBOT_CONF_ENT_TYPE_INT){
			    obus_port = ent->data.integer;
			}
			bb_releaseConfigEntry(ent);
			ent = NULL;
		}
	}

	//18 is tcp:// + : + 10 for port (lots of buffer, as max port is 5 digits) + 1 '\0'
	int zmq_host_str_maxlen = 18 + strlen(obus_host);
	char* zmq_host_str = malloc(zmq_host_str_maxlen);
	snprintf(zmq_host_str, zmq_host_str_maxlen-1, "tcp://%s:%i", obus_host, obus_port);

	free(obus_host);

	plug_ud->pubServ = zmq_host_str;

	int ret = pthread_create(&plug_ud->obusThread, NULL, obusThreadFnc, plug_ud);
	if(ret){
		puts("[obus] ERROR: Failed to create obus thread");
	    return 1;
	}

	return 0;
}

void bloxbot_plugin_obus_deinit(bloxbot_Plugin* plug){
	struct bb_plugin_ob_ud* plug_ud = (struct bb_plugin_ob_ud*)plug->ud;
	if(plug_ud){
		plug_ud->keepRunning = 0;

		pthread_join(plug_ud->obusThread, NULL);

		free(plug_ud->pubServ);
		free(plug_ud);
	}
}
