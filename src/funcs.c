/**
 * funcs.c
 * craftos-base
 * 
 * This file defines functionality related to the OS function table.
 * 
 * This file is licensed under the MIT license.
 * Copyright (c) 2018-2026 JackMacWindows.
 */

#include <craftos.h>
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKE_WRAPPER(ret, name, proto, args) static ret wrap_##name proto {return name args ;}
#define WRAP_CHECK(name) F.name = functions->name != NULL ? functions->name : wrap_##name

struct craftos_func F = {NULL};

MAKE_WRAPPER(FILE*, fopen, (const char * file, const char * mode, craftos_machine_t machine), (file, mode))
MAKE_WRAPPER(int, fclose, (FILE *fp, craftos_machine_t machine), (fp))
MAKE_WRAPPER(size_t, fread, (void * buf, size_t size, size_t count, FILE * fp, craftos_machine_t machine), (buf, size, count, fp))
MAKE_WRAPPER(size_t, fwrite, (const void * buf, size_t size, size_t count, FILE * fp, craftos_machine_t machine), (buf, size, count, fp))
MAKE_WRAPPER(int, fgetc, (FILE * fp, craftos_machine_t machine), (fp))
MAKE_WRAPPER(int, fputc, (int ch, FILE * fp, craftos_machine_t machine), (ch, fp))
MAKE_WRAPPER(long, ftell, (FILE * fp, craftos_machine_t machine), (fp))
MAKE_WRAPPER(int, fseek, (FILE * fp, long offset, int origin, craftos_machine_t machine), (fp, offset, origin))
MAKE_WRAPPER(int, feof, (FILE * fp, craftos_machine_t machine), (fp))
MAKE_WRAPPER(int, ferror, (FILE * fp, craftos_machine_t machine), (fp))
MAKE_WRAPPER(int, remove, (const char * path, craftos_machine_t machine), (path))
MAKE_WRAPPER(int, rename, (const char * from, const char * to, craftos_machine_t machine), (from, to))

#if HAVE_UNISTD_H
#include <unistd.h>
MAKE_WRAPPER(int, mkdir, (const char * path, int mode, craftos_machine_t machine), (path, mode))
#endif

#if HAVE_SYS_STAT_H
#define __USE_XOPEN2K8 1
#include <sys/stat.h>
static int wrap_stat(const char * path, struct craftos_stat * st, craftos_machine_t machine) {
    struct stat st2;
    int retval = stat(path, &st2);
    if (retval != 0) return retval;
    st->st_dev = st2.st_dev;
    st->st_ino = st2.st_ino;
    st->st_mode = st2.st_mode;
    st->st_nlink = st2.st_nlink;
    st->st_uid = st2.st_uid;
    st->st_gid = st2.st_gid;
    st->st_rdev = st2.st_rdev;
    st->st_size = st2.st_size;
    st->st_blksize = st2.st_blksize;
    st->st_blocks = st2.st_blocks;
    st->st_atim.tv_sec = st2.st_atim.tv_sec;
    st->st_atim.tv_nsec = st2.st_atim.tv_nsec;
    st->st_ctim.tv_sec = st2.st_ctim.tv_sec;
    st->st_ctim.tv_nsec = st2.st_ctim.tv_nsec;
    st->st_mtim.tv_sec = st2.st_mtim.tv_sec;
    st->st_mtim.tv_nsec = st2.st_mtim.tv_nsec;
    return 0;
}
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
MAKE_WRAPPER(craftos_DIR *, opendir, (const char * path, craftos_machine_t machine), (path))
MAKE_WRAPPER(int, closedir, (craftos_DIR * dir, craftos_machine_t machine), ((DIR*)dir))
static struct craftos_dirent * wrap_readdir(craftos_DIR * dir, craftos_machine_t machine) {
    static struct craftos_dirent dr;
    struct dirent * d = readdir((DIR *)dir);
    if (d == NULL) return NULL;
    dr.d_ino = d->d_ino;
    dr.d_reclen = d->d_reclen;
    dr.d_type = d->d_type;
#if _DIRENT_HAVE_D_NAMLEN
    dr.d_namlen = d->d_namlen;
#else
    dr.d_namlen = strlen(dr.d_name);
#endif
    strcpy(dr.d_name, d->d_name);
    return &dr;
}
#endif

int craftos_init(const craftos_func_t functions) {
    if (
        functions->timestamp == NULL
        || functions->convertPixelValue == NULL
#if !HAVE_SYS_STAT_H
        || functions->stat == NULL
#endif
#if !HAVE_UNISTD_H
        || functions->mkdir == NULL
#endif
#if !HAVE_DIRENT_H
        || functions->opendir == NULL
        || functions->closedir == NULL
        || functions->readdir == NULL
#endif
    ) return -1;
    F.timestamp = functions->timestamp;
    F.convertPixelValue = functions->convertPixelValue;
    if (functions->startTimer != NULL && functions->cancelTimer != NULL) {
        F.startTimer = functions->startTimer;
        F.cancelTimer = functions->cancelTimer;
    } else {
        F.startTimer = NULL;
        F.cancelTimer = NULL;
    }
    if (functions->setComputerLabel != NULL)
        F.setComputerLabel = functions->setComputerLabel;
    else F.setComputerLabel = NULL;
    if (functions->redstone_getInput != NULL && functions->redstone_setOutput != NULL) {
        F.redstone_getInput = functions->redstone_getInput;
        F.redstone_setOutput = functions->redstone_setOutput;
    } else {
        F.redstone_getInput = NULL;
        F.redstone_setOutput = NULL;
    }
    F.malloc = functions->malloc != NULL ? functions->malloc : malloc;
    F.realloc = functions->realloc != NULL ? functions->realloc : realloc;
    F.free = functions->free != NULL ? functions->free : free;
    if (
        functions->mutex_create != NULL &&
        functions->mutex_destroy != NULL &&
        functions->mutex_lock != NULL &&
        functions->mutex_unlock != NULL
    ) {
        F.mutex_create = functions->mutex_create;
        F.mutex_destroy = functions->mutex_destroy;
        F.mutex_lock = functions->mutex_lock;
        F.mutex_unlock = functions->mutex_unlock;
    } else {
        F.mutex_create = NULL;
        F.mutex_destroy = NULL;
        F.mutex_lock = NULL;
        F.mutex_unlock = NULL;
    }
    WRAP_CHECK(fopen);
    WRAP_CHECK(fclose);
    WRAP_CHECK(fread);
    WRAP_CHECK(fwrite);
    WRAP_CHECK(fgetc);
    WRAP_CHECK(fputc);
    WRAP_CHECK(ftell);
    WRAP_CHECK(fseek);
    WRAP_CHECK(feof);
    WRAP_CHECK(ferror);
    WRAP_CHECK(remove);
    WRAP_CHECK(rename);
#if HAVE_SYS_STAT_H
    WRAP_CHECK(stat);
#endif
#if HAVE_UNISTD_H
    WRAP_CHECK(mkdir);
#endif
#if HAVE_DIRENT_H
    WRAP_CHECK(opendir);
    WRAP_CHECK(closedir);
    WRAP_CHECK(readdir);
#endif
    if (
        functions->http_request != NULL &&
        functions->http_handle_close != NULL &&
        functions->http_handle_read != NULL &&
        functions->http_handle_getc != NULL &&
        functions->http_handle_tell != NULL &&
        functions->http_handle_seek != NULL &&
        functions->http_handle_getResponseCode != NULL &&
        functions->http_handle_getResponseHeader != NULL
    ) {
        F.http_request = functions->http_request;
        F.http_handle_close = functions->http_handle_close;
        F.http_handle_read = functions->http_handle_read;
        F.http_handle_getc = functions->http_handle_getc;
        F.http_handle_tell = functions->http_handle_tell;
        F.http_handle_seek = functions->http_handle_seek;
        F.http_handle_getResponseCode = functions->http_handle_getResponseCode;
        F.http_handle_getResponseHeader = functions->http_handle_getResponseHeader;
        if (
            functions->http_websocket != NULL &&
            functions->http_websocket_send != NULL &&
            functions->http_websocket_close != NULL
        ) {
            F.http_websocket = functions->http_websocket;
            F.http_websocket_send = functions->http_websocket_send;
            F.http_websocket_close = functions->http_websocket_close;
        } else {
            F.http_websocket = NULL;
            F.http_websocket_send = NULL;
            F.http_websocket_close = NULL;
        }
    } else {
        F.http_request = NULL;
        F.http_handle_close = NULL;
        F.http_handle_read = NULL;
        F.http_handle_getc = NULL;
        F.http_handle_tell = NULL;
        F.http_handle_seek = NULL;
        F.http_handle_getResponseCode = NULL;
        F.http_handle_getResponseHeader = NULL;
        F.http_websocket = NULL;
        F.http_websocket_send = NULL;
        F.http_websocket_close = NULL;
    }
    return 0;
}
