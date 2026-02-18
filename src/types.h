/**
 * types.h
 * craftos-base
 * 
 * This file declares internal types for the emulator.
 * 
 * This file is licensed under the MIT license.
 * Copyright (c) 2018-2026 JackMacWindows.
 */

#ifndef CRAFTOS_TYPES_H
#define CRAFTOS_TYPES_H
#include <craftos.h>
#include "config.h"

#if HAVE_SYS_STAT_H
#define __USE_XOPEN2K8 1
#include <sys/stat.h>
#elif !defined(S_ISDIR)
#define S_ISDIR(mode) (((mode) & 0xF000) == 0x4000)
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#elif !defined(W_OK)
#define W_OK 2
#endif

#define MOUNT_FLAG_RO 1
#define MOUNT_FLAG_MMFS 3

struct craftos_mount_list {
    struct craftos_mount_list * next;
    char ** mount_path;
    union {
        char * filesystem_path;
        const struct mmfs_dir * root_dir;
    };
    unsigned char flags;
};

struct craftos_peripheral_list {
    struct craftos_peripheral_list * next;
    const char * side;
    const char ** types;
    void * userp;
    const struct luaL_Reg * funcs;
};

struct craftos_alarm_list {
    struct craftos_alarm_list * next;
    int id;
};

struct craftos_sw_timer_list {
    struct craftos_sw_timer_list * next;
    int id;
    double deadline;
    unsigned char isAlarm;
};

struct craftos_api_list {
    struct craftos_api_list * next;
    const char * name;
    const struct luaL_Reg * funcs;
};

/** The global OS function table. */
extern struct craftos_func F;

/**
 * Returns the currently running machine instance.
 * @param L The Lua state to get from
 * @return The machine for the state
 */
extern craftos_machine_t get_comp(struct lua_State *L);

#endif