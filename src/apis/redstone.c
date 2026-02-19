#include <craftos.h>
#include "../types.h"
#include <lua.h>
#include <lauxlib.h>
#include <string.h>

static const char* side_names[] = {"left", "right", "top", "bottom", "front", "back"};

static craftos_redstone_side_t get_channel(const char* name) {
    int i;
    for (i = 0; i < 6; i++) if (strcmp(name, side_names[i]) == 0) return (craftos_redstone_side_t)i;
    return CRAFTOS_REDSTONE_SIDE_INVALID;
}

static int rs_getSides(lua_State *L) {
    int i;
    lua_createtable(L, 6, 0);
    for (i = 0; i < 6; i++) {
        lua_pushstring(L, side_names[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static int rs_getInput(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    craftos_redstone_side_t channel = get_channel(luaL_checkstring(L, 1));
    if (channel == CRAFTOS_REDSTONE_SIDE_INVALID) luaL_error(L, "Invalid side");
    lua_pushboolean(L, F.redstone_getInput ? F.redstone_getInput(channel, machine) : 0);
    return 1;
}

static int rs_getOutput(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    craftos_redstone_side_t channel = get_channel(luaL_checkstring(L, 1));
    if (channel == CRAFTOS_REDSTONE_SIDE_INVALID) luaL_error(L, "Invalid side");
    lua_pushboolean(L, machine->rs_output[channel]);
    return 1;
}

static int rs_setOutput(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    craftos_redstone_side_t channel = get_channel(luaL_checkstring(L, 1));
    if (channel == CRAFTOS_REDSTONE_SIDE_INVALID) luaL_error(L, "Invalid side");
    machine->rs_output[channel] = lua_toboolean(L, 2) ? 15 : 0;
    if (F.redstone_setOutput) F.redstone_setOutput(channel, lua_toboolean(L, 2) ? 15 : 0, machine);
    return 0;
}

static int rs_getAnalogInput(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    craftos_redstone_side_t channel = get_channel(luaL_checkstring(L, 1));
    if (channel == CRAFTOS_REDSTONE_SIDE_INVALID) luaL_error(L, "Invalid side");
    lua_pushinteger(L, F.redstone_getInput ? F.redstone_getInput(channel, machine) : 0);
    return 1;
}

static int rs_getAnalogOutput(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    craftos_redstone_side_t channel = get_channel(luaL_checkstring(L, 1));
    if (channel == CRAFTOS_REDSTONE_SIDE_INVALID) luaL_error(L, "Invalid side");
    lua_pushinteger(L, machine->rs_output[channel]);
    return 1;
}

static int rs_setAnalogOutput(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    craftos_redstone_side_t channel = get_channel(luaL_checkstring(L, 1));
    if (channel == CRAFTOS_REDSTONE_SIDE_INVALID) luaL_error(L, "Invalid side");
    int num = luaL_checkinteger(L, 2);
    if (num < 0) num = 0;
    else if (num > 15) num = 15;
    machine->rs_output[channel] = num;
    if (F.redstone_setOutput) F.redstone_setOutput(channel, num, machine);
    return 0;
}

static int rs_getBundledInput(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    craftos_redstone_side_t channel = get_channel(luaL_checkstring(L, 1));
    if (channel == CRAFTOS_REDSTONE_SIDE_INVALID) luaL_error(L, "Invalid side");
    if (F.redstone_getInput) lua_pushinteger(L, F.redstone_getInput(channel + 6, machine));
    else lua_pushnumber(L, 0);
    return 1;
}

static int rs_getBundledOutput(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    craftos_redstone_side_t channel = get_channel(luaL_checkstring(L, 1));
    if (channel == CRAFTOS_REDSTONE_SIDE_INVALID) luaL_error(L, "Invalid side");
    lua_pushinteger(L, machine->rs_output[channel + 6]);
    return 1;
}

static int rs_setBundledOutput(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    craftos_redstone_side_t channel = get_channel(luaL_checkstring(L, 1));
    if (channel == CRAFTOS_REDSTONE_SIDE_INVALID) luaL_error(L, "Invalid side");
    machine->rs_output[channel + 6] = luaL_checkinteger(L, 2);
    if (F.redstone_setOutput) F.redstone_setOutput(channel + 6, machine->rs_output[channel + 6], machine);
    return 0;
}

static int rs_testBundledInput(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    craftos_redstone_side_t channel = get_channel(luaL_checkstring(L, 1));
    if (channel == CRAFTOS_REDSTONE_SIDE_INVALID) luaL_error(L, "Invalid side");
    unsigned short mask = luaL_checkinteger(L, 2);
    if (F.redstone_getInput) lua_pushboolean(L, (F.redstone_getInput(channel + 6, machine) & mask) == mask);
    else lua_pushboolean(L, mask == 0);
    return 1;
}

const luaL_Reg rs_lib[] = {
    {"getSides", rs_getSides},
    {"getInput", rs_getInput},
    {"setOutput", rs_setOutput},
    {"getOutput", rs_getOutput},
    {"getAnalogInput", rs_getAnalogInput},
    {"setAnalogOutput", rs_setAnalogOutput},
    {"getAnalogOutput", rs_getAnalogOutput},
    {"getAnalogueInput", rs_getAnalogInput},
    {"setAnalogueOutput", rs_setAnalogOutput},
    {"getAnalogueOutput", rs_getAnalogOutput},
    {"getBundledInput", rs_getBundledInput},
    {"getBundledOutput", rs_getBundledOutput},
    {"setBundledOutput", rs_setBundledOutput},
    {"testBundledInput", rs_testBundledInput},
    {NULL, NULL}
};
