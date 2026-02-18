#include <craftos.h>
#include <lua.h>
#include <lauxlib.h>

void craftos_event_http_success(craftos_machine_t machine, const char * url, craftos_http_handle_t handle) {
    
}

void craftos_event_websocket_success(craftos_machine_t machine, const char * url, craftos_http_websocket_t handle) {

}

const luaL_Reg http_lib[] = {
    {NULL, NULL}
};

const luaL_Reg http_lib_ws[] = {
    {NULL, NULL}
};
