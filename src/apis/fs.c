#include <craftos.h>
#include "../types.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>

extern int fs_handle_close(lua_State *L);
extern int fs_handle_gc(lua_State *L);
extern int fs_handle_readAll(lua_State *L);
extern int fs_handle_readLine(lua_State *L);
extern int fs_handle_readByte(lua_State *L);
extern int fs_handle_readAllByte(lua_State *L);
extern int fs_handle_writeString(lua_State *L);
extern int fs_handle_writeLine(lua_State *L);
extern int fs_handle_writeByte(lua_State *L);
extern int fs_handle_flush(lua_State *L);
extern int fs_handle_seek(lua_State *L);

extern int fs_handle_mmfs_close(lua_State *L);
extern int fs_handle_mmfs_gc(lua_State *L);
extern int fs_handle_mmfs_readAll(lua_State *L);
extern int fs_handle_mmfs_readLine(lua_State *L);
extern int fs_handle_mmfs_readByte(lua_State *L);
extern int fs_handle_mmfs_readAllByte(lua_State *L);
extern int fs_handle_mmfs_seek(lua_State *L);
