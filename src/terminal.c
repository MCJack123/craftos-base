/**
 * terminal.c
 * craftos-base
 * 
 * This file defines the functions related to terminal rendering.
 * 
 * This file is licensed under the MIT license.
 * Copyright (c) 2018-2026 JackMacWindows.
 */

#include <craftos.h>
#include "types.h"
#include "terminal.h"
#include "font.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

unsigned int craftos_terminal_defaultPalette[16] = {
    0xf0f0f0,
    0xf2b233,
    0xe57fd8,
    0x99b2f2,
    0xdede6c,
    0x7fcc19,
    0xf2b2cc,
    0x4c4c4c,
    0x999999,
    0x4c99b2,
    0xb266e5,
    0x3366cc,
    0x7f664c,
    0x57a64e,
    0xcc4c4c,
    0x111111
};

craftos_terminal_t craftos_terminal_create(unsigned int width, unsigned int height) {
    int y;
    craftos_terminal_t term = F.malloc(sizeof(struct craftos_terminal));
    if (term == NULL) return NULL;
    term->width = width;
    term->height = height;
    term->screen = F.malloc(width * height);
    if (term->screen == NULL) {
        F.free(term);
        return NULL;
    }
    term->colors = F.malloc(width * height);
    if (term->colors == NULL) {
        F.free(term->screen);
        F.free(term);
        return NULL;
    }
    memcpy(term->palette, craftos_terminal_defaultPalette, 64);
    for (y = 0; y < 16; y++) term->pixelPalette[y] = F.convertPixelValue(y, (term->palette[y] >> 16) & 0xFF, (term->palette[y] >> 8) & 0xFF, term->palette[y] & 0xFF);
    term->cursorX = 0;
    term->cursorY = 0;
    term->changed = 0;
    term->blink = 1;
    term->canBlink = 1;
    term->paletteChanged = 0;
    term->cursorColor = 15;
    term->activeColors = 0xF0;
    craftos_terminal_clear(term, -1, 0xF0);
    /* TODO: blink timer */
    return term;
}

void craftos_terminal_destroy(craftos_terminal_t term) {
    F.free(term->colors);
    F.free(term->screen);
    F.free(term);
}

void craftos_terminal_clear(craftos_terminal_t term, int line, unsigned char col) {
    if (line < 0) {
        memset(term->screen, ' ', term->width * term->height);
        memset(term->colors, col, term->width * term->height);
    } else {
        memset(term->screen + line*term->width, ' ', term->width);
        memset(term->colors + line*term->width, col, term->width);
    }
    term->changed = 1;
}

void craftos_terminal_scroll(craftos_terminal_t term, int lines, unsigned char col) {
    if (lines > 0 ? (unsigned)lines >= term->height : (unsigned)-lines >= term->height) {
        /* scrolling more than the height is equivalent to clearing the screen */
        memset(term->screen, ' ', term->height * term->width);
        memset(term->colors, col, term->height * term->width);
    } else if (lines > 0) {
        memmove(term->screen, term->screen + lines * term->width, (term->height - lines) * term->width);
        memset(term->screen + (term->height - lines) * term->width, ' ', lines * term->width);
        memmove(term->colors, term->colors + lines * term->width, (term->height - lines) * term->width);
        memset(term->colors + (term->height - lines) * term->width, col, lines * term->width);
    } else if (lines < 0) {
        memmove(term->screen - lines * term->width, term->screen, (term->height + lines) * term->width);
        memset(term->screen, ' ', -lines * term->width);
        memmove(term->colors - lines * term->width, term->colors, (term->height + lines) * term->width);
        memset(term->colors, col, -lines * term->width);
    }
    term->changed = 1;
}

void craftos_terminal_write(craftos_terminal_t term, int x, int y, const char * text, int len, unsigned char col) {
    if (y < 0 || y >= term->height || x >= term->width) return;
    if (x < 0) {
        text += x;
        len -= x;
        x = 0;
    }
    if (len <= 0) return;
    if (x + len > term->width) len = term->width - x;
    if (len <= 0) return;
    memcpy(term->screen + y*term->width + x, text, len);
    memset(term->colors + y*term->width + x, col, len);
    term->changed = 1;
}

void craftos_terminal_blit(craftos_terminal_t term, int x, int y, const unsigned char* text, const unsigned char* col, int len) {
    if (y < 0 || y >= term->height || x >= term->width) return;
    if (x < 0) {
        text += x;
        col += x;
        len -= x;
        x = 0;
    }
    if (len <= 0) return;
    if (x + len > term->width) len = term->width - x;
    if (len <= 0) return;
    memcpy(term->screen + y*term->width + x, text, len);
    memcpy(term->colors + y*term->width + x, col, len);
    term->changed = 1;
}

void craftos_terminal_cursor(craftos_terminal_t term, char color, int x, int y) {
    term->cursorColor = color;
    term->cursorX = x;
    term->cursorY = y;
    if (color < 0) term->blink = term->canBlink = 0;
    term->changed = 1;
}

void craftos_terminal_render(craftos_terminal_t term, void * framebuffer, size_t stride, int depth, int scaleX, int scaleY) {
    unsigned int y, x, cp, v;
    void * line;
    unsigned char c;
    int cx = term->cursorX;
    int blink = fmod(F.timestamp(), 1000.0) >= 500.0;
    switch (depth) {
        case 4: case 8: case 16: case 24: case 32: break;
        default: return;
    }
    if (term->canBlink && blink != term->blink) {
        term->blink = blink;
        term->changed = 1;
    }
    if (term->paletteChanged) {
        term->paletteChanged = 0;
        for (y = 0; y < 16; y++) term->pixelPalette[y] = F.convertPixelValue(y, (term->palette[y] >> 16) & 0xFF, (term->palette[y] >> 8) & 0xFF, term->palette[y] & 0xFF);
    }
    if (term->changed) {
        term->changed = 0;
        for (y = 0; y < term->height * 9 * scaleY; y++) {
            line = framebuffer + y*stride;
            for (x = 0; x < term->width * 6 * scaleX; x++) {
                cp = (y / 9 / scaleY) * term->width + (x / 6 / scaleX);
                c = term->screen[cp];
                v = font_data[((c >> 4) * 9 + ((y % (9 * scaleY)) / scaleY)) * 96 + ((c & 0xF) * 6 + ((x % (6 * scaleX)) / scaleX))] ? term->pixelPalette[term->colors[cp] & 0x0F] : term->pixelPalette[term->colors[cp] >> 4];
                switch (depth) {
                    case 4:
                        if (x % 2) ((unsigned char *)line)[x / 2] |= (v & 0xF);
                        else ((unsigned char *)line)[x / 2] = (v & 0xF) << 4;
                        break;
                    case 8: ((unsigned char *)line)[x] = (v & 0xFF); break;
                    case 16: ((unsigned short *)line)[x] = (v & 0xFFFF); break;
                    case 24:
                        ((unsigned char *)line)[x * 3] = v & 0xFF;
                        ((unsigned char *)line)[x * 3 + 1] = (v & 0xFF00) >> 8;
                        ((unsigned char *)line)[x * 3 + 2] = (v & 0xFF0000) >> 16;
                        break;
                    case 32: ((unsigned int *)line)[x] = v; break;
                }
            }
            if (term->blink && y / scaleY == term->cursorY * 9 + 7 && cx >= 0 && cx < term->width * 6 * scaleX) {
                v = term->pixelPalette[term->cursorColor];
                for (x = cx * 6 * scaleX; x < (cx + 1) * 6 * scaleX - scaleX; x++) {
                    switch (depth) {
                        case 4:
                            if (x % 2) ((unsigned char *)line)[x / 2] |= (v & 0xF);
                            else ((unsigned char *)line)[x / 2] = (v & 0xF) << 4;
                            break;
                        case 8: ((unsigned char *)line)[x] = (v & 0xFF); break;
                        case 16: ((unsigned short *)line)[x] = (v & 0xFFFF); break;
                        case 24:
                            ((unsigned char *)line)[x * 3] = v & 0xFF;
                            ((unsigned char *)line)[x * 3 + 1] = (v & 0xFF00) >> 8;
                            ((unsigned char *)line)[x * 3 + 2] = (v & 0xFF0000) >> 16;
                            break;
                        case 32: ((unsigned int *)line)[x] = v; break;
                    }
                }
            }
        }
    }
}

void craftos_terminal_render_vga4p(craftos_terminal_t term, void * framebuffer, size_t stride, size_t height) {

}
