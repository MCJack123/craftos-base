#include <craftos.h>
#include "../types.h"
#include "../mmfs.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>

int fs_handle_close(lua_State *L) {
    FILE ** fp = (FILE**)lua_touserdata(L, lua_upvalueindex(1));
    if (*fp == NULL)
        return luaL_error(L, "attempt to use a closed file");
    F.fclose(*fp, get_comp(L));
    *fp = NULL;
    get_comp(L)->files_open--;
    return 0;
}

int fs_handle_gc(lua_State *L) {
    FILE ** fp = (FILE**)lua_touserdata(L, lua_upvalueindex(1));
    if (*fp == NULL)
        return 0;
    F.fclose(*fp, get_comp(L));
    *fp = NULL;
    get_comp(L)->files_open--;
    return 0;
}

int fs_handle_readLine(lua_State *L) {
    FILE * fp = *(FILE**)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    unsigned i, j;
    size_t sz = 0;
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (F.feof(fp, machine)) return 0;
    if (F.ferror(fp, machine)) return luaL_error(L, "Could not read file");
    char* retval = (char*)F.malloc(256);
    for (i = 0; 1; i += 256) {
        for (j = 0; j < 256 && !F.feof(fp, machine); j++) {
            int c = F.fgetc(fp, machine);
            if (c == EOF) c = 0;
            retval[i+j] = c;
            if (c == '\n') break;
        }
        sz += j;
        if (F.feof(fp, machine) || retval[i+j] == '\n' || j < 256) break;
        char * retvaln = (char*)F.realloc(retval, i + 512);
        if (retvaln == NULL) {
            F.free(retval);
            return luaL_error(L, "failed to allocate memory");
        }
        retval = retvaln;
    }
    size_t len = sz - (sz > 0 && retval[sz-1] == '\n' && !lua_toboolean(L, 1) ? 1 : 0);
    if (len == 0 && F.feof(fp, machine)) {F.free(retval); return 0;}
    lua_pushlstring(L, retval, len);
    F.free(retval);
    return 1;
}

int fs_handle_readChar(lua_State *L) {
    FILE * fp = *(FILE**)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (F.feof(fp, machine)) return 0;
    if (F.ferror(fp, machine)) luaL_error(L, "Could not read file");
    if (lua_isnumber(L, 1)) {
        if (lua_tointeger(L, 1) < 0) luaL_error(L, "Cannot read a negative number of characters");
        const size_t s = lua_tointeger(L, 1);
        if (s == 0) {
            if (/*fp->peek() == EOF ||*/ F.feof(fp, machine)) return 0;
            lua_pushstring(L, "");
            return 1;
        }
        char* retval = F.malloc(s);
        const size_t actual = F.fread(retval, 1, s, fp, machine);
        if (actual == 0) {F.free(retval); return 0;}
        lua_pushlstring(L, retval, actual);
        F.free(retval);
    } else {
        const int retval = F.fgetc(fp, machine);
        if (retval == EOF || F.feof(fp, machine)) return 0;
        lua_pushlstring(L, (const char *)&retval, 1);
    }
    return 1;
}

int fs_handle_readByte(lua_State *L) {
    FILE * fp = *(FILE**)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (F.feof(fp, machine)) return 0;
    if (F.ferror(fp, machine)) return luaL_error(L, "Could not read file");
    if (lua_isnumber(L, 1)) {
        if (lua_tointeger(L, 1) < 0) luaL_error(L, "Cannot read a negative number of characters");
        const size_t s = lua_tointeger(L, 1);
        if (s == 0) {
            if (/*fp->peek() == EOF ||*/ F.feof(fp, machine)) return 0;
            lua_pushstring(L, "");
            return 1;
        }
        char* retval = F.malloc(s);
        const size_t actual = F.fread(retval, 1, s, fp, machine);
        if (actual == 0) {F.free(retval); return 0;}
        lua_pushlstring(L, retval, actual);
        F.free(retval);
    } else {
        const int retval = F.fgetc(fp, machine);
        if (retval == EOF || F.feof(fp, machine)) return 0;
        lua_pushinteger(L, (unsigned char)retval);
    }
    return 1;
}

int fs_handle_readAllByte(lua_State *L) {
    FILE * fp = *(FILE**)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (F.feof(fp, machine)) {
        if (F.ftell(fp, machine) < 1) return 0;
        lua_pushliteral(L, "");
        return 1;
    }
    if (F.ferror(fp, machine)) return 0;
    const long pos = F.ftell(fp, machine);
    F.fseek(fp, 0, SEEK_END, machine);
    if (F.ferror(fp, machine)) return 0;
    long size = (long)F.ftell(fp, machine) - pos;
    F.fseek(fp, pos, SEEK_SET, machine);
    char * str = (char*)F.malloc(size);
    if (str == NULL) return luaL_error(L, "failed to allocate memory");
    size = F.fread(str, 1, size, fp, machine);
    F.fgetc(fp, machine); /* should set EOF */
    lua_pushlstring(L, str, size);
    F.free(str);
    return 1;
}

int fs_handle_writeString(lua_State *L) {
    FILE * fp = *(FILE**)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (lua_isnoneornil(L, 1)) return 0;
    else if (!lua_isstring(L, 1) && !lua_isnumber(L, 1)) return luaL_error(L, "bad argument #1 (string expected, got %s)", lua_typename(L, lua_type(L, 1)));
    if (F.feof(fp, machine)) F.fseek(fp, 0, SEEK_END, machine);
    else if (F.ferror(fp, machine)) return luaL_error(L, "Could not write file");
    size_t sz = 0;
    const char * str = lua_tolstring(L, 1, &sz);
    F.fwrite(str, 1, sz, fp, machine);
    return 0;
}

int fs_handle_writeLine(lua_State *L) {
    FILE * fp = *(FILE**)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (lua_isnoneornil(L, 1)) return 0;
    else if (!lua_isstring(L, 1) && !lua_isnumber(L, 1)) return luaL_error(L, "bad argument #1 (string expected, got %s)", lua_typename(L, lua_type(L, 1)));
    if (F.feof(fp, machine)) F.fseek(fp, 0, SEEK_END, machine);
    else if (F.ferror(fp, machine)) return luaL_error(L, "Could not write file");
    size_t sz = 0;
    const char * str = lua_tolstring(L, 1, &sz);
    F.fwrite(str, 1, sz, fp, machine);
    F.fputc('\n', fp, machine);
    return 0;
}

int fs_handle_writeByte(lua_State *L) {
    FILE * fp = *(FILE**)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (F.feof(fp, machine)) F.fseek(fp, 0, SEEK_END, machine);
    else if (F.ferror(fp, machine)) return luaL_error(L, "Could not write file");
    if (lua_type(L, 1) == LUA_TNUMBER) {
        const char b = (unsigned char)(lua_tointeger(L, 1) & 0xFF);
        F.fputc(b, fp, machine);
    } else if (lua_isstring(L, 1)) {
        size_t sz = 0;
        const char * str = lua_tolstring(L, 1, &sz);
        if (sz == 0) return 0;
        F.fwrite(str, 1, sz, fp, machine);
    } else return luaL_error(L, "bad argument #1 (number or string expected, got %s)", lua_typename(L, lua_type(L, 1)));
    return 0;
}

int fs_handle_flush(lua_State *L) {
    FILE * fp = *(FILE**)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    F.fflush(fp, machine);
    return 0;
}

int fs_handle_seek(lua_State *L) {
    FILE * fp = *(FILE**)lua_touserdata(L, lua_upvalueindex(1));
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
    F.fseek(fp, offset, origin, machine);
    if (F.ferror(fp, machine)) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    lua_pushinteger(L, F.ftell(fp, machine));
    return 1;
}

int fs_handle_mmfs_close(lua_State *L) {
    struct mmfs_fd ** fp = (struct mmfs_fd**)lua_touserdata(L, lua_upvalueindex(1));
    if (*fp == NULL)
        return luaL_error(L, "attempt to use a closed file");
    F.free(*fp);
    *fp = NULL;
    get_comp(L)->files_open--;
    return 0;
}

int fs_handle_mmfs_gc(lua_State *L) {
    struct mmfs_fd ** fp = (struct mmfs_fd**)lua_touserdata(L, lua_upvalueindex(1));
    if (*fp == NULL)
        return 0;
    F.free(*fp);
    *fp = NULL;
    get_comp(L)->files_open--;
    return 0;
}

int fs_handle_mmfs_readLine(lua_State *L) {
    struct mmfs_fd * fp = *(struct mmfs_fd**)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    unsigned i, j;
    size_t found = 0;
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (fp->ptr >= fp->end) return 0;
    const unsigned char * start = fp->ptr;
    while (fp->ptr < fp->end && *fp->ptr++ != '\n');
    size_t len = (fp->ptr - start) - ((fp->ptr - start) > 0 && fp->ptr < fp->end && *fp->ptr == '\n' && !lua_toboolean(L, 1));
    if (len == 0 && fp->ptr >= fp->end) {return 0;}
    lua_pushlstring(L, start, len);
    return 1;
}

int fs_handle_mmfs_readByte(lua_State *L) {
    struct mmfs_fd * fp = *(struct mmfs_fd**)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (fp->ptr >= fp->end) return 0;
    if (lua_isnumber(L, 1)) {
        if (lua_tointeger(L, 1) < 0) luaL_error(L, "Cannot read a negative number of characters");
        const size_t s = lua_tointeger(L, 1);
        if (s == 0) {
            lua_pushstring(L, "");
            return 1;
        }
        const size_t actual = fp->ptr + s - 1 >= fp->end ? fp->end - fp->ptr : s;
        if (actual == 0) return 0;
        lua_pushlstring(L, fp->ptr, actual);
        fp->ptr += actual;
    } else {
        lua_pushlstring(L, fp->ptr++, 1);
    }
    return 1;
}

int fs_handle_mmfs_readAllByte(lua_State *L) {
    struct mmfs_fd * fp = *(struct mmfs_fd**)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    if (fp->ptr >= fp->end) {
        if (fp->ptr == fp->start) return 0;
        lua_pushliteral(L, "");
        return 1;
    }
    lua_pushlstring(L, fp->ptr, fp->end - fp->ptr);
    fp->ptr = fp->end;
    return 1;
}

int fs_handle_mmfs_seek(lua_State *L) {
    struct mmfs_fd * fp = *(struct mmfs_fd**)lua_touserdata(L, lua_upvalueindex(1));
    craftos_machine_t machine = get_comp(L);
    if (fp == NULL) return luaL_error(L, "attempt to use a closed file");
    const char * whence = luaL_optstring(L, 1, "cur");
    const long offset = luaL_optinteger(L, 2, 0);
    if (strcmp(whence, "set") == 0 && luaL_optinteger(L, 2, 0) < 0) {
        lua_pushnil(L);
        lua_pushliteral(L, "Position is negative");
        return 2;
    }
    int origin;
    if (strcmp(whence, "set") == 0) fp->ptr = fp->start + offset;
    else if (strcmp(whence, "cur") == 0) fp->ptr += offset;
    else if (strcmp(whence, "end") == 0) fp->ptr = fp->end - offset;
    else return luaL_error(L, "bad argument #1 to 'seek' (invalid option '%s')", whence);
    if (fp->ptr < fp->start) fp->ptr = fp->start; /* no arbitrary memory read/write for you ;) */
    lua_pushinteger(L, fp->ptr - fp->start);
    return 1;
}
