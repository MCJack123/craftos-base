#include <craftos.h>
#include <SDL3/SDL.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

static time_t start;
static const SDL_PixelFormatDetails * pixelFormat;

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

static const struct craftos_func funcs = {
    timestamp,
    convertPixelValue,
    startTimer,
    cancelTimer,
    NULL
};

int main() {
    start = time(NULL);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if (craftos_init(&funcs) != 0) return 1;
    SDL_Window * win = SDL_CreateWindow("CraftOS Terminal", 620, 350, 0);
    if (win == NULL) {
        SDL_Quit();
        return 2;
    }
    SDL_Surface * surf = SDL_GetWindowSurface(win);
    pixelFormat = SDL_GetPixelFormatDetails(surf->format);
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
    craftos_machine_t machine = craftos_machine_create(&config);
    if (machine == NULL) {
        free(bios);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 4;
    }
    craftos_machine_mount_real(machine, "/usr/share/craftos/rom", "rom", 1);
    for (;;) {
        SDL_Event ev;
        craftos_status_t status = craftos_machine_run(machine);
        if (status == CRAFTOS_MACHINE_STATUS_SHUTDOWN) break;
        else if (status == CRAFTOS_MACHINE_STATUS_ERROR) {
            craftos_terminal_render(machine->term, surf->pixels + 4 * surf->pitch + 4 * pixelFormat->bytes_per_pixel, surf->pitch, pixelFormat->bits_per_pixel, 2, 2);
            SDL_UpdateWindowSurface(win);
            for (;;) if (SDL_WaitEvent(&ev) && ev.type == SDL_EVENT_QUIT) break;
            break;
        } else if (status == CRAFTOS_MACHINE_STATUS_YIELD) {
            craftos_terminal_render(machine->term, surf->pixels + 4 * surf->pitch + 4 * pixelFormat->bytes_per_pixel, surf->pitch, pixelFormat->bits_per_pixel, 2, 2);
            SDL_UpdateWindowSurface(win);
            for (;;) if (SDL_WaitEvent(&ev) && ev.type == SDL_EVENT_QUIT) break;
            break;
        }
    }
    craftos_machine_destroy(machine);
    free(bios);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
