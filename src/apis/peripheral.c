#include "../types.h"
#include <string.h>
#include <lua.h>
#include <lauxlib.h>

static int peripheral_isPresent(lua_State *L) {
    struct craftos_peripheral_list * p;
    craftos_machine_t computer = get_comp(L);
    const char * side = luaL_checkstring(L, 1);
    for (p = computer->peripherals; p; p = p->next) {
        if (strcmp(p->side, side) == 0) {
            lua_pushboolean(L, 1);
            return 1;
        }
    }
    lua_pushboolean(L, 0);
    return 1;
}

static int peripheral_getType(lua_State *L) {
    struct craftos_peripheral_list * p;
    craftos_machine_t computer = get_comp(L);
    const char * side = luaL_checkstring(L, 1);
    for (p = computer->peripherals; p; p = p->next) {
        if (strcmp(p->side, side) == 0) {
            int i;
            for (i = 0; p->types[i]; i++) lua_pushstring(L, p->types[i]);
            return i;
        }
    }
    return 0;
}

static int peripheral_getMethods(lua_State *L) {
    struct craftos_peripheral_list * p;
    craftos_machine_t computer = get_comp(L);
    const char * side = luaL_checkstring(L, 1);
    for (p = computer->peripherals; p; p = p->next) {
        if (strcmp(p->side, side) == 0) {
            int i;
            for (i = 0; p->funcs[i].name; i++) lua_pushstring(L, p->funcs[i].name);
            return i;
        }
    }
    return luaL_error(L, "No such peripheral");
}

static int peripheral_call(lua_State *L) {
    struct craftos_peripheral_list * p;
    craftos_machine_t computer = get_comp(L);
    const char * side = luaL_checkstring(L, 1);
    const char * func = luaL_checkstring(L, 2);
    for (p = computer->peripherals; p; p = p->next) {
        if (strcmp(p->side, side) == 0) {
            int i;
            for (i = 0; p->funcs[i].name; i++) {
                if (strcmp(p->funcs[i].name, func) == 0) {
                    lua_remove(L, 1);
                    lua_pushlightuserdata(L, p->userp);
                    lua_replace(L, 1);
                    return p->funcs[i].func(L);
                }
            }
            return luaL_error(L, "No such method");
        }
    }
    return luaL_error(L, "No such peripheral");
}

static int peripheral_hasType(lua_State *L) {
    struct craftos_peripheral_list * p;
    craftos_machine_t computer = get_comp(L);
    const char * side = luaL_checkstring(L, 1);
    const char * type = luaL_checkstring(L, 2);
    for (p = computer->peripherals; p; p = p->next) {
        if (strcmp(p->side, side) == 0) {
            int i;
            for (i = 0; p->types[i]; i++) {
                if (strcmp(type, p->types[i]) == 0) {
                    lua_pushboolean(L, 1);
                    return 1;
                }
            }
            lua_pushboolean(L, 0);
            return 1;
        }
    }
    return luaL_error(L, "No such peripheral");
}

const luaL_Reg peripheral_lib[] = {
    {"isPresent", peripheral_isPresent},
    {"getType", peripheral_getType},
    {"getMethods", peripheral_getMethods},
    {"call", peripheral_call},
    {"hasType", peripheral_hasType},
    {NULL, NULL}
};
