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

//Continue processing other hooks
#define BB_RET_OK 0
#define BB_RET_ERR 1
//Stops processing after this hook returns
#define BB_RET_STOP 2

//The below are only used by command handlers
#define BB_RET_QUIT 6

typedef int (*bloxbot_plugin_msg_fnc)(bloxbot_Plugin* plugin, char* target, char* srcNick, char* srcLogin, char* srcHost, char* msg);
typedef int (*bloxbot_plugin_privmsg_fnc)(bloxbot_Plugin* plugin, char* srcNick, char* srcLogin, char* srcHost, char* msg);
typedef int (*bloxbot_plugin_servercode_fnc)(bloxbot_Plugin* plugin, char* src, int code, char* msg);

typedef int (*bloxbot_plugin_cmd_fnc)(bloxbot_Plugin* plugin, char* target, char* srcNick, char* srcLogin, char* srcHost, unsigned char isPublic, char* cmd, char* argString);

#define _BB_LONGEST_SYM_LEN 13

typedef struct bloxbot_Plugin{
	void* _handle;//DO NOT USE THIS IN PLUGIN CODE!

	//Feel free to set this to whatever you like, it is not used internally. It is here entirely for plugin use.
	void* ud;

	bloxbot_plugin_init_fnc init;
	bloxbot_plugin_deinit_fnc deinit;

	bloxbot_plugin_msg_fnc on_msg;
	bloxbot_plugin_privmsg_fnc on_privmsg;
	bloxbot_plugin_servercode_fnc on_servercode;
} bloxbot_Plugin;

typedef struct bloxbot_Command{
    bloxbot_plugin_cmd_fnc cmdFnc;
	char* helpString;
	char* cmdName;
	unsigned char isAlias;
	bloxbot_Plugin* plugin;
} bloxbot_Command;

void _bb_plugin_init();

void bb_unloadPlugin(char* name);
bloxbot_Plugin* bb_loadPlugin(char* name);
void bb_reloadPlugins();

int bb_addCommand(bloxbot_Plugin* plug, char* cmdName, bloxbot_plugin_cmd_fnc cmdFnc, char* helpString);
int bb_addAlias(char* aliasName, char* cmdName);
int bb_removeCommand(char* cmdName);
bloxbot_Command* bb_getCommandByName(char* cmdName);

void bb_freeArgs(char** args, int numArgs);
int bb_processArgs(char* argString, char*** argsTo);

int bb_processCommand(char* target, char* srcNick, char* srcLogin, char* srcHost, char* msg);

#define _BB_HOOK_MSG 1
#define _BB_HOOK_PRIVMSG 2
#define _BB_HOOK_SERVERCODE 3

int _bb_hook(int hook_id, ...);

#endif
