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

#ifndef BB_PLUGIN_H_
#define BB_PLUGIN_H_

//We include bloxbot.h here so that plugins don't have to do the runaround
#include "bloxbot.h"

typedef struct bloxbot_Plugin bloxbot_Plugin;

typedef void (*bloxbot_plugin_init_fnc)(bloxbot_Plugin* plugin);
typedef void (*bloxbot_plugin_deinit_fnc)(bloxbot_Plugin* plugin);
typedef void (*bloxbot_plugin_msg_fnc)(bloxbot_Plugin* plugin, char* target, char* srcNick, char* srcLogin, char* srcHost, char* msg);
typedef void (*bloxbot_plugin_privmsg_fnc)(bloxbot_Plugin* plugin, char* srcNick, char* srcLogin, char* srcHost, char* msg);

#define _BB_LONGEST_SYM_LEN 10

typedef struct bloxbot_Plugin{
    void* _handle;//DO NOT USE THIS IN PLUGIN CODE!
    void* ud;
    bloxbot_plugin_init_fnc init;
    bloxbot_plugin_deinit_fnc deinit;
    bloxbot_plugin_msg_fnc on_msg;
    bloxbot_plugin_privmsg_fnc on_privmsg;
} bloxbot_Plugin;

bloxbot_Plugin* bb_loadPlugin(char* name);

#define _BB_HOOK_INIT 1
#define _BB_HOOK_DEINIT 2
#define _BB_HOOK_MSG 3
#define _BB_HOOK_PRIVMSG 4

void _bb_hook(int hook_id, ...);

#endif
