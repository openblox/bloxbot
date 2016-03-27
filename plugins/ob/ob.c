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

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

void bloxbot_plugin_ob_init(bloxbot_Plugin* plug){
    blox_join("#OpenBlox");
}

void bloxbot_plugin_ob_deinit(){}

bloxbot_Plugin* bloxbot_plugin_ob_entry(){
    bloxbot_Plugin* plug = malloc(sizeof(bloxbot_Plugin));
    if(!plug){
        puts("Out of memory");
        return NULL;
    }
    bzero(plug, sizeof(bloxbot_Plugin));

    plug->init = bloxbot_plugin_ob_init;
    plug->deinit = bloxbot_plugin_ob_deinit;

    return plug;
}
