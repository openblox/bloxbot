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

#include <dlfcn.h>

#include <glib.h>

static GHashTable* pluginTable = NULL;

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

    char entryFncName[22 + nameLen];
    strcpy(entryFncName, "bloxbot_plugin_");
    strcat(entryFncName, name);
    strcat(entryFncName, "_entry");

    dlerror();//Clear any existing errors

    void* handle = dlopen(libName, RTLD_LAZY);
    if(!handle){
        fprintf(stderr, "Error loading plugin '%s': %s\n", name, dlerror());
        return NULL;
    }

    dlerror();//Clear

    bloxbot_plugin_entry_fnc entryFnc = NULL;

    entryFnc = (bloxbot_plugin_entry_fnc)dlsym(handle, entryFncName);
    if(!entryFnc){
        fprintf(stderr, "Error loading plugin '%s': Could not find entry symbol\n", name);

        dlclose(handle);
        return NULL;
    }

    bloxbot_Plugin* plug = entryFnc();
    if(!plug){
        //Plugin itself should print more verbose error messages
        fprintf(stderr, "Error loading plugin '%s'\n", name);
        dlclose(handle);
        return NULL;
    }

    plug->_handle = handle;

    g_hash_table_insert(pluginTable, strdup(name), plug);

    return plug;
}
