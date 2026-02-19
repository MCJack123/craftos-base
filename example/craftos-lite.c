#include <craftos.h>
#include <SDL3/SDL.h>
#include <curl/curl.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

static time_t start;
static const SDL_PixelFormatDetails * pixelFormat;
static craftos_machine_t machine;
static SDL_Window * win;
static SDL_Surface * surf;

static const struct {SDL_Keycode key; unsigned char k;} keymap[] = {
    {0, 1},
    {SDLK_1, 2},
    {SDLK_2, 3},
    {SDLK_3, 4},
    {SDLK_4, 5},
    {SDLK_5, 6},
    {SDLK_6, 7},
    {SDLK_7, 8},
    {SDLK_8, 9},
    {SDLK_9, 10},
    {SDLK_0, 11},
    {SDLK_MINUS, 12},
    {SDLK_EQUALS, 13},
    {SDLK_BACKSPACE, 14},
    {SDLK_TAB, 15},
    {SDLK_Q, 16},
    {SDLK_W, 17},
    {SDLK_E, 18},
    {SDLK_R, 19},
    {SDLK_T, 20},
    {SDLK_Y, 21},
    {SDLK_U, 22},
    {SDLK_I, 23},
    {SDLK_O, 24},
    {SDLK_P, 25},
    {SDLK_LEFTBRACKET, 26},
    {SDLK_RIGHTBRACKET, 27},
    {SDLK_RETURN, 28},
    {SDLK_LCTRL, 29},
    {SDLK_A, 30},
    {SDLK_S, 31},
    {SDLK_D, 32},
    {SDLK_F, 33},
    {SDLK_G, 34},
    {SDLK_H, 35},
    {SDLK_J, 36},
    {SDLK_K, 37},
    {SDLK_L, 38},
    {SDLK_SEMICOLON, 39},
    {SDLK_APOSTROPHE, 40},
    {SDLK_GRAVE, 41},
    {SDLK_LSHIFT, 42},
    {SDLK_BACKSLASH, 43},
    {SDLK_Z, 44},
    {SDLK_X, 45},
    {SDLK_C, 46},
    {SDLK_V, 47},
    {SDLK_B, 48},
    {SDLK_N, 49},
    {SDLK_M, 50},
    {SDLK_COMMA, 51},
    {SDLK_PERIOD, 52},
    {SDLK_SLASH, 53},
    {SDLK_RSHIFT, 54},
    {SDLK_KP_MULTIPLY, 55},
    {SDLK_LALT, 56},
    {SDLK_SPACE, 57},
    {SDLK_CAPSLOCK, 58},
    {SDLK_F1, 59},
    {SDLK_F2, 60},
    {SDLK_F3, 61},
    {SDLK_F4, 62},
    {SDLK_F5, 63},
    {SDLK_F6, 64},
    {SDLK_F7, 65},
    {SDLK_F8, 66},
    {SDLK_F9, 67},
    {SDLK_F10, 68},
    {SDLK_NUMLOCKCLEAR, 69},
    {SDLK_SCROLLLOCK, 70},
    {SDLK_KP_7, 71},
    {SDLK_KP_8, 72},
    {SDLK_KP_9, 73},
    {SDLK_KP_MINUS, 74},
    {SDLK_KP_4, 75},
    {SDLK_KP_5, 76},
    {SDLK_KP_6, 77},
    {SDLK_KP_PLUS, 78},
    {SDLK_KP_1, 79},
    {SDLK_KP_2, 80},
    {SDLK_KP_3, 81},
    {SDLK_KP_0, 82},
    {SDLK_KP_DECIMAL, 83},
    {SDLK_F11, 87},
    {SDLK_F12, 88},
    {SDLK_F13, 100},
    {SDLK_F14, 101},
    {SDLK_F15, 102},
    {SDLK_KP_EQUALS, 141},
    {SDLK_KP_AT, 145},
    {SDLK_KP_COLON, 146},
    {SDLK_STOP, 149},
    {SDLK_KP_ENTER, 156},
    {SDLK_RCTRL, 157},
    {SDLK_KP_COMMA, 179},
    {SDLK_KP_DIVIDE, 181},
    {SDLK_RALT, 184},
    {SDLK_PAUSE, 197},
    {SDLK_HOME, 199},
    {SDLK_UP, 200},
    {SDLK_PAGEUP, 201},
    {SDLK_LEFT, 203},
    {SDLK_RIGHT, 205},
    {SDLK_END, 207},
    {SDLK_DOWN, 208},
    {SDLK_PAGEDOWN, 209},
    {SDLK_INSERT, 210},
    {SDLK_DELETE, 211}
};

static unsigned char getkey(SDL_Keycode key) {
    int i;
    for (i = 0; i < sizeof(keymap) / sizeof(keymap[0]); i++)
        if (keymap[i].key == key) return keymap[i].k;
    return 0;
}

static double timestamp() {
    return start * 1000 + SDL_GetTicks();
}

static unsigned long convertPixelValue(unsigned char index, unsigned char r, unsigned char g, unsigned char b) {
    return SDL_MapRGB(pixelFormat, NULL, r, g, b);
}

static Uint32 timer_cb(void *userdata, SDL_TimerID timerID, Uint32 interval) {
    craftos_event_timer((craftos_machine_t)userdata, timerID);
    return 0;
}

static int startTimer(unsigned long millis, craftos_machine_t machine) {
    return SDL_AddTimer(millis, timer_cb, machine);
}

static void cancelTimer(int id, craftos_machine_t machine) {
    SDL_RemoveTimer(id);
}

typedef struct {
    char *data;
    char *ptr;
    size_t size;
    CURL *curl;
} http_response_t;

static size_t http_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    http_response_t * handle = (http_response_t*)userp;
    size_t realsize = size * nmemb;
    handle->data = realloc(handle->data, handle->size + realsize);
    if (!handle->data) return 0;
    memcpy(handle->data + handle->size, contents, realsize);
    handle->size += realsize;
    return realsize;
}

static int http_request(const char * url, const char * method, const unsigned char * body, size_t body_size, craftos_http_header_t * headers, int redirect, craftos_machine_t machine) {
    CURL *curl = curl_easy_init();
    if (!curl) return 1;
    
    struct curl_slist *header_list = NULL;
    int i;
    
    http_response_t *handle_data = malloc(sizeof(http_response_t));
    handle_data->data = NULL;
    handle_data->size = 0;
    handle_data->curl = curl;
    for (i = 0; headers[i].key != NULL; i++) {
        char header_str[1024];
        snprintf(header_str, sizeof(header_str), "%s: %s", headers[i].key, headers[i].value);
        header_list = curl_slist_append(header_list, header_str);
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
    if (header_list) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
    if (body && body_size > 0) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body_size);
    }
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, redirect ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, handle_data);
    
    CURLcode res = curl_easy_perform(curl);
    
    if (header_list) curl_slist_free_all(header_list);
    
    if (res != CURLE_OK) {
        free(handle_data->data);
        free(handle_data);
        curl_easy_cleanup(curl);
        return 1;
    }
    
    handle_data->ptr = handle_data->data;
    craftos_event_http_success(machine, url, (craftos_http_handle_t)handle_data);
    
    return 0;
}

static int http_handle_close(craftos_http_handle_t _handle, craftos_machine_t machine) {
    http_response_t * handle = (http_response_t*)_handle;
    curl_easy_cleanup(handle->curl);
    free(handle->data);
    free(handle);
    return 0;
}

static size_t http_handle_read(void * buf, size_t size, size_t count, craftos_http_handle_t _handle, craftos_machine_t machine) {
    http_response_t * handle = (http_response_t*)_handle;
    size_t realsize = size * count;
    if (handle->ptr + realsize > handle->data + handle->size)
        realsize = (handle->data + handle->size) - handle->ptr;
    if (realsize == 0) return 0;
    memcpy(buf, handle->ptr, realsize);
    handle->ptr += realsize;
    return realsize / size;
}

static int http_handle_getc(craftos_http_handle_t _handle, craftos_machine_t machine) {
    http_response_t * handle = (http_response_t*)_handle;
    if (handle->ptr >= handle->data + handle->size) return EOF;
    return *handle->ptr++;
}

static long http_handle_tell(craftos_http_handle_t _handle, craftos_machine_t machine) {
    http_response_t * handle = (http_response_t*)_handle;
    return handle->ptr - handle->data;
}

static int http_handle_seek(craftos_http_handle_t _handle, long offset, int origin, craftos_machine_t machine) {
    http_response_t * handle = (http_response_t*)_handle;
    switch (origin) {
        case SEEK_SET:
            if (offset < 0 || offset >= handle->size) return -1;
            handle->ptr = handle->data + offset;
            break;
        case SEEK_CUR:
            if (offset < handle->data - handle->ptr || offset >= handle->size - (handle->ptr - handle->data)) return -1;
            handle->ptr += offset;
            break;
        case SEEK_END:
            if (offset > 0 || -offset >= handle->size) return -1;
            handle->ptr = handle->data + handle->size - offset;
            break;
    }
    return 0;
}

static int http_handle_eof(craftos_http_handle_t _handle, craftos_machine_t machine) {
    http_response_t * handle = (http_response_t*)_handle;
    return handle->ptr >= handle->data + handle->size;
}

static int http_handle_getResponseCode(craftos_http_handle_t _handle, craftos_machine_t machine) {
    http_response_t * handle = (http_response_t*)_handle;
    long code = 0;
    curl_easy_getinfo(handle->curl, CURLINFO_RESPONSE_CODE, &code);
    return code;
}

static void http_handle_getResponseHeader(craftos_http_handle_t _handle, craftos_http_header_t ** header, craftos_machine_t machine) {
    http_response_t * handle = (http_response_t*)_handle;
    *header = curl_easy_nextheader(handle->curl, CURLH_HEADER, -1, (struct curl_header*)*header);
}

static const craftos_func_t funcs = {
    timestamp,
    convertPixelValue,
    startTimer,
    cancelTimer,
    NULL,
    NULL, NULL, 
    NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, 
    NULL, 
    NULL, 
    NULL, NULL, NULL, 
    http_request,
    http_handle_close,
    http_handle_read,
    http_handle_getc,
    http_handle_tell,
    http_handle_seek,
    http_handle_eof,
    http_handle_getResponseCode,
    http_handle_getResponseHeader,
    NULL, NULL, NULL
};

static Uint32 render_cb(void *userdata, SDL_TimerID timerID, Uint32 interval) {
    craftos_terminal_render(machine->term, surf->pixels + 4 * surf->pitch + 4 * pixelFormat->bytes_per_pixel, surf->pitch, pixelFormat->bits_per_pixel, 2, 2);
    return interval;
}

int main() {
    start = time(NULL);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if (craftos_init(&funcs) != 0) return 1;
    win = SDL_CreateWindow("CraftOS Terminal", 620, 350, SDL_WINDOW_INPUT_FOCUS);
    if (win == NULL) {
        SDL_Quit();
        return 2;
    }
    surf = SDL_GetWindowSurface(win);
    pixelFormat = SDL_GetPixelFormatDetails(surf->format);
    SDL_ClearSurface(surf, 17.0 / 255.0, 17.0 / 255.0, 17.0 / 255.0, 1.0);
    FILE* fp = fopen("/usr/share/craftos/bios.lua", "r");
    if (fp == NULL) {
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 3;
    }
    char* bios = NULL;
    size_t sz = 0;
    while (!feof(fp)) {
        bios = realloc(bios, sz + 4096);
        size_t read = fread(bios + sz, 1, 4096, fp);
        sz += read;
        if (read < 4096) bios[sz] = 0;
    }
    fclose(fp);
    craftos_machine_config_t config = {
        0,
        NULL,
        NULL,
        bios,
        51,
        19,
        0,
        "."
    };
    machine = craftos_machine_create(&config);
    if (machine == NULL) {
        free(bios);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 4;
    }
    craftos_machine_mount_real(machine, "/usr/share/craftos/rom", "rom", 1);
    SDL_StartTextInput(win);
    SDL_AddTimer(50, render_cb, NULL);
    for (;;) {
        SDL_Event ev;
        unsigned char k;
        craftos_status_t status = craftos_machine_run(machine);
        SDL_UpdateWindowSurface(win);
        if (status == CRAFTOS_MACHINE_STATUS_SHUTDOWN) break;
        else if (status == CRAFTOS_MACHINE_STATUS_ERROR) {
            for (;;) if (SDL_WaitEvent(&ev) && ev.type == SDL_EVENT_QUIT) break;
            break;
        } else if (status == CRAFTOS_MACHINE_STATUS_YIELD) {
            if (SDL_WaitEventTimeout(&ev, 50)) {
                switch (ev.type) {
                    case SDL_EVENT_QUIT:
                        goto exit;
                    case SDL_EVENT_KEY_DOWN:
                        k = getkey(ev.key.key);
                        if (k) craftos_event_key(machine, k, ev.key.repeat);
                        break;
                    case SDL_EVENT_KEY_UP:
                        k = getkey(ev.key.key);
                        if (k) craftos_event_key_up(machine, k);
                        break;
                    case SDL_EVENT_TEXT_INPUT:
                        craftos_event_char(machine, ev.text.text[0]);
                        break;
                }
            }
        }
    }
exit:
    craftos_machine_destroy(machine);
    free(bios);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
