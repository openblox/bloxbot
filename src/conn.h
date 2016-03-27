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

#ifndef BB_CONN_H_
#define BB_CONN_H_

#include <stdlib.h>
#include <strings.h>

#include "net_util.h"

extern unsigned char bb_verifyTLS;
extern unsigned char bb_useClientCert;

typedef struct bloxbot_Conn bloxbot_Conn;

typedef size_t (*bloxbot_read_fnc)(bloxbot_Conn* conn, char* buf, int count);
typedef size_t (*bloxbot_write_fnc)(bloxbot_Conn* conn, char* buf, size_t count);
typedef void (*bloxbot_close_fnc)(bloxbot_Conn* conn);

typedef struct bloxbot_Conn{
    void* ud;
    bloxbot_read_fnc read;
    bloxbot_write_fnc write;
    bloxbot_close_fnc close;
} bloxbot_Conn;

typedef bloxbot_Conn* (*bloxbot_open_fnc)(char* addr, int port);

#include "conn_plain.h"
#include "conn_tls.h"

#endif
