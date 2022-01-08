#include "../uxn.h"
#include "screen.h"

/*
Copyright (c) 2021 Devine Lu Linvega
Copyright (c) 2021 Andrew Alderwick

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

UxnScreen uxn_screen;

static Uint8 blending[5][16] = {
	{0, 0, 0, 0, 1, 0, 1, 1, 2, 2, 0, 2, 3, 3, 3, 0},
	{0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3},
	{1, 2, 3, 1, 1, 2, 3, 1, 1, 2, 3, 1, 1, 2, 3, 1},
	{2, 3, 1, 2, 2, 3, 1, 2, 2, 3, 1, 2, 2, 3, 1, 2},
	{1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0}};

static Uint8 font[][8] = {
	{0x00, 0x7c, 0x82, 0x82, 0x82, 0x82, 0x82, 0x7c},
	{0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10},
	{0x00, 0x7c, 0x82, 0x02, 0x7c, 0x80, 0x80, 0xfe},
	{0x00, 0x7c, 0x82, 0x02, 0x1c, 0x02, 0x82, 0x7c},
	{0x00, 0x0c, 0x14, 0x24, 0x44, 0x84, 0xfe, 0x04},
	{0x00, 0xfe, 0x80, 0x80, 0x7c, 0x02, 0x82, 0x7c},
	{0x00, 0x7c, 0x82, 0x80, 0xfc, 0x82, 0x82, 0x7c},
	{0x00, 0x7c, 0x82, 0x02, 0x1e, 0x02, 0x02, 0x02},
	{0x00, 0x7c, 0x82, 0x82, 0x7c, 0x82, 0x82, 0x7c},
	{0x00, 0x7c, 0x82, 0x82, 0x7e, 0x02, 0x82, 0x7c},
	{0x00, 0x7c, 0x82, 0x02, 0x7e, 0x82, 0x82, 0x7e},
	{0x00, 0xfc, 0x82, 0x82, 0xfc, 0x82, 0x82, 0xfc},
	{0x00, 0x7c, 0x82, 0x80, 0x80, 0x80, 0x82, 0x7c},
	{0x00, 0xfc, 0x82, 0x82, 0x82, 0x82, 0x82, 0xfc},
	{0x00, 0x7c, 0x82, 0x80, 0xf0, 0x80, 0x82, 0x7c},
	{0x00, 0x7c, 0x82, 0x80, 0xf0, 0x80, 0x80, 0x80}};

static void
screen_write(UxnScreen *p, Uint8 layer, Uint16 x, Uint16 y, Uint8 color)
{
	/* copied from the "old" ppu */
	if(x < p->width && y < p->height) {
		Uint32 row = (x + y * p->width) / 0x2;
		Uint8 shift = (!(x & 0x1) << 2) + (layer << 1);
		Uint8 pix = p->pixels[row];
		Uint8 mask = ~(0x3 << shift);
		Uint8 pixnew = (pix & mask) + (color << shift);
		if(pix != pixnew) {
			p->pixels[row] = pixnew;
			p->changed = 1;
		}
	}
}

static void
screen_blit(UxnScreen *p, Uint8 layer, Uint16 x, Uint16 y, Uint8 *sprite, Uint8 color, Uint8 flipx, Uint8 flipy, Uint8 twobpp)
{
	int v, h, opaque = blending[4][color];
	for(v = 0; v < 8; v++) {
		Uint16 c = sprite[v] | (twobpp ? sprite[v + 8] : 0) << 8;
		for(h = 7; h >= 0; --h, c >>= 1) {
			Uint8 ch = (c & 1) | ((c >> 7) & 2);
			if(opaque || ch)
				screen_write(p,
					layer,
					x + (flipx ? 7 - h : h),
					y + (flipy ? 7 - v : v),
					blending[ch][color]);
		}
	}
}

void
screen_palette(UxnScreen *p, Uint8 *addr)
{
	int i, shift;
	for(i = 0, shift = 4; i < 4; ++i, shift ^= 4) {
		Uint8
			r = (addr[0 + i / 2] >> shift) & 0x0f,
			g = (addr[2 + i / 2] >> shift) & 0x0f,
			b = (addr[4 + i / 2] >> shift) & 0x0f;
		r = (r << 1) | (r & 1);
		g = (g << 2) | (g & 3);
		b = (b << 1) | (b & 1);
		p->palette[i] = r << 11 | g << 5 | b;
	}
	p->changed = 1;
}

void
screen_resize(UxnScreen *p, Uint16 width, Uint16 height)
{
	Uint8 *pixels = realloc(p->pixels, width * height);

	if(pixels) {
		p->pixels = pixels;
		p->width = width;
		p->height = height;
		screen_clear(p, BG);
		screen_clear(p, FG);
	}
}

void
screen_clear(UxnScreen *p, Uint8 layer)
{
	Uint32 i, size = p->width * p->height;
	Uint8 mask = layer ? 0x0f : 0xf0;
	for(i = 0; i < size; i++)
		p->pixels[i] &= mask;
	p->changed = 1;
}

void
screen_debug(UxnScreen *p, Uint8 *stack, Uint8 wptr, Uint8 rptr, Uint8 *memory)
{
	Uint8 i, x, y, b;
	for(i = 0; i < 0x20; i++) {
		x = ((i % 8) * 3 + 1) * 8, y = (i / 8 + 1) * 8, b = stack[i];
		/* working stack */
		screen_blit(p, FG, x, y, font[(b >> 4) & 0xf], 1 + (wptr == i) * 0x7, 0, 0, 0);
		screen_blit(p, FG, x + 8, y, font[b & 0xf], 1 + (wptr == i) * 0x7, 0, 0, 0);
		y = 0x28 + (i / 8 + 1) * 8;
		b = memory[i];
		/* return stack */
		screen_blit(p, FG, x, y, font[(b >> 4) & 0xf], 3, 0, 0, 0);
		screen_blit(p, FG, x + 8, y, font[b & 0xf], 3, 0, 0, 0);
	}
	/* return pointer */
	screen_blit(p, FG, 0x8, y + 0x10, font[(rptr >> 4) & 0xf], 0x2, 0, 0, 0);
	screen_blit(p, FG, 0x10, y + 0x10, font[rptr & 0xf], 0x2, 0, 0, 0);
	/* guides */
	for(x = 0; x < 0x10; x++) {
		screen_write(p, FG, x, p->height / 2, 2);
		screen_write(p, FG, p->width - x, p->height / 2, 2);
		screen_write(p, FG, p->width / 2, p->height - x, 2);
		screen_write(p, FG, p->width / 2, x, 2);
		screen_write(p, FG, p->width / 2 - 0x10 / 2 + x, p->height / 2, 2);
		screen_write(p, FG, p->width / 2, p->height / 2 - 0x10 / 2 + x, 2);
	}
}

/* IO */

Uint8
screen_dei(Device *d, Uint8 port)
{
	switch(port) {
	case 0x2: return uxn_screen.width >> 8;
	case 0x3: return uxn_screen.width;
	case 0x4: return uxn_screen.height >> 8;
	case 0x5: return uxn_screen.height;
	default: return d->dat[port];
	}
}

void
screen_deo(Device *d, Uint8 port)
{
	switch(port) {
	case 0x1: DEVPEEK16(d->vector, 0x0); break;
	case 0x5:
		if(!FIXED_SIZE) {
			Uint16 w, h;
			DEVPEEK16(w, 0x2);
			DEVPEEK16(h, 0x4);
			set_size(w, h, 1);
		}
		break;
	case 0xe: {
		Uint16 x, y;
		Uint8 layer = d->dat[0xe] & 0x40;
		DEVPEEK16(x, 0x8);
		DEVPEEK16(y, 0xa);
		screen_write(&uxn_screen, layer, x, y, d->dat[0xe] & 0x3);
		if(d->dat[0x6] & 0x01) DEVPOKE16(0x8, x + 1); /* auto x+1 */
		if(d->dat[0x6] & 0x02) DEVPOKE16(0xa, y + 1); /* auto y+1 */
		break;
	}
	case 0xf: {
		Uint16 x, y, addr;
		Uint8 twobpp = !!(d->dat[0xf] & 0x80);
		Uint8 layer = (d->dat[0xf] & 0x40) ? FG : BG;
		DEVPEEK16(x, 0x8);
		DEVPEEK16(y, 0xa);
		DEVPEEK16(addr, 0xc);
		screen_blit(&uxn_screen, layer, x, y, &d->mem[addr], d->dat[0xf] & 0xf, d->dat[0xf] & 0x10, d->dat[0xf] & 0x20, twobpp);
		if(d->dat[0x6] & 0x04) DEVPOKE16(0xc, addr + 8 + twobpp * 8); /* auto addr+length */
		if(d->dat[0x6] & 0x01) DEVPOKE16(0x8, x + 8);                 /* auto x+8 */
		if(d->dat[0x6] & 0x02) DEVPOKE16(0xa, y + 8);                 /* auto y+8 */
		break;
	}
	}
}