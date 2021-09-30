#include "ppu.h"

/*
Copyright (c) 2021 Devine Lu Linvega
Copyright (c) 2021 Andrew Alderwick

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

static Uint8 blending[5][16] = {
	{0, 0, 0, 0, 1, 0, 1, 1, 2, 2, 0, 2, 3, 3, 3, 0},
	{0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3},
	{1, 2, 3, 1, 1, 2, 3, 1, 1, 2, 3, 1, 1, 2, 3, 1},
	{2, 3, 1, 2, 2, 3, 1, 2, 2, 3, 1, 2, 2, 3, 1, 2},
	{1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0}};

static Uint16
ppu_row(Ppu *p, Uint16 x, Uint16 y)
{
	return (y % 8) + ((x / 8 + y / 8 * p->width / 8) * 16);
}

static void
ppu_clear(Ppu *p)
{
	int x, y;
	for(y = 0; y < p->height; ++y) {
		for(x = 0; x < p->width; ++x) {
			ppu_write(p, p->bg, x, y, 0);
			ppu_write(p, p->fg, x, y, 0);
		}
	}
}

Uint8
ppu_read(Ppu *p, Uint16 x, Uint16 y)
{
	Uint16 ch1, ch2, row = ppu_row(p, x, y);
	ch1 = (p->fg[row] >> (7 - x % 8)) & 1;
	ch2 = (p->fg[row + 8] >> (7 - x % 8)) & 1;
	if(!ch1 && !ch2) {
		ch1 = (p->bg[row] >> (7 - x % 8)) & 1;
		ch2 = (p->bg[row + 8] >> (7 - x % 8)) & 1;
	}
	return ch1 + (ch2 << 1);
}

void
ppu_write(Ppu *p, Uint8 *layer, Uint16 x, Uint16 y, Uint8 color)
{
	Uint16 row = ppu_row(p, x, y), col = 7 - (x % 8);
	if(x >= p->width || y >= p->height)
		return;
	if(color == 0 || color == 2)
		layer[row] &= ~(1UL << col);
	else
		layer[row] |= 1UL << col;
	if(color == 0 || color == 1)
		layer[row + 8] &= ~(1UL << col);
	else
		layer[row + 8] |= 1UL << col;
}

void
ppu_1bpp(Ppu *p, Uint8 *layer, Uint16 x, Uint16 y, Uint8 *sprite, Uint8 color, Uint8 flipx, Uint8 flipy)
{
	Uint16 v, h;
	for(v = 0; v < 8; v++)
		for(h = 0; h < 8; h++) {
			Uint8 ch1 = (sprite[v] >> (7 - h)) & 0x1;
			if(ch1 || blending[4][color])
				ppu_write(p,
					layer,
					x + (flipx ? 7 - h : h),
					y + (flipy ? 7 - v : v),
					blending[ch1][color]);
		}
}

void
ppu_2bpp(Ppu *p, Uint8 *layer, Uint16 x, Uint16 y, Uint8 *sprite, Uint8 color, Uint8 flipx, Uint8 flipy)
{
	Uint16 v, h;
	for(v = 0; v < 8; v++)
		for(h = 0; h < 8; h++) {
			Uint8 ch1 = ((sprite[v] >> (7 - h)) & 0x1);
			Uint8 ch2 = ((sprite[v + 8] >> (7 - h)) & 0x1);
			Uint8 ch = ch1 + ch2 * 2;
			if(ch || blending[4][color])
				ppu_write(p,
					layer,
					x + (flipx ? 7 - h : h),
					y + (flipy ? 7 - v : v),
					blending[ch][color]);
		}
}

/* output */

int
ppu_set_size(Ppu *p, Uint16 width, Uint16 height)
{
	ppu_clear(p);
	p->width = width;
	p->height = height;
	p->pixels = realloc(p->bg, p->width / 4 * p->height * sizeof(Uint8) * 2);
	p->bg = p->pixels;
	p->fg = p->pixels + (p->width / 4 * p->height * sizeof(Uint8));
	ppu_clear(p);
	return p->bg && p->fg;
}
