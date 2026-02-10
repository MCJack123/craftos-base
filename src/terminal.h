#ifndef TERMINAL_H
#define TERMINAL_H
#include <craftos.h>

extern craftos_terminal_t craftos_terminal_create(unsigned int width, unsigned int height);
extern void craftos_terminal_destroy(craftos_terminal_t term);
extern void craftos_terminal_clear(craftos_terminal_t term, int line, unsigned char colors);
extern void craftos_terminal_scroll(craftos_terminal_t term, int lines, unsigned char colors);
extern void craftos_terminal_blit(craftos_terminal_t term, int x, int y, const unsigned char* text, const unsigned char* colors, int len);
extern void craftos_terminal_cursor(craftos_terminal_t term, char color, int x, int y);

#endif