#include <craftos.h>
#include "../types.h"
#include <ctype.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>

extern int http_handle_close(lua_State *L);
extern int http_handle_gc(lua_State *L);
extern int http_handle_readLine(lua_State *L);
extern int http_handle_readByte(lua_State *L);
extern int http_handle_readAllByte(lua_State *L);
extern int http_handle_seek(lua_State *L);
extern int http_handle_getResponseCode(lua_State *L);
extern int http_handle_getResponseHeaders(lua_State *L);

extern int websocket_close(lua_State *L);
extern int websocket_gc(lua_State *L);
extern int websocket_send(lua_State *L);
extern int websocket_receive(lua_State *L);

const struct luaL_Reg http_handle_funcs[] = {
    {"close", http_handle_close},
    {"readAll", http_handle_readAllByte},
    {"readLine", http_handle_readLine},
    {"read", http_handle_readByte},
    {"seek", http_handle_seek},
    {"getResponseCode", http_handle_getResponseCode},
    {"getResponseHeaders", http_handle_getResponseHeaders},
    {NULL, NULL}
};

static luaL_Reg websocket_funcs[] = {
    {"close", websocket_close},
    {"receive", websocket_receive},
    {"send", websocket_send},
    {NULL, NULL}
};

static int stricmp_(const char * a, const char * b) {
    for (; *a && *b; a++, b++)
        if (tolower(*a) != tolower(*b)) return *a < *b ? -1 : 1;
    return 0;
}

void craftos_event_http_success(craftos_machine_t machine, const char * url, craftos_http_handle_t handle) {
    int code = F.http_handle_getResponseCode(handle, machine);
    if (code < 400)
        craftos_machine_queue_event(machine, "http_success", "zPGR", url, handle, http_handle_gc, http_handle_funcs);
    else {
        char err[24];
        snprintf(err, 24, "HTTP Status %d", code);
        craftos_machine_queue_event(machine, "http_failure", "zxzPGR", url, err, handle, http_handle_gc, http_handle_funcs);
    }
}

void craftos_event_websocket_success(craftos_machine_t machine, const char * url, craftos_http_websocket_t handle) {
    struct websocket_handle * h = F.malloc(sizeof(struct websocket_handle));
    h->handle = handle;
    h->url = F.malloc(strlen(url) + 1);
    strcpy(h->url, url);
    craftos_machine_queue_event(machine, "websocket_success", "zPGR", url, handle, websocket_gc, websocket_funcs);
}

static int http_request(lua_State *L) {
    const char * url, * method = "GET";
    const unsigned char * body = NULL;
    size_t body_size = 0, header_count = 0;
    craftos_http_header_t * headers = NULL;
    int redirect = 1;
    char content_length[16];
    if (F.http_request == NULL) {
        lua_pushboolean(L, 0);
        return 1;
    }
    if (!lua_isstring(L, 1) && !lua_istable(L, 1)) luaL_error(L, "bad argument #1 (expected string or table, got %s)", lua_typename(L, lua_type(L, 1)));
    craftos_machine_t comp = get_comp(L);
    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "url");
        if (!lua_isstring(L, -1)) {return luaL_error(L, "bad field 'url' (string expected, got %s)", lua_typename(L, lua_type(L, -1)));}
        url = lua_tostring(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 1, "body");
        if (!lua_isnil(L, -1) && !lua_isstring(L, -1)) {return luaL_error(L, "bad field 'body' (string expected, got %s)", lua_typename(L, lua_type(L, -1)));}
        else if (lua_isstring(L, -1)) body = lua_tolstring(L, -1, &body_size);
        lua_pop(L, 1);
        lua_getfield(L, 1, "method");
        if (!lua_isnil(L, -1) && !lua_isstring(L, -1)) {return luaL_error(L, "bad field 'method' (string expected, got %s)", lua_typename(L, lua_type(L, -1)));}
        else if (lua_isstring(L, -1)) method = lua_tostring(L, -1);
        else method = body ? "POST" : "GET";
        lua_pop(L, 1);
        lua_getfield(L, 1, "redirect");
        if (!lua_isnil(L, -1) && !lua_isboolean(L, -1)) {return luaL_error(L, "bad field 'redirect' (boolean expected, got %s)", lua_typename(L, lua_type(L, -1)));}
        else if (lua_isboolean(L, -1)) redirect = lua_toboolean(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 1, "headers");
        if (!lua_isnil(L, -1) && !lua_istable(L, -1)) {return luaL_error(L, "bad field 'headers' (table expected, got %s)", lua_typename(L, lua_type(L, -1)));}
        else if (lua_istable(L, -1)) {
            int i = 0;
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                header_count++;
                lua_pop(L, 1);
            }
            headers = F.malloc(sizeof(craftos_http_header_t) * (header_count + 1));
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                const char * key = lua_tostring(L, -2), *val = lua_tostring(L, -1);
                if (key && val) {
                    headers[i].key = key;
                    headers[i++].value = val;
                }
                lua_pop(L, 1);
            }
            headers[i].key = headers[i].value = NULL;
            header_count = i;
        }
        lua_pop(L, 1);
        /*
        lua_getfield(L, 1, "timeout");
        if (!lua_isnil(L, -1) && !lua_isnumber(L, -1)) {return luaL_error(L, "bad field 'timeout' (number expected, got %s)", lua_typename(L, lua_type(L, -1)));}
        else if (lua_isnumber(L, -1)) timeout = lua_tonumber(L, -1);
        else timeout = 0;
        lua_pop(L, 1);
        */
    } else {
        url = luaL_checkstring(L, 1);
        if (!lua_isnoneornil(L, 2)) body = luaL_checklstring(L, 2, &body_size);
        if (lua_istable(L, 3)) {
            int i = 0;
            lua_pushvalue(L, 3);
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                header_count++;
                lua_pop(L, 1);
            }
            headers = F.malloc(sizeof(craftos_http_header_t) * (header_count + 1));
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                const char * key = lua_tostring(L, -2), *val = lua_tostring(L, -1);
                if (key && val) {
                    headers[i].key = key;
                    headers[i++].value = val;
                }
                lua_pop(L, 1);
            }
            headers[i].key = headers[i].value = NULL;
            header_count = i;
        }
        method = luaL_optstring(L, 5, body ? "POST" : "GET");
        redirect = !lua_isboolean(L, 6) || lua_toboolean(L, 6);
    }
    int have_headers = 0, i = 0;
    for (; i < header_count; i++) {
        if (stricmp_(headers[i].key, "user-agent") == 0) have_headers |= 1;
        if (stricmp_(headers[i].key, "content-type") == 0) have_headers |= 2;
        if (stricmp_(headers[i].key, "content-length") == 0) have_headers |= 4;
        if (stricmp_(headers[i].key, "accept-charset") == 0) have_headers |= 8;
    }
    if (!(have_headers & 1)) {
        headers = F.realloc(headers, sizeof(craftos_http_header_t) * (header_count + 2));
        headers[header_count].key = "User-Agent";
        headers[header_count++].value = "computercraft/" CRAFTOS_CC_VERSION " CraftOS-Base/" CRAFTOS_BASE_VERSION;
        headers[header_count].key = headers[header_count].value = NULL;
    }
    if (!(have_headers & 8)) {
        headers = F.realloc(headers, sizeof(craftos_http_header_t) * (header_count + 2));
        headers[header_count].key = "Accept-Charset";
        headers[header_count++].value = "UTF-8";
        headers[header_count].key = headers[header_count].value = NULL;
    }
    if (body) {
        if (!(have_headers & 2)) {
            headers = F.realloc(headers, sizeof(craftos_http_header_t) * (header_count + 2));
            headers[header_count].key = "Content-Type";
            headers[header_count++].value = "application/x-www-form-urlencoded; charset=utf-8";
            headers[header_count].key = headers[header_count].value = NULL;
        }
        if (!(have_headers & 4)) {
            snprintf(content_length, 16, "%d", body_size);
            headers = F.realloc(headers, sizeof(craftos_http_header_t) * (header_count + 2));
            headers[header_count].key = "Content-Length";
            headers[header_count++].value = content_length;
            headers[header_count].key = headers[header_count].value = NULL;
        }
    }
    lua_pushboolean(L, F.http_request(url, method, body, body_size, headers, redirect, comp) == 0);
    if (headers) F.free(headers);
    return 1;
}

static int http_checkURL(lua_State *L) {
    if (F.http_request == NULL) {
        lua_pushboolean(L, 0);
        return 1;
    }
    /* TODO: implement? */
    craftos_machine_queue_event(get_comp(L), "http_check", "zq", luaL_checkstring(L, 1), 1);
    lua_pushboolean(L, 1);
    return 1;
}

const luaL_Reg http_lib[] = {
    {"request", http_request},
    {"checkURL", http_checkURL},
    {NULL, NULL}
};

static int http_websocket(lua_State *L) {
    craftos_machine_t comp = get_comp(L);
    const char * url;
    craftos_http_header_t * headers = NULL;
    size_t header_count = 0;
    if (F.http_websocket == NULL) luaL_error(L, "Websocket connections are disabled");
    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "url");
        if (!lua_isstring(L, -1)) luaL_error(L, "bad field 'url' (expected string, got %s)", lua_typename(L, lua_type(L, -1)));
        url = lua_tostring(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, 1, "headers");
        if (!lua_isnil(L, -1) && !lua_istable(L, -1)) luaL_error(L, "bad field 'headers' (expected table, got %s)", lua_typename(L, lua_type(L, -1)));
        if (lua_istable(L, -1)) {
            int i = 0;
            lua_pushvalue(L, 3);
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                header_count++;
                lua_pop(L, 1);
            }
            headers = F.malloc(sizeof(craftos_http_header_t) * (header_count + 1));
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                const char * key = lua_tostring(L, -2), *val = lua_tostring(L, -1);
                if (key && val) {
                    headers[i].key = key;
                    headers[i++].value = val;
                }
                lua_pop(L, 1);
            }
            headers[i].key = headers[i].value = NULL;
            header_count = i;
        }
        lua_pop(L, 1);
        /*
        lua_getfield(L, 1, "timeout");
        if (!lua_isnil(L, -1) && !lua_isnumber(L, -1)) luaL_error(L, "bad field 'timeout' (expected number, got %s)", lua_typename(L, lua_type(L, -1)));
        double timeout = luaL_optnumber(L, -1, config.http_timeout / 1000.0);
        lua_pop(L, 1);
        */
    } else if (lua_isstring(L, 1)) {
        url = lua_tostring(L, 1);
        if (lua_istable(L, 2)) {
            int i = 0;
            lua_pushvalue(L, 3);
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                header_count++;
                lua_pop(L, 1);
            }
            headers = F.malloc(sizeof(craftos_http_header_t) * (header_count + 1));
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                const char * key = lua_tostring(L, -2), *val = lua_tostring(L, -1);
                if (key && val) {
                    headers[i].key = key;
                    headers[i++].value = val;
                }
                lua_pop(L, 1);
            }
            headers[i].key = headers[i].value = NULL;
            header_count = i;
        }
    } else luaL_error(L, "bad argument #1 (expected string or table, got %s)", lua_typename(L, lua_type(L, 1)));
    lua_pushboolean(L, F.http_websocket(url, headers, comp) == 0);
    if (headers) F.free(headers);
    return 1;
}

const luaL_Reg http_lib_ws[] = {
    {"websocket", http_websocket},
    {NULL, NULL}
};
