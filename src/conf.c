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

#include "conf.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <glib.h>

GHashTable* confTable = NULL;
char* confFile = NULL;

bloxbot_ConfigEntry* _bb_conf_ent_new(){
    bloxbot_ConfigEntry* ent = malloc(sizeof(bloxbot_ConfigEntry));
    if(!ent){
        return NULL;
    }

    ent->type = 0;
    ent->refs = 0;

    return ent;
}

static void _bb_destroy_conf(void* vdEnt){
    if(!vdEnt){
        return;
    }

    bloxbot_ConfigEntry* ent = (bloxbot_ConfigEntry*)vdEnt;

    if(ent->refs < 2){
        switch(ent->type){
            case BLOXBOT_CONF_ENT_TYPE_STR: {
                free(ent->data.str.str);
                break;
            }
            case BLOXBOT_CONF_ENT_TYPE_ARRAY: {
                bloxbot_ConfigEntry** arry = ent->data.array.array;

                int i;
                for(i = 0; i < ent->data.array.len; i++){
                    if(arry[i]){
                        _bb_destroy_conf(arry[i]);
                    }
                }
            }
        }

        free(ent);
    }
}

unsigned char bb_loadConfig(char* name){
    if(confFile){
        // Config already loaded, but we don't see that as an error
        return 0;
    }

    confTable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, _bb_destroy_conf);

    return bb_reloadConfig(name);
}

#define _BB_CONF_PARSE_NORM 0
#define _BB_CONF_PARSE_TYPE 1
#define _BB_CONF_PARSE_COMMENT 2

unsigned char bb_reloadConfig(char* newConfig){
    if(confFile){
        if(newConfig){
            if(strcmp(confFile, newConfig) != 0){
                free(confFile);
                confFile = strdup(newConfig);
            }
        }
    }else{
        if(!newConfig){
            return 1;
        }
        confFile = strdup(newConfig);
    }

    FILE* f = fopen(confFile, "r");
    if(!f){
        free(confFile);
        confFile = NULL;
        return 1;
    }

    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    int curLineNum = 0;

    unsigned char curState = _BB_CONF_PARSE_NORM;
    char* curKey = NULL;
    bloxbot_ConfigEntry* curEnt = NULL;

    while((read = getline(&line, &len, f)) != -1){
        curLineNum++;

        if(curState == _BB_CONF_PARSE_NORM){
            if(read >= 2){//2 chars
                if(line[0] == '/'){
                    if(line[1] == '/'){
                        continue;
                    }else if(line[1] == '*'){
                        unsigned char gotEndStart = 0;
                        unsigned char endedComment = 0;

                        int i;
                        for(i = 1; i < read; i++){
                            if(gotEndStart){
                                if(line[i] == '/'){
                                    endedComment = 1;
                                    break;
                                }
                            }else{
                                if(line[i] == '*'){
                                    gotEndStart = 1;
                                }
                            }
                        }

                        if(!endedComment){
                            curState = _BB_CONF_PARSE_COMMENT;
                        }
                    }
                }else if(line[0] == 'i'){
                    if(curKey){
                        free(curKey);
                    }

                    if(curEnt){
                        _bb_destroy_conf(curEnt);
                    }

                    curEnt = _bb_conf_ent_new();
                    if(!curEnt){
                        free(line);
                        return 1;
                    }

                    curEnt->type = BLOXBOT_CONF_ENT_TYPE_INT;
                    curEnt->data.integer = -1;

                    curKey = strndup(&line[2], read - 3);

                    curState = _BB_CONF_PARSE_TYPE;
                }else if(line[0] == 's'){
                    if(curKey){
                        free(curKey);
                    }

                    if(curEnt){
                        _bb_destroy_conf(curEnt);
                    }

                    curEnt = _bb_conf_ent_new();
                    if(!curEnt){
                        free(line);
                        return 1;
                    }

                    curEnt->type = BLOXBOT_CONF_ENT_TYPE_STR;
                    curEnt->data.str.str = NULL;
                    curEnt->data.str.len = 0;

                    curKey = strndup(&line[2], read - 3);

                    curState = _BB_CONF_PARSE_TYPE;
                }else if(line[0] == 'a'){
                    if(curKey){
                        free(curKey);
                    }

                    if(curEnt){
                        _bb_destroy_conf(curEnt);
                    }

                    curEnt = _bb_conf_ent_new();
                    if(!curEnt){
                        free(line);
                        return 1;
                    }

                    curEnt->type = BLOXBOT_CONF_ENT_TYPE_ARRAY;
                    curEnt->data.array.array = NULL;
                    curEnt->data.array.len = 0;

                    curKey = strndup(&line[2], read - 3);

                    curState = _BB_CONF_PARSE_TYPE;
                }
            }
        }else if(curState == _BB_CONF_PARSE_COMMENT){
            if(line[0] == '*'){
                if(line[1] == '/'){
                    curState = _BB_CONF_PARSE_NORM;
                }
            }
        }else if(curState == _BB_CONF_PARSE_TYPE){
            if(read >= 2){
                if(curEnt->type == BLOXBOT_CONF_ENT_TYPE_INT){
                    curEnt->data.integer = atoi(line);

                    goto updateConfKey;
                }else if(curEnt->type == BLOXBOT_CONF_ENT_TYPE_STR){
                    if(curEnt->data.str.len == 0){
                        curEnt->data.str.len = (read - 1);
                        curEnt->data.str.str = strndup(line, read - 1);
                        if(!curEnt->data.str.str){
                            _bb_destroy_conf(curEnt);
                            free(curKey);
                            free(line);
                            return 1;
                        }
                    }else{
                        curEnt->data.str.len += read;

                        char* tmpStr = realloc(curEnt->data.str.str, curEnt->data.str.len + 1);
                        if(!tmpStr){
                            _bb_destroy_conf(curEnt);
                            free(curKey);
                            free(line);
                            return 1;
                        }
                        curEnt->data.str.str = tmpStr;

                        strcat(tmpStr, "\n");
                        strncat(tmpStr, line, read - 1);
                    }
                }else if(curEnt->type == BLOXBOT_CONF_ENT_TYPE_ARRAY){
                    curEnt->data.array.len = curEnt->data.array.len + 1;

                    struct bloxbot_ConfigEntry** tmpArray = realloc(curEnt->data.array.array, sizeof(struct bloxbot_ConfigEntry*) * curEnt->data.array.len);
                    if(!tmpArray){
                        _bb_destroy_conf(curEnt);
                        free(curKey);
                        free(line);
                        return 1;
                    }
                    curEnt->data.array.array = tmpArray;

                    struct bloxbot_ConfigEntry* newEnt = _bb_conf_ent_new();
                    if(!newEnt){
                        _bb_destroy_conf(curEnt);
                        free(curKey);
                        free(line);
                        return 1;
                    }

                    newEnt->type = BLOXBOT_CONF_ENT_TYPE_STR;
                    newEnt->data.str.len = 0;
                    newEnt->data.str.str = NULL;

                    if(line[1] == ':' && line[0] == 'a'){
                        printf("ERROR: Invalid array on line %i\n", curLineNum);
                        _bb_destroy_conf(curEnt);
                        _bb_destroy_conf(newEnt);
                        free(curKey);
                        free(line);
                        return 2;
                    }

                    if(line[1] == ':' && line[0] == 'i'){
                        newEnt->type = BLOXBOT_CONF_ENT_TYPE_INT;
                        newEnt->data.integer = atoi(line);
                    }else{
                        if(line[1] == ':' && line[0] == 's'){
                            newEnt->data.str.len = (read - 3);
                            newEnt->data.str.str = strndup(&line[2], read - 3);
                        }else{
                            newEnt->data.str.len = (read - 1);
                            newEnt->data.str.str = strndup(line, read - 1);
                        }

                        if(!curEnt->data.str.str){
                            _bb_destroy_conf(curEnt);
                            _bb_destroy_conf(newEnt);
                            free(curKey);
                            free(line);
                            return 1;
                        }
                    }

                    tmpArray[curEnt->data.array.len - 1] = newEnt;
                }
            }else{
              updateConfKey:
                curEnt->refs = 1;

                g_hash_table_insert(confTable, strdup(curKey), curEnt);

                free(curKey);
                curKey = NULL;

                curEnt = NULL;

                curState = _BB_CONF_PARSE_NORM;
            }
        }
    }

    if(curEnt && curKey){
        curEnt->refs = 1;

        g_hash_table_insert(confTable, strdup(curKey), curEnt);

        free(curKey);
    }

    return 0;
}

unsigned char bb_configLoaded(){
    return (confTable != NULL) && (confFile != NULL);
}

unsigned char bb_hasConfigEntry(char* name){
    if(bb_configLoaded()){
        return g_hash_table_contains(confTable, name);
    }
    return 1;
}

bloxbot_ConfigEntry* bb_getConfigEntry(char* name){
    if(confTable){
        bloxbot_ConfigEntry* ent = (bloxbot_ConfigEntry*)g_hash_table_lookup(confTable, name);
        if(ent){
            ent->refs++;
        }
        return ent;
    }
    return NULL;
}

void bb_releaseConfigEntry(bloxbot_ConfigEntry* ent){
    if(ent){
        ent->refs--;
        if(ent->refs < 1){
            _bb_destroy_conf(ent);
        }
    }
}
