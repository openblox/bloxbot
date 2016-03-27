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

static void _bb_destroy_plugin(void* vdPlug){
    bloxbot_Plugin* plug = (bloxbot_Plugin*)vdPlug;
    dlclose(plug->_handle);
    free(plug);
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

    if(!pluginTable){
        pluginTable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, _bb_destroy_plugin);
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

    char* plug_symn = &plug_sym_name[15 + nameLen + 1];

    dlerror();//Clear any existing errors

    void* handle = dlopen(libName, RTLD_NOW);
    if(!handle){
        fprintf(stderr, "Error loading plugin '%s': %s\n", name, dlerror());
        return NULL;
    }

    dlerror();//Clear

    bloxbot_plugin_init_fnc initFnc = NULL;

    strcpy(plug_symn, "init");
    initFnc = (bloxbot_plugin_init_fnc)dlsym(handle, plug_sym_name);
    if(!initFnc){
        fprintf(stderr, "Error loading plugin '%s': Could not find initialization function symbol\n", name);

        dlclose(handle);
        return NULL;
    }

    bloxbot_plugin_deinit_fnc deinitFnc = NULL;

    strcpy(plug_symn, "deinit");
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

    strcpy(plug_symn, "on_msg");
    plug->on_msg = dlsym(handle, plug_sym_name);

    strcpy(plug_symn, "on_privmsg");
    plug->on_msg = dlsym(handle, plug_sym_name);

    g_hash_table_insert(pluginTable, strdup(name), plug);

    return plug;
}

struct _bb_hook_info{
    va_list argp;
    int hook_id;
};

static void _bb_for_each_hook(void* vdName, void* vdPlug, void* ud){
    if(!vdPlug){
        return;
    }
    if(!ud){
        return;
    }

    bloxbot_Plugin* plug = (bloxbot_Plugin*)vdPlug;
    struct _bb_hook_info* hookinfo = (struct _bb_hook_info*)ud;

    switch(hookinfo->hook_id){
        case _BB_HOOK_INIT: {
            if(plug->init){
                plug->init(plug);
            }
            break;
        }
        case _BB_HOOK_DEINIT: {
            if(plug->deinit){
                plug->deinit(plug);
            }
            break;
        }
        case _BB_HOOK_MSG: {
            if(plug->on_msg){
                plug->on_msg(plug, va_arg(hookinfo->argp, char*), va_arg(hookinfo->argp, char*), va_arg(hookinfo->argp, char*), va_arg(hookinfo->argp, char*), va_arg(hookinfo->argp, char*));
            }
            break;
        }
        case _BB_HOOK_PRIVMSG: {
            if(plug->on_privmsg){
                plug->on_privmsg(plug, va_arg(hookinfo->argp, char*), va_arg(hookinfo->argp, char*), va_arg(hookinfo->argp, char*), va_arg(hookinfo->argp, char*));
            }
            break;
        }
    }
}

void _bb_hook(int hook_id, ...){
    if(!pluginTable){
        return;
    }

    struct _bb_hook_info* hookinfo = malloc(sizeof(struct _bb_hook_info));
    if(!hookinfo){
        puts("Out of memory");
        exit(EXIT_FAILURE);
        return;//Not necessary, but stops some stupid compilers from giving warnings
    }
    hookinfo->hook_id = hook_id;

    va_start(hookinfo->argp, hook_id);

    g_hash_table_foreach(pluginTable, _bb_for_each_hook, hookinfo);

    free(hookinfo);

    va_end(hookinfo->argp);
}
