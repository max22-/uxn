#include <stdlib.h>

/*
Copyright (c) 2021 Devine Lu Linvega
Copyright (c) 2021 Andrew Alderwick

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

#define FIXED_SIZE 1

#define BG 0
#define FG 1

typedef struct UxnScreen {
	Uint16 palette[4];
	Uint16 width, height;
	Uint8 *pixels;
	Uint8 changed;
} UxnScreen;

extern UxnScreen uxn_screen;

/* this should probably be done differently */
int set_size(Uint16 width, Uint16 height, int is_resize);

void screen_palette(UxnScreen *p, Uint8 *addr);
void screen_resize(UxnScreen *p, Uint16 width, Uint16 height);
void screen_clear(UxnScreen *p, Uint8 layer);
void screen_debug(UxnScreen *p, Uint8 *stack, Uint8 wptr, Uint8 rptr, Uint8 *memory);

Uint8 screen_dei(Device *d, Uint8 port);
void screen_deo(Device *d, Uint8 port);