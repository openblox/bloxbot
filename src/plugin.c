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

void _bb_plugin_init(){
	pluginTable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, _bb_destroy_plugin);
	cmdTable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
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
		oplug->deinit(oplug);
		g_hash_table_remove(pluginTable, name);
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

	plug->init(plug);

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

int bb_addCommand(bloxbot_Plugin* plug, char* cmdName, bloxbot_plugin_cmd_fnc cmdFnc){
	return 0;
}

int bb_addAlias(char* aliasName, char* cmdName){
	return 0;
}

int bb_removeCommand(char* cmdName){
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
