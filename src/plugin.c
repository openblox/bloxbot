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

#include "plugin.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <dlfcn.h>

#include <glib.h>

GHashTable* pluginTable = NULL;
GHashTable* cmdTable = NULL;

struct _bb_Cmd{
	bloxbot_Plugin* plug;
	char* cmdName;
	bloxbot_plugin_cmd_fnc cmdFnc;
};

static void _bb_destroy_plugin(void* vdPlug){
	bloxbot_Plugin* plug = (bloxbot_Plugin*)vdPlug;
	dlclose(plug->_handle);
	free(plug);
}

static void _bb_destroy_cmd(void* vdCmd){
	bloxbot_Command* cmd = (bloxbot_Command*)vdCmd;
	if(cmd->isAlias){
		free(cmd->cmdName);
	}else{
		free(cmd->cmdName);
		free(cmd->helpString);
	}
	free(cmd);
}

int _bb_cmd_reload(bloxbot_Plugin* plugin, char* target, char* srcNick, char* srcLogin, char* srcHost, unsigned char isPublic, char* cmd, char* argString){
	if(blox_isAdmin(srcNick, srcLogin, srcHost)){
		blox_msgToUser(target, srcNick, isPublic, "Reloading...");
		
	    bb_reloadPlugins();
		
		blox_msgToUser(target, srcNick, isPublic, "Reload complete.");
	}else{
		blox_msgToUser(target, srcNick, isPublic, "Permission denied.");
	}
	return 0;
}

void _bb_plugin_init(){
	pluginTable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, _bb_destroy_plugin);
	cmdTable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, _bb_destroy_cmd);

	bb_addCommand(NULL, "reload", _bb_cmd_reload, "Reloads plugins");
}

void bb_unloadPlugin(char* name){
	if(!name){
		return;
	}
	if(!pluginTable){
		return;
	}
	bloxbot_Plugin* oplug = (bloxbot_Plugin*)g_hash_table_lookup(pluginTable, name);
	if(oplug){
		g_hash_table_remove(pluginTable, name);
		oplug->deinit(oplug);
		dlclose(oplug->_handle);
		free(oplug);
	}
}

bloxbot_Plugin* bb_loadPlugin(char* name){
	if(!name){
		return NULL;
	}

	bloxbot_Plugin* oplug = (bloxbot_Plugin*)g_hash_table_lookup(pluginTable, name);
	if(oplug){
		printf("'%s' is already loaded.\n", name);
		return oplug;
	}

	int nameLen = strlen(name);
	char libName[19 + (nameLen*2)];
	strcpy(libName, "plugins/");
	strcat(libName, name);
	strcat(libName, "/.libs/");
	strcat(libName, name);
	strcat(libName, ".so");

	char plug_sym_name[_BB_LONGEST_SYM_LEN + 17 + nameLen];
	strcpy(plug_sym_name, "bloxbot_plugin_");
	strcat(plug_sym_name, name);
	plug_sym_name[15 + nameLen] = '_';

	dlerror();//Clear any existing errors

	void* handle = dlopen(libName, RTLD_NOW);
	if(!handle){
		fprintf(stderr, "Error loading plugin '%s': %s\n", name, dlerror());
		return NULL;
	}

	dlerror();//Clear

	bloxbot_plugin_init_fnc initFnc = NULL;

	strcpy(&plug_sym_name[15 + nameLen + 1], "init");
	initFnc = (bloxbot_plugin_init_fnc)dlsym(handle, plug_sym_name);
	if(!initFnc){
		fprintf(stderr, "Error loading plugin '%s': Could not find initialization function symbol\n", name);

		dlclose(handle);
		return NULL;
	}

	bloxbot_plugin_deinit_fnc deinitFnc = NULL;

	strcpy(&plug_sym_name[15 + nameLen + 1], "deinit");
	deinitFnc = (bloxbot_plugin_deinit_fnc)dlsym(handle, plug_sym_name);
	if(!deinitFnc){
		fprintf(stderr, "Error loading plugin '%s': Could not find deinitialization function symbol\n", name);

		dlclose(handle);
		return NULL;
	}

	bloxbot_Plugin* plug = malloc(sizeof(bloxbot_Plugin));
	if(!plug){
		puts("Out of memory.");
		dlclose(handle);
		exit(EXIT_FAILURE);
		return NULL;
	}

	plug->_handle = handle;
	plug->ud = NULL;
	plug->init = initFnc;
	plug->deinit = deinitFnc;

	strcpy(&plug_sym_name[15 + nameLen + 1], "on_msg");
	plug->on_msg = dlsym(handle, plug_sym_name);

	strcpy(&plug_sym_name[15 + nameLen + 1], "on_privmsg");
	plug->on_privmsg = dlsym(handle, plug_sym_name);

	strcpy(&plug_sym_name[15 + nameLen + 1], "on_servercode");
	plug->on_servercode = dlsym(handle, plug_sym_name);

	g_hash_table_insert(pluginTable, strdup(name), plug);

	int r = plug->init(plug);
	if(r != 0){
		plug->deinit(plug);
		dlclose(plug->_handle);
		free(plug);
		return NULL;
	}

	return plug;
}

void bb_reloadPlugins(){
	puts("Reloading plugins..");

	GList* pluginList = NULL;

	GHashTableIter iter;
	void* vdName;
	void* vdPlug;

	g_hash_table_iter_init(&iter, pluginTable);
	while(g_hash_table_iter_next(&iter, &vdName, &vdPlug)){
		if(!vdName){
			break;
		}
		char* plugName = (char*)vdName;
		if(strcmp(plugName, "ob") != 0){//Don't reload "ob" plugin! It has sockets that shouldn't be interrupted.
			pluginList = g_list_prepend(pluginList, strdup(plugName));
		}
	}

	GList* iterator = NULL;
	for(iterator = pluginList; iterator; iterator = iterator->next){
		bb_unloadPlugin((char*)iterator->data);
	}

	iterator = NULL;
	for(iterator = pluginList; iterator; iterator = iterator->next){
		bb_loadPlugin((char*)iterator->data);
		free(iterator->data);
	}

	g_list_free(pluginList);

	puts("Reload complete");
}

int bb_addCommand(bloxbot_Plugin* plug, char* cmdName, bloxbot_plugin_cmd_fnc cmdFnc, char* helpString){
    bloxbot_Command* cmd = malloc(sizeof(bloxbot_Command));
	if(!cmd){
		puts("Out of memory.");
		exit(EXIT_FAILURE);
		return 1;
	}

	cmd->isAlias = 0;
	cmd->cmdName = strdup(cmdName);
	cmd->helpString = strdup(helpString);
	cmd->cmdFnc = cmdFnc;
	cmd->plugin = plug;
	
	g_hash_table_insert(cmdTable, strdup(cmdName), cmd);
	return 0;
}

int bb_addAlias(char* aliasName, char* cmdName){
	bloxbot_Command* cmd = malloc(sizeof(bloxbot_Command));
	if(!cmd){
		puts("Out of memory.");
		exit(EXIT_FAILURE);
		return 1;
	}

	cmd->isAlias = 1;
	cmd->cmdName = strdup(cmdName);
	cmd->helpString = NULL;
	cmd->cmdFnc = NULL;
	
	g_hash_table_insert(cmdTable, strdup(aliasName), cmd);
	return 0;
}

int bb_removeCommand(char* cmdName){
	g_hash_table_remove(cmdTable, cmdName);
	return 0;
}

bloxbot_Command* bb_getCommandByName(char* cmdName){
	GHashTableIter iter;
	
	void* vdCmd = g_hash_table_lookup(cmdTable, cmdName);
	if(vdCmd){
		bloxbot_Command* tCmd = (bloxbot_Command*)vdCmd;

	    if(tCmd->isAlias){
			return bb_getCommandByName(tCmd->cmdName);
		}else{
			return tCmd;
		}
	}

	return NULL;
}

#define _BB_ARG_PROC_STATE_NORM 0
#define _BB_ARG_PROC_STATE_QUOTE_DOUBLE 1
#define _BB_ARG_PROC_STATE_QUOTE_SINGLE 2

void bb_freeArgs(char** args, int numArgs){
	int i;
	for(i = 0; i < numArgs; i++){
		free(args[i]);
	}
	free(args);
}

int bb_processArgs(char* argString, char*** argsTo){
	char** toRet = NULL;
	int numArgs = 0;
	
	int lenArg = strlen(argString);
	
	char* thisArg = malloc(lenArg);
	int thisArgI = 0;

    int state = _BB_ARG_PROC_STATE_NORM;
	unsigned char nextEscaped = 0;

	int i;
	for(i = 0; i < lenArg; i++){
		if(state == _BB_ARG_PROC_STATE_NORM){
			if(nextEscaped){
				nextEscaped = 0;
			}
			
			if(argString[i] == '\'' && thisArgI == 0){
				state = _BB_ARG_PROC_STATE_QUOTE_SINGLE;
				continue;
			}else if(argString[i] == '"' && thisArgI == 0){
				state = _BB_ARG_PROC_STATE_QUOTE_DOUBLE;
				continue;
			}else{
				if(argString[i] != ' '){
					thisArg[thisArgI] = argString[i];
					thisArgI++;
					continue;
				}else{
					if(thisArgI > 0){
						char** newToRet = realloc(toRet, sizeof(char*) * (numArgs + 1));
						if(!newToRet){
							int b;
							for(b = 0; b < numArgs; b++){
								free(toRet[b]);
							}
							free(toRet);
							free(thisArg);
							return -1;
						}
						
						toRet = newToRet;
						toRet[numArgs] = strndup(thisArg, thisArgI);
						numArgs++;
						thisArgI = 0;
					}
				}
			}
		}else if(state == _BB_ARG_PROC_STATE_QUOTE_DOUBLE){
			if(argString[i] == '\\'){
				if(nextEscaped){
					thisArg[thisArgI] = argString[i];
					thisArgI++;
					nextEscaped = 0;
				}else{
					nextEscaped = 1;
				}
				continue;
			}else if(argString[i] == '"'){
				if(nextEscaped){
					thisArg[thisArgI] = argString[i];
					thisArgI++;
					nextEscaped = 0;
				}else{
					char** newToRet = realloc(toRet, sizeof(char*) * (numArgs + 1));
					if(!newToRet){
						int b;
						for(b = 0; b < numArgs; b++){
							free(toRet[b]);
						}
						free(toRet);
						free(thisArg);
						return -1;
					}
						
					toRet = newToRet;
					toRet[numArgs] = strndup(thisArg, thisArgI);
					numArgs++;
					thisArgI = 0;
					state = _BB_ARG_PROC_STATE_NORM;
				}
				continue;
			}else{
				if(nextEscaped){
					nextEscaped = 0;
				}
				thisArg[thisArgI] = argString[i];
				thisArgI++;
			}
		}else if(state == _BB_ARG_PROC_STATE_QUOTE_SINGLE){
			if(argString[i] == '\\'){
				if(nextEscaped){
					thisArg[thisArgI] = argString[i];
					thisArgI++;
					nextEscaped = 0;
				}else{
					nextEscaped = 1;
				}
				continue;
			}else if(argString[i] == '\''){
				if(nextEscaped){
					thisArg[thisArgI] = argString[i];
					thisArgI++;
					nextEscaped = 0;
				}else{
					char** newToRet = realloc(toRet, sizeof(char*) * (numArgs + 1));
					if(!newToRet){
						int b;
						for(b = 0; b < numArgs; b++){
							free(toRet[b]);
						}
						free(toRet);
						free(thisArg);
						return -1;
					}
						
					toRet = newToRet;
					toRet[numArgs] = strndup(thisArg, thisArgI);
					numArgs++;
					thisArgI = 0;
					state = _BB_ARG_PROC_STATE_NORM;
				}
				continue;
			}else{
				if(nextEscaped){
					nextEscaped = 0;
				}
				thisArg[thisArgI] = argString[i];
				thisArgI++;
			}
		}
	}

	if(thisArgI > 0){
		char** newToRet = realloc(toRet, sizeof(char*) * (numArgs + 1));
		if(!newToRet){
			int b;
			for(b = 0; b < numArgs; b++){
				free(toRet[b]);
			}
			free(toRet);
			free(thisArg);
			return -1;
		}
						
		toRet = newToRet;
		toRet[numArgs] = strndup(thisArg, thisArgI);
		numArgs++;

		free(thisArg);
	}

	*argsTo = toRet;
	return numArgs;
}

int bb_processCommand(char* target, char* srcNick, char* srcLogin, char* srcHost, char* msg){
	unsigned char isPublic = 0;
	
	int curIndex = 0;
	int i;
	int msgLen = strlen(msg);
	for(i = 0; i < msgLen; i++){
		if(msg[i] == '!'){
			isPublic = !isPublic;
		}else{
			curIndex = i;
			break;
		}
	}

	if(!target){
		isPublic = 0;
	}

	int cmdLen = 0;
	for(; i < msgLen + 1; i++){
		if(msg[i] == ' ' || msg[i] == '\n' || msg[i] == '\r' || msg[i] == '\0'){
			cmdLen = i - curIndex;
			break;
		}
	}

	if(cmdLen > 0){
		char* cmd = malloc(cmdLen + 1);
		if(!cmd){
			puts("Out of memory.");
			exit(EXIT_FAILURE);
			return 1;
		}
		strncpy(cmd, &msg[curIndex], cmdLen);
		cmd[cmdLen] = '\0';

		bloxbot_Command* theCmd = bb_getCommandByName(cmd);

		if(theCmd){
			return theCmd->cmdFnc(theCmd->plugin, target, srcNick, srcLogin, srcHost, isPublic, cmd, &msg[curIndex + cmdLen + 1]);
		}
	}

	return 0;
}

int _bb_hook(int hook_id, ...){
	if(!pluginTable){
		return BB_RET_ERR;
	}

    int toRet = BB_RET_OK;

	va_list argp;
	va_start(argp, hook_id);

	GHashTableIter iter;
	void* vdName;
	void* vdPlug;

	g_hash_table_iter_init(&iter, pluginTable);
	while(g_hash_table_iter_next(&iter, &vdName, &vdPlug)){
		if(!vdPlug){
			break;
		}

		va_list argpp;
		va_copy(argpp, argp);

		bloxbot_Plugin* plug = (bloxbot_Plugin*)vdPlug;

		unsigned char breakOut = 0;

		switch(hook_id){
			case _BB_HOOK_MSG: {
				if(plug->on_msg){
					int ret = plug->on_msg(plug, va_arg(argpp, char*), va_arg(argpp, char*), va_arg(argpp, char*), va_arg(argpp, char*), va_arg(argpp, char*));
					if(ret != BB_RET_OK){
						toRet = ret;
						breakOut = 1;
					}
				}
				break;
			}
			case _BB_HOOK_PRIVMSG: {
				if(plug->on_privmsg){
					int ret = plug->on_privmsg(plug, va_arg(argpp, char*), va_arg(argpp, char*), va_arg(argpp, char*), va_arg(argpp, char*));
					if(ret != BB_RET_OK){
						toRet = ret;
						breakOut = 1;
					}
				}
				break;
			}
			case _BB_HOOK_SERVERCODE: {
				if(plug->on_servercode){
					int ret = plug->on_servercode(plug, va_arg(argpp, char*), va_arg(argpp, int), va_arg(argpp, char*));
					if(ret != BB_RET_OK){
						toRet = ret;
						breakOut = 1;
					}
				}
				break;
			}
		}

		va_end(argpp);

		if(breakOut){
			break;
		}
	}

	va_end(argp);

	return toRet;
}
