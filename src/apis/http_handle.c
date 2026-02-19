#include <craftos.h>
#include "../types.h"
#include "../mmfs.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>

int http_handle_close(lua_State *L) {
    craftos_http_handle_t * fp = (craftos_http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (*fp == NULL) return luaL_error(L, "attempt to use a closed file");
    F.http_handle_close(*fp, get_comp(L));
    *fp = NULL;
    return 0;
}

int http_handle_gc(lua_State *L) {
    craftos_http_handle_t* fp = (craftos_http_handle_t*)lua_touserdata(L, 1);
    if (*fp == NULL) return 0;
    F.http_handle_close(*fp, get_comp(L));
    *fp = NULL;
    return 0;
}

int http_handle_readLine(lua_State *L) {
    craftos_http_handle_t fp = *(craftos_http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    unsigned i, j;
    size_t sz = 0;
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (F.http_handle_eof(fp, machine)) return 0;
    char* retval = (char*)F.malloc(256);
    for (i = 0; 1; i += 256) {
        for (j = 0; j < 256 && !F.http_handle_eof(fp, machine); j++) {
            int c = F.http_handle_getc(fp, machine);
            if (c == EOF) c = 0;
            retval[i+j] = c;
            if (c == '\n') break;
        }
        sz += j;
        if (F.http_handle_eof(fp, machine) || retval[i+j] == '\n' || j < 256) break;
        char * retvaln = (char*)F.realloc(retval, i + 512);
        if (retvaln == NULL) {
            F.free(retval);
            return luaL_error(L, "failed to allocate memory");
        }
        retval = retvaln;
    }
    size_t len = sz - (sz > 0 && retval[sz-1] == '\n' && !lua_toboolean(L, 1) ? 1 : 0);
    if (len == 0 && F.http_handle_eof(fp, machine)) {F.free(retval); return 0;}
    lua_pushlstring(L, retval, len);
    F.free(retval);
    return 1;
}

int http_handle_readChar(lua_State *L) {
    craftos_http_handle_t fp = *(craftos_http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (F.http_handle_eof(fp, machine)) return 0;
    if (lua_isnumber(L, 1)) {
        if (lua_tointeger(L, 1) < 0) luaL_error(L, "Cannot read a negative number of characters");
        const size_t s = lua_tointeger(L, 1);
        if (s == 0) {
            if (/*fp->peek() == EOF ||*/ F.http_handle_eof(fp, machine)) return 0;
            lua_pushstring(L, "");
            return 1;
        }
        char* retval = F.malloc(s);
        const size_t actual = F.http_handle_read(retval, 1, s, fp, machine);
        if (actual == 0) {F.free(retval); return 0;}
        lua_pushlstring(L, retval, actual);
        F.free(retval);
    } else {
        const int retval = F.http_handle_getc(fp, machine);
        if (retval == EOF || F.http_handle_eof(fp, machine)) return 0;
        lua_pushlstring(L, (const char *)&retval, 1);
    }
    return 1;
}

int http_handle_readByte(lua_State *L) {
    craftos_http_handle_t fp = *(craftos_http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (F.http_handle_eof(fp, machine)) return 0;
    if (lua_isnumber(L, 1)) {
        if (lua_tointeger(L, 1) < 0) luaL_error(L, "Cannot read a negative number of characters");
        const size_t s = lua_tointeger(L, 1);
        if (s == 0) {
            if (/*fp->peek() == EOF ||*/ F.http_handle_eof(fp, machine)) return 0;
            lua_pushstring(L, "");
            return 1;
        }
        char* retval = F.malloc(s);
        const size_t actual = F.http_handle_read(retval, 1, s, fp, machine);
        if (actual == 0) {F.free(retval); return 0;}
        lua_pushlstring(L, retval, actual);
        F.free(retval);
    } else {
        const int retval = F.http_handle_getc(fp, machine);
        if (retval == EOF || F.http_handle_eof(fp, machine)) return 0;
        lua_pushinteger(L, (unsigned char)retval);
    }
    return 1;
}

int http_handle_readAllByte(lua_State *L) {
    craftos_http_handle_t fp = *(craftos_http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (F.http_handle_eof(fp, machine)) {
        if (F.http_handle_tell(fp, machine) < 1) return 0;
        lua_pushliteral(L, "");
        return 1;
    }
    const long pos = F.http_handle_tell(fp, machine);
    F.http_handle_seek(fp, 0, SEEK_END, machine);
    long size = (long)F.http_handle_tell(fp, machine) - pos;
    F.http_handle_seek(fp, pos, SEEK_SET, machine);
    char * str = (char*)F.malloc(size);
    if (str == NULL) return luaL_error(L, "failed to allocate memory");
    size = F.http_handle_read(str, 1, size, fp, machine);
    F.http_handle_getc(fp, machine); /* should set EOF */
    lua_pushlstring(L, str, size);
    F.free(str);
    return 1;
}

int http_handle_seek(lua_State *L) {
    craftos_http_handle_t fp = *(craftos_http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    const char * whence = luaL_optstring(L, 1, "cur");
    const size_t offset = luaL_optinteger(L, 2, 0);
    if (strcmp(whence, "set") == 0 && luaL_optinteger(L, 2, 0) < 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "Position is negative");
        return 2;
    }
    int origin;
    if (strcmp(whence, "set") == 0) origin = SEEK_SET;
    else if (strcmp(whence, "cur") == 0) origin = SEEK_CUR;
    else if (strcmp(whence, "end") == 0) origin = SEEK_END;
    else return luaL_error(L, "bad argument #1 to 'seek' (invalid option '%s')", whence);
    F.http_handle_seek(fp, offset, origin, machine);
    lua_pushinteger(L, F.http_handle_tell(fp, machine));
    return 1;
}

int http_handle_getResponseCode(lua_State *L) {
    craftos_http_handle_t fp = *(craftos_http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    lua_pushinteger(L, F.http_handle_getResponseCode(fp, machine));
    return 1;
}

int http_handle_getResponseHeaders(lua_State *L) {
    craftos_http_handle_t fp = *(craftos_http_handle_t*)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    lua_newtable(L);
    craftos_http_header_t * header = NULL;
    for (F.http_handle_getResponseHeader(fp, &header, machine); header; F.http_handle_getResponseHeader(fp, &header, machine)) {
        lua_pushstring(L, header->key);
        lua_pushstring(L, header->value);
        lua_settable(L, -3);
    }
    return 1;
}

int websocket_gc(lua_State *L) {
    struct websocket_handle ** ws = (struct websocket_handle **)lua_touserdata(L, 1);
    if (*ws == NULL) return 0;
    F.http_websocket_close((*ws)->handle, get_comp(L));
    F.free((*ws)->url);
    F.free(*ws);
    *ws = NULL;
    return 0;
}

int websocket_close(lua_State *L) {
    struct websocket_handle * * ws = (struct websocket_handle **)lua_touserdata(L, lua_upvalueindex(1));
    if (*ws == NULL) return 0;
    F.http_websocket_close((*ws)->handle, get_comp(L));
    F.free((*ws)->url);
    F.free(*ws);
    *ws = NULL;
    return 0;
}

int websocket_send(lua_State *L) {
    size_t sz;
    const char * str = luaL_checklstring(L, 1, &sz);
    if (sz > 131072) luaL_error(L, "Message is too large");
    struct websocket_handle * ws = *(struct websocket_handle **)lua_touserdata(L, lua_upvalueindex(1));
    if (ws == NULL) return luaL_error(L, "attempt to use a closed file");
    F.http_websocket_send(ws->handle, sz, str, lua_toboolean(L, 2), get_comp(L));
    return 0;
}

int websocket_receive(lua_State *L) {
    struct websocket_handle * ws = *(struct websocket_handle **)lua_touserdata(L, lua_upvalueindex(1));
    int tm = 0;
    if (lua_getctx(L, &tm) == LUA_YIELD) {
        if (lua_isstring(L, 1)) {
            if (ws == NULL) {
                lua_pushnil(L);
                return 1;
            }
            const char * ev = lua_tostring(L, 1);
            const char * url = NULL;
            int tmid = 0;
            if (lua_isnumber(L, 2)) tmid = lua_tointeger(L, 2);
            else if (lua_isstring(L, 2)) url = lua_tostring(L, 2);
            if (strcmp(ev, "websocket_message") == 0 && strcmp(url, ws->url) == 0) {
                if (tm > 0) {
                    if (F.cancelTimer) {
                        F.cancelTimer(tm, get_comp(L));
                    } else {
                        /* TODO: software timers */
                    }
                }
                lua_pushvalue(L, 3);
                lua_pushvalue(L, 4);
                return 2;
            } else if ((strcmp(ev, "websocket_closed") == 0 && strcmp(url, ws->url) == 0) ||
                       (tm > 0 && strcmp(ev, "timer") == 0 && lua_isnumber(L, 2) && tmid == tm)) {
                lua_pushnil(L);
                return 1;
            } else if (strcmp(ev, "terminate") == 0) {
                if (tm > 0) {
                    if (F.cancelTimer) {
                        F.cancelTimer(tm, get_comp(L));
                    } else {
                        /* TODO: software timers */
                    }
                }
                return luaL_error(L, "Terminated");
            }
        }
    } else {
        if (ws == NULL) luaL_error(L, "attempt to use a closed file");
        if (!lua_isnoneornil(L, 1)) {
            if (F.startTimer) {
                tm = F.startTimer(luaL_checknumber(L, 1) * 1000, get_comp(L));
            } else {
                /* TODO: software timers */
            }
        } else tm = -1;
    }
    lua_settop(L, 0);
    return lua_yieldk(L, 0, tm, websocket_receive);
}
