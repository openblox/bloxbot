/*
 * Copyright (C) 2017 John M. Harris, Jr. <johnmh@openblox.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

int bb_cmd_echo(bloxbot_Plugin* plugin, char* target, char* srcNick, char* srcLogin, char* srcHost, unsigned char isPublic, char* cmd, char* argString){
	char** args;
	int numArgs = bb_processArgs(argString, &args);

	int i;
	for(i = 0; i < numArgs; i++){
		blox_msgToUser(target, srcNick, isPublic, args[i]);
		free(args[i]);
	}
	free(args);
	
	return 0;
}

int bb_cmd_quit(bloxbot_Plugin* plugin, char* target, char* srcNick, char* srcLogin, char* srcHost, unsigned char isPublic, char* cmd, char* argString){
	if(blox_isAdmin(srcNick, srcLogin, srcHost)){		
		return BB_RET_QUIT;
	}else{
		blox_msgToUser(target, srcNick, isPublic, "Permission denied.");
	}
	
	return 0;
}

int bb_cmd_join(bloxbot_Plugin* plugin, char* target, char* srcNick, char* srcLogin, char* srcHost, unsigned char isPublic, char* cmd, char* argString){
	if(blox_isAdmin(srcNick, srcLogin, srcHost)){
		char** args;
		int numArgs = bb_processArgs(argString, &args);

		if(numArgs > 0){
			blox_join(args[0]);
		}else{
			blox_msgToUser(target, srcNick, isPublic, "'join' expects at least one parameter.");
		}

		bb_freeArgs(args, numArgs);
	}else{
		blox_msgToUser(target, srcNick, isPublic, "Permission denied.");
	}
	return 0;
}

int bb_cmd_part(bloxbot_Plugin* plugin, char* target, char* srcNick, char* srcLogin, char* srcHost, unsigned char isPublic, char* cmd, char* argString){
	if(blox_isAdmin(srcNick, srcLogin, srcHost)){
		char** args;
		int numArgs = bb_processArgs(argString, &args);

		if(numArgs > 0){
			if(numArgs > 1){
				blox_partr(args[0], args[1]);
			}else{
				blox_part(args[0]);
			}
		}else{
			if(!target){
				blox_msgToUser(target, srcNick, isPublic, "'part' expects at least one parameter.");
			}else{
				blox_part(target);
			}
		}
		
		bb_freeArgs(args, numArgs);
	}else{
		blox_msgToUser(target, srcNick, isPublic, "Permission denied.");
	}
	return 0;
}

int bb_cmd_say(bloxbot_Plugin* plugin, char* target, char* srcNick, char* srcLogin, char* srcHost, unsigned char isPublic, char* cmd, char* argString){
	if(blox_isAdmin(srcNick, srcLogin, srcHost)){
		char** args;
		int numArgs = bb_processArgs(argString, &args);

		if(numArgs > 1){
			blox_sendMsg(args[0], args[1]);
		}else{
			blox_msgToUser(target, srcNick, isPublic, "'say' expects at least two parameters.");
		}
		
		bb_freeArgs(args, numArgs);
	}else{
		blox_msgToUser(target, srcNick, isPublic, "Permission denied.");
	}
	return 0;
}

void bloxbot_plugin_cmd_init(bloxbot_Plugin* plug){
	puts("Init called.");
	bb_addCommand(plug, "echo", bb_cmd_echo, "Echos a string passed back to the user");
	bb_addCommand(plug, "quit", bb_cmd_quit, "Makes bloxbot quit");
	bb_addCommand(plug, "join", bb_cmd_join, "Makes bloxbot join a channel");
	bb_addCommand(plug, "part", bb_cmd_part, "Makes bloxbot part a channel");
	bb_addCommand(plug, "say", bb_cmd_say, "Makes bloxbot send a message to a target");
	bb_addAlias("send", "say");
}

void bloxbot_plugin_cmd_deinit(){
	puts("Deinit called.");
	bb_removeCommand("echo");
	bb_removeCommand("quit");
	bb_removeCommand("join");
	bb_removeCommand("part");
}
