#include <stdio.h>

/*
Copyright (c) 2021 Devine Lu Linvega

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

#include "uxn.h"

#pragma mark - Core

int
error(char *msg, const char *err)
{
	printf("Error %s: %s\n", msg, err);
	return 0;
}

void
printstack(Stack *s)
{
	Uint8 x, y;
	for(y = 0; y < 0x08; ++y) {
		for(x = 0; x < 0x08; ++x) {
			Uint8 p = y * 0x08 + x;
			printf(p == s->ptr ? "[%02x]" : " %02x ", s->dat[p]);
		}
		printf("\n");
	}
}

#pragma mark - Devices

void
console_poke(Device *d, Uint8 b0, Uint8 b1)
{
	switch(b0) {
	case 0x08: printf("%c", b1); break;
	case 0x09: printf("0x%02x\n", b1); break;
	case 0x0b: printf("0x%04x\n", (d->dat[0x0a] << 8) + b1); break;
	}
	fflush(stdout);
	(void)d;
	(void)b0;
}

void
file_poke(Device *d, Uint8 b0, Uint8 b1)
{
	Uint8 read = b0 == 0xd;
	if(read || b0 == 0xf) {
		char *name = (char *)&d->mem[mempeek16(d->dat, 0x8)];
		Uint16 result = 0, length = mempeek16(d->dat, 0xa);
		Uint16 offset = mempeek16(d->dat, 0x4);
		Uint16 addr = mempeek16(d->dat, b0 - 1);
		FILE *f = fopen(name, read ? "r" : (offset ? "a" : "w"));
		if(f) {
			if(fseek(f, offset, SEEK_SET) != -1 && (result = read ? fread(&d->mem[addr], 1, length, f) : fwrite(&d->mem[addr], 1, length, f)))
				printf("%s %d bytes, at %04x from %s\n", read ? "Loaded" : "Saved", length, addr, name);
			fclose(f);
		}
		mempoke16(d->dat, 0x2, result);
	}
	(void)b1;
}

void
ppnil(Device *d, Uint8 b0, Uint8 b1)
{
	(void)d;
	(void)b0;
	(void)b1;
}

#pragma mark - Generics

int
start(Uxn *u)
{
	printf("RESET --------\n");
	if(!evaluxn(u, PAGE_PROGRAM))
		return error("Reset", "Failed");
	printstack(&u->wst);
	printf("FRAME --------\n");
	if(!evaluxn(u, PAGE_PROGRAM + 0x08))
		return error("Frame", "Failed");
	printstack(&u->wst);
	return 1;
}

int
main(int argc, char **argv)
{
	Uxn u;

	if(argc < 2)
		return error("Input", "Missing");
	if(!bootuxn(&u))
		return error("Boot", "Failed");
	if(!loaduxn(&u, argv[1]))
		return error("Load", "Failed");

	portuxn(&u, 0x00, "console", console_poke);
	portuxn(&u, 0x01, "empty", ppnil);
	portuxn(&u, 0x02, "empty", ppnil);
	portuxn(&u, 0x03, "empty", ppnil);
	portuxn(&u, 0x04, "empty", ppnil);
	portuxn(&u, 0x05, "empty", ppnil);
	portuxn(&u, 0x06, "file", file_poke);
	start(&u);

	return 0;
}
