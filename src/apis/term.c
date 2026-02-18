#include <craftos.h>
#include "../types.h"
#include "../terminal.h"
#include <lua.h>
#include <lauxlib.h>
#include <math.h>

int term_write(lua_State *L) {
    size_t sz;
    const char* text = luaL_checklstring(L, 1, &sz);
    craftos_machine_t machine = get_comp(L);
    craftos_terminal_write(machine->term, machine->term->cursorX, machine->term->cursorY, (const uint8_t*)text, sz, machine->term->activeColors);
    machine->term->cursorX += sz;
    craftos_terminal_cursor(machine->term, machine->term->canBlink ? machine->term->activeColors & 0xF : -1, machine->term->cursorX, machine->term->cursorY);
    return 0;
}

int term_scroll(lua_State *L) {
    int scroll = luaL_checkinteger(L, 1);
    craftos_machine_t machine = get_comp(L);
    craftos_terminal_scroll(machine->term, scroll, machine->term->activeColors);
    return 0;
}

int term_setCursorPos(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    machine->term->cursorX = luaL_checkinteger(L, 1) - 1;
    machine->term->cursorY = luaL_checkinteger(L, 2) - 1;
    craftos_terminal_cursor(machine->term, machine->term->canBlink ? machine->term->activeColors & 0xF : -1, machine->term->cursorX, machine->term->cursorY);
    return 0;
}

int term_getCursorBlink(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    lua_pushboolean(L, machine->term->canBlink);
    return 1;
}

int term_setCursorBlink(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    machine->term->canBlink = lua_toboolean(L, 1);
    craftos_terminal_cursor(machine->term, machine->term->canBlink ? machine->term->activeColors & 0xF : -1, machine->term->cursorX, machine->term->cursorY);
    return 0;
}

int term_getCursorPos(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    lua_pushinteger(L, machine->term->cursorX+1);
    lua_pushinteger(L, machine->term->cursorY+1);
    return 2;
}

int term_getSize(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    lua_pushinteger(L, machine->term->width);
    lua_pushinteger(L, machine->term->height);
    return 2;
}

int term_clear(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    craftos_terminal_clear(machine->term, -1, machine->term->activeColors);
    return 0;
}

int term_clearLine(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    if (machine->term->cursorY < 0 || machine->term->cursorY >= machine->term->height) return 0;
    craftos_terminal_clear(machine->term, machine->term->cursorY, machine->term->activeColors);
    return 0;
}

static int log2i(unsigned int n) {
    if (n == 0) return 0;
    int i = 0;
    while (n) {i++; n >>= 1;}
    return i - 1;
}

int term_setTextColor(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    machine->term->activeColors = (machine->term->activeColors & 0xF0) | ((int)log2(luaL_checkinteger(L, 1)) & 0x0F);
    return 0;
}

int term_setBackgroundColor(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    machine->term->activeColors = (machine->term->activeColors & 0x0F) | (((int)log2(luaL_checkinteger(L, 1)) & 0x0F) << 4);
    return 0;
}

int term_isColor(lua_State *L) {
    lua_pushboolean(L, 1);
    return 1;
}

int term_getTextColor(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    lua_pushinteger(L, 1 << (machine->term->activeColors & 0x0F));
    return 1;
}

int term_getBackgroundColor(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    lua_pushinteger(L, 1 << (machine->term->activeColors >> 4));
    return 1;
}

int hexch(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    else return 0;
}

int term_blit(lua_State *L) {
    size_t len, blen, flen;
    int i;
    craftos_machine_t machine = get_comp(L);
    const char * str = luaL_checklstring(L, 1, &len);
    const char * fg = luaL_checklstring(L, 2, &flen);
    const char * bg = luaL_checklstring(L, 3, &blen);
    if (len != flen || flen != blen) {
        lua_pushstring(L, "Arguments must be the same length");
        lua_error(L);
    }
    unsigned char* colors = F.malloc(len);
    for (i = 0; i < len; i++) colors[i] = hexch(fg[i]) | (hexch(bg[i]) << 4);
    craftos_terminal_blit(machine->term, machine->term->cursorX, machine->term->cursorY, (const uint8_t*)str, colors, len);
    F.free(colors);
    machine->term->cursorX += len;
    craftos_terminal_cursor(machine->term, machine->term->canBlink ? machine->term->activeColors & 0xF : -1, machine->term->cursorX, machine->term->cursorY);
    return 0;
}

int term_getPaletteColor(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    int color = log2i(luaL_checkinteger(L, 1)) & 0x0F;
    unsigned int v = machine->term->palette[color];
    lua_pushnumber(L, ((v >> 16) & 0xFF) / 255.0);
    lua_pushnumber(L, ((v >> 8) & 0xFF) / 255.0);
    lua_pushnumber(L, (v & 0xFF) / 255.0);
    return 3;
}

int term_nativePaletteColor(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    int color = log2i(luaL_checkinteger(L, 1)) & 0x0F;
    unsigned int v = craftos_terminal_defaultPalette[color];
    lua_pushnumber(L, ((v >> 16) & 0xFF) / 255.0);
    lua_pushnumber(L, ((v >> 8) & 0xFF) / 255.0);
    lua_pushnumber(L, (v & 0xFF) / 255.0);
    return 3;
}

int term_setPaletteColor(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    int color = log2i(luaL_checkinteger(L, 1)) & 0x0F;
    unsigned int c;
    if (lua_gettop(L) > 2) {
        lua_Number r = luaL_checknumber(L, 2);
        lua_Number g = luaL_checknumber(L, 3);
        lua_Number b = luaL_checknumber(L, 4);
        c = (int)floor(r * 255.0 + 0.5) << 16 | (int)floor(g * 255.0 + 0.5) << 8 | (int)floor(b * 255.0 + 0.5);
    } else {
        c = luaL_checkinteger(L, 2);
    }
    machine->term->palette[color] = c;
    machine->term->paletteChanged = 1;
    return 0;
}

const luaL_Reg term_lib[] = {
    {"write", term_write},
    {"scroll", term_scroll},
    {"getCursorPos", term_getCursorPos},
    {"setCursorPos", term_setCursorPos},
    {"getCursorBlink", term_getCursorBlink},
    {"setCursorBlink", term_setCursorBlink},
    {"getSize", term_getSize},
    {"clear", term_clear},
    {"clearLine", term_clearLine},
    {"setTextColour", term_setTextColor},
    {"setTextColor", term_setTextColor},
    {"setBackgroundColour", term_setBackgroundColor},
    {"setBackgroundColor", term_setBackgroundColor},
    {"isColour", term_isColor},
    {"isColor", term_isColor},
    {"getTextColour", term_getTextColor},
    {"getTextColor", term_getTextColor},
    {"getBackgroundColour", term_getBackgroundColor},
    {"getBackgroundColor", term_getBackgroundColor},
    {"blit", term_blit},
    {"getPaletteColor", term_getPaletteColor},
    {"getPaletteColour", term_getPaletteColor},
    {"setPaletteColor", term_setPaletteColor},
    {"setPaletteColour", term_setPaletteColor},
    {"nativePaletteColor", term_nativePaletteColor},
    {"nativePaletteColour", term_nativePaletteColor},
    {NULL, NULL}
};