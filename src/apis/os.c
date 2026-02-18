#include "../types.h"
#include <math.h>
#include <string.h>
#include <time.h>
#include <lua.h>
#include <lauxlib.h>

static int os_getComputerID(lua_State *L) {
    lua_pushinteger(L, get_comp(L)->id);
    return 1;
}

static int os_getComputerLabel(lua_State *L) {
    craftos_machine_t machine = get_comp(L);
    if (machine->label == NULL) return 0;
    lua_pushstring(L, machine->label);
    return 1;
}

static int os_setComputerLabel(lua_State *L) {
    craftos_machine_t comp = get_comp(L);
    if (comp->label != NULL) {
        F.free(comp->label);
        comp->label = NULL;
    }
    if (!lua_isnoneornil(L, 1)) {
        const char * label = luaL_checkstring(L, 1);
        comp->label = F.malloc(strlen(label) + 1);
        strcpy(comp->label, label);
    }
    if (F.setComputerLabel != NULL) F.setComputerLabel(comp->label, comp);
    return 0;
}

static int os_queueEvent(lua_State *L) {
    craftos_machine_t computer = get_comp(L);
    luaL_checkstring(L, 1);
    if (!lua_checkstack(computer->eventQueue, 1)) luaL_error(L, "Could not allocate space for event");
    lua_State *param = lua_newthread(computer->eventQueue);
    const int count = lua_gettop(L);
    lua_checkstack(param, count);
    lua_xmove(L, param, count);
    return 0;
}

static int os_clock(lua_State *L) {
    craftos_machine_t computer = get_comp(L);
    lua_pushnumber(L, (F.timestamp() - computer->system_start) / 1000.0);
    return 1;
}

int os_startTimer(lua_State *L) {
    craftos_machine_t computer = get_comp(L);
    lua_Number _time = luaL_checknumber(L, 1);
    if (_time < 0.001) _time = 0.001;
    if (F.startTimer != NULL)
        lua_pushinteger(L, F.startTimer(_time * 1000, computer));
    else {
        /* TODO: implement software timers */
    }
    return 1;
}

int os_cancelTimer(lua_State *L) {
    craftos_machine_t computer = get_comp(L);
    const int id = luaL_checkinteger(L, 1);
    struct craftos_alarm_list ** alarm = &computer->alarms;
    if (F.cancelTimer != NULL) {
        for (; *alarm; alarm = &(*alarm)->next)
            if ((*alarm)->id == id) return 0;
        F.cancelTimer(id, computer);
    } else {
        /* TODO: implement software timers */
    }
    return 0;
}

static int getfield(lua_State *L, const char *key, int d) {
    int res;
    lua_getfield(L, -1, key);
    if (lua_isnumber(L, -1))
        res = (int)lua_tointeger(L, -1);
    else {
        if (d < 0)
            return luaL_error(L, "field " LUA_QS " missing in date table", key);
        res = d;
    }
    lua_pop(L, 1);
    return res;
}

static int getboolfield(lua_State *L, const char *key) {
    lua_getfield(L, -1, key);
    const int res = lua_isnil(L, -1) ? -1 : lua_toboolean(L, -1);
    lua_pop(L, 1);
    return res;
}

static int os_time(lua_State *L) {
    if (lua_istable(L, 1)) {
        struct tm ts;
        lua_settop(L, 1);  /* make sure table is at the top */
        ts.tm_sec = getfield(L, "sec", 0);
        ts.tm_min = getfield(L, "min", 0);
        ts.tm_hour = getfield(L, "hour", 12);
        ts.tm_mday = getfield(L, "day", -1);
        ts.tm_mon = getfield(L, "month", -1) - 1;
        ts.tm_year = getfield(L, "year", -1) - 1900;
        ts.tm_isdst = getboolfield(L, "isdst");
        lua_pushinteger(L, mktime(&ts));
        return 1;
    }
    const char * typ = luaL_optstring(L, 1, "ingame");
    /* lowercase? */
    if (strcmp(typ, "ingame") == 0) {
        lua_pushnumber(L, floor(fmod(F.timestamp() - get_comp(L)->system_start + 300000.0, 1200000.0) / 50.0) / 1000.0);
        return 1;
    } else if (strcmp(typ, "utc") != 0 && strcmp(typ, "local") != 0) luaL_error(L, "Unsupported operation");
    time_t t = time(NULL);
    struct tm rightNow;
    if (strcmp(typ, "utc") == 0) rightNow = *gmtime(&t);
    else rightNow = *localtime(&t);
    const int hour = rightNow.tm_hour;
    const int minute = rightNow.tm_min;
    const int second = rightNow.tm_sec;
    const int milli = (int)floor(fmod(F.timestamp(), 1000.0));
    lua_pushnumber(L, (double)hour + ((double)minute / 60.0) + ((double)second / 3600.0) + (milli / 3600000.0));
    return 1;
}

static int os_epoch(lua_State *L) {
    const char * typ = luaL_optstring(L, 1, "ingame");
    /* lowercase? */
    if (strcmp(typ, "utc") == 0) {
        lua_pushnumber(L, F.timestamp());
    } else if (strcmp(typ, "local") == 0) {
        time_t t = time(NULL);
        const time_t utime = mktime(gmtime(&t));
        struct tm * ltime = localtime(&t);
        const double off = mktime(ltime) - utime + (ltime->tm_isdst ? 3600 : 0);
        lua_pushnumber(L, F.timestamp() + (off * 1000LL));
    } else if (strcmp(typ, "ingame") == 0) {
        const double m_time = fmod(F.timestamp() - get_comp(L)->system_start + 300000.0, 1200000.0) / 50000.0;
        const double m_day = (F.timestamp() - get_comp(L)->system_start) / 20 + 1;
        lua_Integer epoch = (lua_Integer)(m_day * 86400000) + (lua_Integer)(m_time * 3600000.0);
        /*if (config.standardsMode) epoch = (lua_Integer)floor(epoch / 200) * 200;*/
        lua_pushinteger(L, epoch);
    } else luaL_error(L, "Unsupported operation");
    return 1;
}

static int os_day(lua_State *L) {
    const char * typ = luaL_optstring(L, 1, "ingame");
    /* lowercase? */
    time_t t = time(NULL);
    if (strcmp(typ, "ingame") == 0) {
        lua_pushnumber(L, (F.timestamp() - get_comp(L)->system_start) / 20 + 1);
        return 1;
    } else if (strcmp(typ, "local") == 0) t = mktime(localtime(&t));
    else if (strcmp(typ, "utc") != 0) luaL_error(L, "Unsupported operation");
    lua_pushinteger(L, t / (60 * 60 * 24));
    return 1;
}

static int os_setAlarm(lua_State *L) {
    const double time = luaL_checknumber(L, 1);
    if (time < 0.0 || time >= 24.0) luaL_error(L, "Number out of range");
    craftos_machine_t computer = get_comp(L);
    const double current_time = floor(fmod(F.timestamp() - computer->system_start + 300000.0, 1200000.0) / 50.0) / 1000.0;
    double delta_time;
    if (time >= current_time) delta_time = time - current_time;
    else delta_time = (time + 24.0) - current_time;
    double real_time = (double)(delta_time * 50000.0);
    int id;
    if (F.startTimer != NULL)
        id = F.startTimer(real_time * 1000, computer);
    else {
        /* TODO: implement software timers */
    }
    struct craftos_alarm_list * alarm = F.malloc(sizeof(struct craftos_alarm_list));
    alarm->next = computer->alarms;
    alarm->id = id;
    computer->alarms = alarm;
    return 1;
}

static int os_cancelAlarm(lua_State *L) {
    craftos_machine_t computer = get_comp(L);
    const int id = luaL_checkinteger(L, 1);
    struct craftos_alarm_list ** alarm = &computer->alarms;
    if (F.cancelTimer != NULL) {
        for (; *alarm; alarm = &(*alarm)->next) {
            if ((*alarm)->id == id) {
                struct craftos_alarm_list * a = *alarm;
                *alarm = a->next;
                F.free(a);
                F.cancelTimer(id, computer);
                return 0;
            }
        }
    } else {
        /* TODO: implement software timers */
    }
    return 0;   
}

static int os_shutdown(lua_State *L) {
    get_comp(L)->running = CRAFTOS_MACHINE_STATUS_SHUTDOWN;
    return 0;
}

static int os_reboot(lua_State *L) {
    get_comp(L)->running = CRAFTOS_MACHINE_STATUS_RESTART;
    return 0;
}

const luaL_Reg os_lib[] = {
    {"getComputerID", os_getComputerID},
    {"computerID", os_getComputerID},
    {"getComputerLabel", os_getComputerLabel},
    {"computerLabel", os_getComputerLabel},
    {"setComputerLabel", os_setComputerLabel},
    {"queueEvent", os_queueEvent},
    {"clock", os_clock},
    {"startTimer", os_startTimer},
    {"cancelTimer", os_cancelTimer},
    {"time", os_time},
    {"epoch", os_epoch},
    {"day", os_day},
    {"setAlarm", os_setAlarm},
    {"cancelAlarm", os_cancelAlarm},
    {"shutdown", os_shutdown},
    {"reboot", os_reboot},
    {NULL, NULL}
};
