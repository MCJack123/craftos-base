/**
 * machine.c
 * craftos-base
 * 
 * This file defines the functions related to machine execution.
 * 
 * This file is licensed under the MIT license.
 * Copyright (c) 2018-2026 JackMacWindows.
 */

#include <craftos.h>
#include "string_list.h"
#include "terminal.h"
#include "types.h"
#include <stdarg.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <lstate.h>

extern const luaL_Reg fs_lib[];
extern const luaL_Reg http_lib[];
extern const luaL_Reg http_lib_ws[];
extern const luaL_Reg os_lib[];
extern const luaL_Reg peripheral_lib[];
extern const luaL_Reg rs_lib[];
extern const luaL_Reg term_lib[];

void _lua_lock(lua_State *L) {
    if (G(L)->lockstate == 2 || G(L)->lock == NULL) return;
    F.mutex_lock(G(L)->lock);
    G(L)->lockstate = 1;
}

void _lua_unlock(lua_State *L) {
    if (G(L)->lock == NULL) return;
    if (G(L)->lockstate != 1 && G(L)->lockstate != 3) {
        return;
    }
    G(L)->lockstate--;
    F.mutex_unlock(G(L)->lock);
}

void * _lua_newlock() {
    if (F.mutex_create != NULL) return F.mutex_create();
    return NULL;
}

void _lua_freelock(void * l) {
    if (l != NULL) F.mutex_destroy(l);
}

craftos_machine_t get_comp(lua_State *L) {
    static void* lastG = NULL;
    static craftos_machine_t lastM = NULL;
    if (L->l_G != lastG) {
        lua_getfield(L, LUA_REGISTRYINDEX, "_M");
        lastM = lua_touserdata(L, -1);
        lastG = L->l_G;
        lua_pop(L, 1);
    }
    return lastM;
}

craftos_machine_t craftos_machine_create(const craftos_machine_config_t * config) {
    if (F.timestamp == NULL) return NULL;
    craftos_machine_t machine = F.malloc(sizeof(struct craftos_machine));
    if (machine == NULL) return NULL;
    machine->id = config->id;
    machine->running = CRAFTOS_MACHINE_STATUS_SHUTDOWN;
    machine->L = NULL;
    if (config->size_pixel_scale == 0)
        machine->term = craftos_terminal_create(config->width, config->height);
    else machine->term = craftos_terminal_create(config->width / (6 * config->size_pixel_scale), config->height / (9 * config->size_pixel_scale));
    if (machine->term == NULL) {
        F.free(machine);
        return NULL;
    }
    machine->eventQueue = NULL;
    machine->eventQueueLock = F.mutex_create ? F.mutex_create() : NULL;
    machine->coro = NULL;
    machine->bios = config->bios;
    machine->system_start = 0;
    machine->files_open = 0;
    machine->openWebsockets = 0;
    machine->requests_open = 0;
    machine->pixel_scale = config->size_pixel_scale;
    if (config->label != NULL) {
        machine->label = F.malloc(strlen(config->label)+1);
        strcpy(machine->label, config->label);
    } else machine->label = NULL;
    memset(machine->rs_output, 0, sizeof(machine->rs_output));
    machine->mounts = NULL;
    machine->peripherals = NULL;
    machine->alarms = NULL;
    machine->apis = NULL;

    if (craftos_machine_mount_real(machine, config->base_path, "/", 0) != 0) {
        craftos_machine_destroy(machine);
        return NULL;
    }
    if (config->rom_mmfs != NULL && craftos_machine_mount_mmfs(machine, config->rom_mmfs, "/rom") != 0) {
        craftos_machine_destroy(machine);
        return NULL;
    }
    return machine;
}

void craftos_machine_destroy(craftos_machine_t machine) {
    if (machine->L != NULL) {
        if (machine->eventQueueLock) F.mutex_lock(machine->eventQueueLock);
        lua_close(machine->L);
        machine->L = machine->eventQueue = machine->coro = NULL;
        if (machine->eventQueueLock) F.mutex_unlock(machine->eventQueueLock);
    }
    while (machine->apis) {
        struct craftos_api_list * next = machine->apis->next;
        F.free(machine->apis);
        machine->apis = next;
    }
    /* this code is okay as long as timers extends alarms and timers have no allocated fields */
    while (machine->alarms) {
        struct craftos_alarm_list * next = machine->alarms->next;
        F.free(machine->alarms);
        machine->alarms = next;
    }
    while (machine->peripherals) {
        struct craftos_peripheral_list * next = machine->peripherals->next;
        F.free(machine->peripherals);
        machine->peripherals = next;
    }
    while (machine->mounts) {
        struct craftos_mount_list * next = machine->mounts->next;
        char ** ptr = machine->mounts->mount_path;
        while (*ptr) F.free(*ptr++);
        F.free(machine->mounts->mount_path);
        if (machine->mounts->flags & MOUNT_FLAG_MMFS) {
            /* delete mmfs */
        } else F.free(machine->mounts->filesystem_path);
        F.free(machine->mounts);
        machine->mounts = next;
    }
    if (machine->label) F.free(machine->label);
    if (machine->eventQueueLock) F.mutex_destroy(machine->eventQueueLock);
    craftos_terminal_destroy(machine->term);
    F.free(machine);
}

static int getNextEvent(craftos_machine_t machine, lua_State *L, const char * filter) {
    lua_State *event = NULL;
    do {
        if (event) lua_remove(machine->eventQueue, 1);
        if (lua_gettop(machine->eventQueue) == 0) return -1;
        event = lua_tothread(machine->eventQueue, 1);
    } while (filter != NULL && strcmp(lua_tostring(event, 1), filter) != 0 && strcmp(lua_tostring(event, 1), "terminate") != 0);
    int count = lua_gettop(event);
    lua_xmove(event, L, count);
    lua_remove(machine->eventQueue, 1);
    return count;
}

craftos_status_t craftos_machine_run(craftos_machine_t machine) {
    int status, narg;
    lua_State *coro = machine->coro;
    if (machine->running == CRAFTOS_MACHINE_STATUS_SHUTDOWN) {
        struct craftos_api_list * apis = machine->apis;
        /*
        * All Lua contexts are held in this structure. We work with it almost
        * all the time.
        */
        machine->L = luaL_newstate();
        
        machine->coro = coro = lua_newthread(machine->L);
        machine->eventQueue = lua_newthread(machine->L);
        
        luaL_openlibs(coro);
        lua_getglobal(coro, "os"); lua_getfield(coro, -1, "date");
        lua_newtable(coro); luaL_setfuncs(coro, fs_lib, 0); lua_setglobal(coro, "fs");
        if (F.http_request) {
            if (F.http_websocket) {lua_newtable(coro); luaL_setfuncs(coro, http_lib_ws, 0); lua_setglobal(coro, "http");}
            else {lua_newtable(coro); luaL_setfuncs(coro, http_lib, 0); lua_setglobal(coro, "http");}
        }
        lua_newtable(coro); luaL_setfuncs(coro, os_lib, 0); lua_pushvalue(coro, -2); lua_setfield(coro, -2, "date"); lua_setglobal(coro, "os");
        lua_newtable(coro); luaL_setfuncs(coro, peripheral_lib, 0); lua_setglobal(coro, "peripheral");
        lua_newtable(coro); luaL_setfuncs(coro, rs_lib, 0); lua_pushvalue(coro, -1); lua_setglobal(coro, "rs"); lua_setglobal(coro, "redstone");
        lua_newtable(coro); luaL_setfuncs(coro, term_lib, 0); lua_setglobal(coro, "term");
        lua_pop(coro, 2);
        
        lua_pushliteral(coro, "");
        lua_setglobal(coro, "_CC_DEFAULT_SETTINGS");
        lua_pushliteral(coro, "ComputerCraft " CRAFTOS_CC_VERSION " (CraftOS-Base " CRAFTOS_BASE_VERSION ")");
        lua_setglobal(coro, "_HOST");
        lua_pushnil(coro); lua_setglobal(coro, "package");
        lua_pushnil(coro); lua_setglobal(coro, "require");
        lua_pushnil(coro); lua_setglobal(coro, "io");
        
        while (apis) {
            luaL_newlib(coro, apis->funcs);
            lua_setglobal(coro, apis->name);
            apis = apis->next;
        }

        lua_pushlightuserdata(coro, machine);
        lua_setfield(coro, LUA_REGISTRYINDEX, "_M");
        lua_getglobal(coro, "table");
        lua_getfield(coro, -1, "sort");
        lua_setfield(coro, LUA_REGISTRYINDEX, "tsort");
        lua_pop(coro, 1);
        
        /* Load the file containing the script we are going to run */
        printf("Loading BIOS...\n");
        if (machine->bios) status = luaL_loadbuffer(coro, machine->bios, strlen(machine->bios), "@bios.lua");
        else status = luaL_loadfile(coro, "/rom/bios.lua"); /* TODO: use wrapped file procedures */
        if (status) {
            /* If something went wrong, error message is at the top of */
            /* the stack */
            const char * fullstr = lua_tostring(coro, -1);
            printf("Couldn't load BIOS: %s (%d)\n", fullstr, status);
            lua_close(machine->L);
            machine->L = machine->coro = machine->eventQueue = NULL;
            craftos_terminal_clear(machine->term, -1, 0xFE);
            craftos_terminal_write_literal(machine->term, 0, 0, "Error loading BIOS", 0xFE);
            craftos_terminal_write_string(machine->term, 0, 1, fullstr, 0xFE);
            craftos_terminal_write_literal(machine->term, 0, 2, "ComputerCraft may be installed incorrectly", 0xFE);
            return CRAFTOS_MACHINE_STATUS_ERROR;
        }
        
        /* Ask Lua to run our little script */
        machine->running = CRAFTOS_MACHINE_STATUS_YIELD;
        machine->system_start = F.timestamp();
        printf("Running main coroutine.\n");
        status = lua_resume(coro, NULL, 0);
    } else {
        if (lua_isstring(coro, -1)) narg = getNextEvent(machine, coro, lua_tostring(coro, -1));
        else narg = getNextEvent(machine, coro, NULL);
        if (narg < 0) return CRAFTOS_MACHINE_STATUS_YIELD;
        status = lua_resume(coro, NULL, narg);
    }
    while (status == LUA_YIELD) {
        if (machine->running == CRAFTOS_MACHINE_STATUS_SHUTDOWN || machine->running == CRAFTOS_MACHINE_STATUS_RESTART) {
            lua_close(machine->L);
            machine->L = machine->coro = machine->eventQueue = NULL;
            if (machine->running == CRAFTOS_MACHINE_STATUS_RESTART) {
                machine->running = CRAFTOS_MACHINE_STATUS_SHUTDOWN;
                return CRAFTOS_MACHINE_STATUS_RESTART;
            }
            return machine->running;
        }
        if (lua_isstring(coro, -1)) narg = getNextEvent(machine, coro, lua_tostring(coro, -1));
        else narg = getNextEvent(machine, coro, NULL);
        if (narg < 0) return CRAFTOS_MACHINE_STATUS_YIELD;
        status = lua_resume(coro, NULL, narg);
        /*printf("Yield\n");*/
    }
    if (status != 0) {
        const char * fullstr = lua_tostring(coro, -1);
        printf("Errored: %s\n", fullstr);
        craftos_terminal_clear(machine->term, -1, 0xFE);
        craftos_terminal_write_literal(machine->term, 0, 0, "Error running computer", 0xFE);
        craftos_terminal_write_string(machine->term, 0, 1, fullstr, 0xFE);
        craftos_terminal_write_literal(machine->term, 0, 2, "ComputerCraft may be installed incorrectly", 0xFE);
        lua_close(machine->L);
        machine->L = machine->coro = machine->eventQueue = NULL;
        machine->running = CRAFTOS_MACHINE_STATUS_SHUTDOWN;
        return CRAFTOS_MACHINE_STATUS_ERROR;
    } else {
        printf("Closing session.\n");
        craftos_terminal_clear(machine->term, -1, 0xFE);
        craftos_terminal_write_literal(machine->term, 0, 0, "Error running computer", 0xFE);
        craftos_terminal_write_literal(machine->term, 0, 1, "ComputerCraft may be installed incorrectly", 0xFE);
        lua_close(machine->L);
        machine->L = machine->coro = machine->eventQueue = NULL;
        return CRAFTOS_MACHINE_STATUS_ERROR;
    }
}

int craftos_machine_queue_event(craftos_machine_t machine, const char * event, const char * fmt, ...) {
    va_list va;
    size_t sz;
    void** ptr;
    char c;
    lua_State *ev;

    if (machine->eventQueue == NULL) return -1;
    ev = lua_newthread(machine->eventQueue);
    lua_pushstring(ev, event);
    if (fmt != NULL) {
        va_start(va, fmt);
        while (*fmt) {
            switch (*fmt++) {
                case 'b': lua_pushinteger(ev, va_arg(va, int)); break;
                case 'B': lua_pushinteger(ev, va_arg(va, unsigned int)); break;
                case 'h': lua_pushinteger(ev, va_arg(va, int)); break;
                case 'H': lua_pushinteger(ev, va_arg(va, unsigned int)); break;
                case 'i': lua_pushinteger(ev, va_arg(va, int)); break;
                case 'I': lua_pushinteger(ev, va_arg(va, unsigned int)); break;
                case 'l': lua_pushinteger(ev, va_arg(va, long)); break;
                case 'L': lua_pushinteger(ev, va_arg(va, unsigned long)); break;
                case 'j': lua_pushinteger(ev, va_arg(va, lua_Integer)); break;
                case 'J': lua_pushinteger(ev, va_arg(va, lua_Unsigned)); break;
                case 'T': lua_pushinteger(ev, va_arg(va, size_t)); break;
                case 'f': lua_pushnumber(ev, va_arg(va, double)); break;
                case 'd': lua_pushnumber(ev, va_arg(va, double)); break;
                case 'n': lua_pushnumber(ev, va_arg(va, lua_Number)); break;
                case 'c': c = va_arg(va, int); lua_pushlstring(ev, &c, 1); break;
                case 'z': lua_pushstring(ev, va_arg(va, const char *)); break;
                case 's': sz = va_arg(va, size_t); lua_pushlstring(ev, va_arg(va, const char *), sz); break;
                case 'x': lua_pushnil(ev); break;
                case 'q': lua_pushboolean(ev, va_arg(va, int)); break;
                case 'F': lua_pushcfunction(ev, va_arg(va, lua_CFunction)); break;
                case 'u': lua_pushlightuserdata(ev, va_arg(va, void*)); break;
                case 'P': ptr = lua_newuserdata(ev, sizeof(void*)); *ptr = va_arg(va, void*); break;
                case 'G':
                    lua_createtable(ev, 0, 1);
                    lua_pushcfunction(ev, va_arg(va, lua_CFunction));
                    lua_setfield(ev, -2, "__gc");
                    lua_setmetatable(ev, -2);
                    break;
                case 'r': lua_newtable(ev); luaL_setfuncs(ev, va_arg(va, const luaL_Reg*), 0); break;
                case 'R':
                    lua_newtable(ev);
                    lua_pushvalue(ev, -2);
                    luaL_setfuncs(ev, va_arg(va, const luaL_Reg*), 1);
                    lua_remove(ev, -2);
                    break;
                case 'v': lua_xmove(va_arg(va, lua_State*), ev, 1); break;
            }
        }
    }
    va_end(va);
    return 0;
}

int craftos_machine_add_api(craftos_machine_t machine, const char * name, const struct luaL_Reg * funcs) {
    struct craftos_api_list * api = F.malloc(sizeof(struct craftos_api_list));
    if (api == NULL) return -1;
    api->next = machine->apis;
    api->name = name;
    api->funcs = funcs;
    machine->apis = api;
    return 0;
}

int craftos_machine_mount_real(craftos_machine_t machine, const char * src, const char * dest, int readOnly) {
    struct string_list pathc = {NULL, NULL};
    int n;
    struct string_list_node * node;
    struct craftos_mount_list ** mnt = &machine->mounts;
    while (*mnt) mnt = &(*mnt)->next;
    if (string_split_path(dest, &pathc, 1) != 0) return -1;
    *mnt = F.malloc(sizeof(struct craftos_mount_list));
    if (*mnt == NULL) {
        string_list_clear(&pathc);
        return -1;
    }
    for (node = pathc.head, n = 0; node; node = node->next, n++);
    (*mnt)->next = NULL;
    (*mnt)->flags = readOnly ? MOUNT_FLAG_RO : 0;
    (*mnt)->filesystem_path = F.malloc(strlen(src) + 1);
    strcpy((*mnt)->filesystem_path, src);
    (*mnt)->mount_path = F.malloc(sizeof(char*) * (n + 1));
    for (n = 0; pathc.head; n++) (*mnt)->mount_path[n] = string_list_shift_str(&pathc);
    (*mnt)->mount_path[n] = NULL;
    return 0;
}

int craftos_machine_mount_mmfs(craftos_machine_t machine, const void * src, const char * dest) {
    struct string_list pathc = {NULL, NULL};
    int n;
    struct string_list_node * node;
    struct craftos_mount_list ** mnt = &machine->mounts;
    if (strncmp((const char*)src, "MMfs", 4) != 0) return -1;
    while (*mnt) mnt = &(*mnt)->next;
    if (string_split_path(dest, &pathc, 1) != 0) return -1;
    *mnt = F.malloc(sizeof(struct craftos_mount_list));
    if (*mnt == NULL) {
        string_list_clear(&pathc);
        return -1;
    }
    for (node = pathc.head, n = 0; node; node = node->next, n++);
    (*mnt)->next = NULL;
    (*mnt)->flags = MOUNT_FLAG_MMFS | MOUNT_FLAG_RO;
    (*mnt)->root_dir = src;
    (*mnt)->mount_path = F.malloc(sizeof(char*) * (n + 1));
    for (n = 0; pathc.head; n++) (*mnt)->mount_path[n] = string_list_shift_str(&pathc);
    (*mnt)->mount_path[n] = NULL;
    return 0;
}

int craftos_machine_unmount(craftos_machine_t machine, const char * path) {

}

int craftos_machine_peripheral_attach(craftos_machine_t machine, const char * side, const char ** types, const struct luaL_Reg * funcs, void * userp) {
    struct craftos_peripheral_list * peripheral = F.malloc(sizeof(struct craftos_peripheral_list));
    if (peripheral == NULL) return -1;
    peripheral->next = machine->peripherals;
    peripheral->side = side;
    peripheral->types = types;
    peripheral->userp = userp;
    peripheral->funcs = funcs;
    machine->peripherals = peripheral;
    return 0;
}

int craftos_machine_peripheral_detach(craftos_machine_t machine, const char * side) {
    struct craftos_peripheral_list ** peripherals = &machine->peripherals;
    while (*peripherals) {
        if (strcmp((*peripherals)->side, side) == 0) {
            struct craftos_peripheral_list * next = (*peripherals)->next;
            F.free(*peripherals);
            *peripherals = next;
            return 0;
        }
    }
    return -1;
}
