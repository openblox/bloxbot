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

#include "conn_tls.h"

#include "internal.h"

#include <stdio.h>
#include <string.h>

#include <sys/socket.h>

#include <gnutls/gnutls.h>

struct _conn_tls_ud{
    char* hostname;
    gnutls_session_t session;
    int sockfd;
    unsigned char verifyTLS;
};

static int _verify_certificate_callback(gnutls_session_t session){
    struct _conn_tls_ud* mode_ud = (struct _conn_tls_ud*)gnutls_session_get_ptr(session);

    if(!mode_ud->verifyTLS){
        return 0;
    }

    unsigned int stat;

    int ret = gnutls_certificate_verify_peers3(session, mode_ud->hostname, &stat);

    if(ret < 0 || stat != 0){
        puts("Failed to verify certificate.");
        return GNUTLS_E_CERTIFICATE_ERROR;
    }else{
        return 0;
    }
}

size_t _conn_tls_read(bloxbot_Conn* conn, char* buf, size_t count){
    if(!buf){
        return -1;
    }

    struct _conn_tls_ud* mode_ud = (struct _conn_tls_ud*)conn->ud;

    int ret = gnutls_record_recv(mode_ud->session, buf, count);

    if(ret == 0){
        return BB_EOF;
    }

    if(ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN){
        return _conn_tls_read(conn, buf, count);
    }

    if(ret < 0 && gnutls_error_is_fatal(ret) == 0){
        fprintf(stderr, "Warning: %s\n", gnutls_strerror(ret));
    }else if(ret < 0){
        fprintf(stderr, "Error: %s\n", gnutls_strerror(ret));
        return -1;
    }

    return ret;
}

size_t _conn_tls_write(bloxbot_Conn* conn, char* buf, size_t count){
    struct _conn_tls_ud* mode_ud = (struct _conn_tls_ud*)conn->ud;

    int ret = -1;

    ret = gnutls_record_send(mode_ud->session, buf, count);

    if(ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN){
        return _conn_tls_write(conn, buf, count);
    }

    if(ret < 0 && gnutls_error_is_fatal(ret) == 0){
        fprintf(stderr, "Warning: %s\n", gnutls_strerror(ret));
    }else if(ret < 0){
        fprintf(stderr, "Error: %s\n", gnutls_strerror(ret));
        return -1;
    }

    return ret;
}

void _conn_tls_close(bloxbot_Conn* conn){
    struct _conn_tls_ud* mode_ud = (struct _conn_tls_ud*)conn->ud;
    gnutls_bye(mode_ud->session, GNUTLS_SHUT_RDWR);
    gnutls_deinit(mode_ud->session);
}

bloxbot_Conn* bloxbot_conn_tls(char* addr, int port){
    struct _conn_tls_ud* mode_ud = malloc(sizeof(struct _conn_tls_ud));
    if(!mode_ud){
        return NULL;
    }
    bzero(mode_ud, sizeof(struct _conn_tls_ud));
    mode_ud->hostname = addr;
    mode_ud->verifyTLS = bb_verifyTLS;

    gnutls_init(&(mode_ud->session), GNUTLS_CLIENT);

    gnutls_certificate_credentials_t xcred;
    gnutls_certificate_allocate_credentials(&xcred);

    gnutls_certificate_set_x509_system_trust(xcred);
    gnutls_certificate_set_verify_function(xcred, _verify_certificate_callback);

    gnutls_session_set_ptr(mode_ud->session, mode_ud);

    int ret = gnutls_priority_set_direct(mode_ud->session, "NORMAL", NULL);
    if(ret < 0){
        if(ret == GNUTLS_E_INVALID_REQUEST){
            puts("Invalid Request");
        }
        free(mode_ud);
        return NULL;
    }

    gnutls_credentials_set(mode_ud->session, GNUTLS_CRD_CERTIFICATE, xcred);

    gnutls_server_name_set(mode_ud->session, GNUTLS_NAME_DNS, mode_ud->hostname, strlen(mode_ud->hostname));

    mode_ud->sockfd = net_util_connect(addr, port);

    if(mode_ud->sockfd == -1){
        free(mode_ud);
        return NULL;
    }

    gnutls_transport_set_int(mode_ud->session, mode_ud->sockfd);
    gnutls_handshake_set_timeout(mode_ud->session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);

    do{
        ret = gnutls_handshake(mode_ud->session);
    }while(ret < 0 && gnutls_error_is_fatal(ret) == 0);

    if(ret < 0){
        fputs("TLS handshake failed\n", stderr);
        gnutls_perror(ret);
        return NULL;
    }

    //Get everything ready to return
    bloxbot_Conn* conn = malloc(sizeof(bloxbot_Conn));
    if(!conn){
        return NULL;
    }
    bzero(conn, sizeof(bloxbot_Conn));

    conn->read = _conn_tls_read;
    conn->write = _conn_tls_write;
    conn->close = _conn_tls_close;

    conn->ud = mode_ud;

    return conn;
}
