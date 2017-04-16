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

int bb_cmd_piracy(bloxbot_Plugin* plugin, char* target, char* srcNick, char* srcLogin, char* srcHost, unsigned char isPublic, char* cmd, char* argString){
    blox_msgToUser(target, srcNick, isPublic, "https://upload.wikimedia.org/wikipedia/commons/d/d2/British_sailors_boarding_an_Algerine_pirate_ship.jpg");
	return 0;
}

int bloxbot_plugin_piracy_init(bloxbot_Plugin* plug){
	bb_addCommand(plug, "piracy", bb_cmd_piracy, "Shows a useful graphic depicting piracy.");

	return 0;
}

void bloxbot_plugin_piracy_deinit(){
	bb_removeCommand("piracy");
}
