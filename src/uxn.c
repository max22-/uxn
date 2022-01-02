#include "uxn.h"

/*
Copyright (u) 2021 Devine Lu Linvega
Copyright (u) 2021 Andrew Alderwick

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

#define MODE_SHORT 0x20
#define MODE_RETURN 0x40
#define MODE_KEEP 0x80

#pragma mark - Operations

/* clang-format off */

/* Utilities */
static void   (*uxn_push)(Stack *s, Uint16 a);
static Uint16 (*uxn_pop8)(Stack *s);
static Uint16 (*uxn_pop)(Stack *s);
static void   (*uxn_poke)(Uint8 *m, Uint16 a, Uint16 b);
static Uint16 (*uxn_peek)(Uint8 *m, Uint16 a);
static void   (*uxn_devw)(Device *d, Uint8 a, Uint16 b);
static Uint16 (*uxn_devr)(Device *d, Uint8 a);
static void   (*uxn_warp)(Uxn *u, Uint16 a);
static void   (*uxn_pull)(Uxn *u);
/* byte mode */
static void   push8(Stack *s, Uint16 a) { if(s->ptr == 0xff) { s->error = 2; return; } s->dat[s->ptr++] = a; }
static Uint16 pop8k(Stack *s) { if(s->kptr == 0) { s->error = 1; return 0; } return s->dat[--s->kptr]; }
static Uint16 pop8d(Stack *s) { if(s->ptr == 0) { s->error = 1; return 0; } return s->dat[--s->ptr]; }
static void   poke8(Uint8 *m, Uint16 a, Uint16 b) { m[a] = b; }
static Uint16 peek8(Uint8 *m, Uint16 a) { return m[a]; }
static void   devw8(Device *d, Uint8 a, Uint16 b) { d->dat[a & 0xf] = b; d->deo(d, a & 0x0f); }
static Uint16 devr8(Device *d, Uint8 a) { return d->dei(d, a & 0x0f); }
static void   warp8(Uxn *u, Uint16 a){ u->ram.ptr += (Sint8)a; }
static void   pull8(Uxn *u){ push8(u->src, peek8(u->ram.dat, u->ram.ptr++)); }
/* short mode */
static void   push16(Stack *s, Uint16 a) { push8(s, a >> 8); push8(s, a); }
static Uint16 pop16(Stack *s) { Uint8 a = uxn_pop8(s), b = uxn_pop8(s); return a + (b << 8); }
	   void   poke16(Uint8 *m, Uint16 a, Uint16 b) { poke8(m, a, b >> 8); poke8(m, a + 1, b); }
	   Uint16 peek16(Uint8 *m, Uint16 a) { return (peek8(m, a) << 8) + peek8(m, a + 1); }
static void   devw16(Device *d, Uint8 a, Uint16 b) { devw8(d, a, b >> 8); devw8(d, a + 1, b); }
static Uint16 devr16(Device *d, Uint8 a) { return (devr8(d, a) << 8) + devr8(d, a + 1); }
static void   warp16(Uxn *u, Uint16 a){ u->ram.ptr = a; }
static void   pull16(Uxn *u){ push16(u->src, peek16(u->ram.dat, u->ram.ptr++)); u->ram.ptr++; }

#pragma mark - Core

int
uxn_eval(Uxn *u, Uint16 vec)
{
	Uint8 instr;
	Uint16 a,b,c;
	if(!vec || u->dev[0].dat[0xf])
		return 0;
	u->ram.ptr = vec;
	if(u->wst.ptr > 0xf8) u->wst.ptr = 0xf8;
	while((instr = u->ram.dat[u->ram.ptr++])) {
		/* Return Mode */
		if(instr & MODE_RETURN) {
			u->src = &u->rst; 
			u->dst = &u->wst;
		} else {
			u->src = &u->wst; 
			u->dst = &u->rst;
		}
		/* Keep Mode */
		if(instr & MODE_KEEP) {
			uxn_pop8 = pop8k;
			u->src->kptr = u->src->ptr;
		} else {
			uxn_pop8 = pop8d;
		}
		/* Short Mode */
		if(instr & MODE_SHORT) {
			uxn_push = push16; uxn_pop = pop16;
			uxn_poke = poke16; uxn_peek = peek16;
			uxn_devw = devw16; uxn_devr = devr16;
			uxn_warp = warp16; uxn_pull = pull16;
		} else {
			uxn_push = push8; uxn_pop = uxn_pop8;
			uxn_poke = poke8; uxn_peek = peek8;
			uxn_devw = devw8; uxn_devr = devr8;
			uxn_warp = warp8; uxn_pull = pull8;
		}
		switch(instr & 0x1f){
			/* Stack */
			case 0x00: /* LIT */ uxn_pull(u); break;
			case 0x01: /* INC */ a = uxn_pop(u->src); uxn_push(u->src, a + 1); break;
			case 0x02: /* POP */ uxn_pop(u->src); break;
			case 0x03: /* DUP */ a = uxn_pop(u->src); uxn_push(u->src, a); uxn_push(u->src, a); break;
			case 0x04: /* NIP */ a = uxn_pop(u->src); uxn_pop(u->src); uxn_push(u->src, a); break;
			case 0x05: /* SWP */ a = uxn_pop(u->src), b = uxn_pop(u->src); uxn_push(u->src, a); uxn_push(u->src, b); break;
			case 0x06: /* OVR */ a = uxn_pop(u->src), b = uxn_pop(u->src); uxn_push(u->src, b); uxn_push(u->src, a); uxn_push(u->src, b); break;
			case 0x07: /* ROT */ a = uxn_pop(u->src), b = uxn_pop(u->src), c = uxn_pop(u->src); uxn_push(u->src, b); uxn_push(u->src, a); uxn_push(u->src, c); break;
			/* Logic */
			case 0x08: /* EQU */ a = uxn_pop(u->src), b = uxn_pop(u->src); push8(u->src, b == a); break;
			case 0x09: /* NEQ */ a = uxn_pop(u->src), b = uxn_pop(u->src); push8(u->src, b != a); break;
			case 0x0a: /* GTH */ a = uxn_pop(u->src), b = uxn_pop(u->src); push8(u->src, b > a); break;
			case 0x0b: /* LTH */ a = uxn_pop(u->src), b = uxn_pop(u->src); push8(u->src, b < a); break;
			case 0x0c: /* JMP */ a = uxn_pop(u->src); uxn_warp(u, a); break;
			case 0x0d: /* JCN */ a = uxn_pop(u->src); if(uxn_pop8(u->src)) uxn_warp(u, a); break;
			case 0x0e: /* JSR */ a = uxn_pop(u->src); push16(u->dst, u->ram.ptr); uxn_warp(u, a); break;
			case 0x0f: /* STH */ a = uxn_pop(u->src); uxn_push(u->dst, a); break;
			/* Memory */
			case 0x10: /* LDZ */ a = uxn_pop8(u->src); uxn_push(u->src, uxn_peek(u->ram.dat, a)); break;
			case 0x11: /* STZ */ a = uxn_pop8(u->src); b = uxn_pop(u->src); uxn_poke(u->ram.dat, a, b); break;
			case 0x12: /* LDR */ a = uxn_pop8(u->src); uxn_push(u->src, uxn_peek(u->ram.dat, u->ram.ptr + (Sint8)a)); break;
			case 0x13: /* STR */ a = uxn_pop8(u->src); b = uxn_pop(u->src); uxn_poke(u->ram.dat, u->ram.ptr + (Sint8)a, b); break;
			case 0x14: /* LDA */ a = pop16(u->src); uxn_push(u->src, uxn_peek(u->ram.dat, a)); break;
			case 0x15: /* STA */ a = pop16(u->src); b = uxn_pop(u->src); uxn_poke(u->ram.dat, a, b); break;
			case 0x16: /* DEI */ a = uxn_pop8(u->src); uxn_push(u->src, uxn_devr(&u->dev[a >> 4], a)); break;
			case 0x17: /* DEO */ a = uxn_pop8(u->src); b = uxn_pop(u->src); uxn_devw(&u->dev[a >> 4], a, b); break;
			/* Arithmetic */
			case 0x18: /* ADD */ a = uxn_pop(u->src), b = uxn_pop(u->src); uxn_push(u->src, b + a); break;
			case 0x19: /* SUB */ a = uxn_pop(u->src), b = uxn_pop(u->src); uxn_push(u->src, b - a); break;
			case 0x1a: /* MUL */ a = uxn_pop(u->src), b = uxn_pop(u->src); uxn_push(u->src, (Uint32)b * a); break;
			case 0x1b: /* DIV */ a = uxn_pop(u->src), b = uxn_pop(u->src); if(a == 0) { u->src->error = 3; a = 1; } uxn_push(u->src, b / a); break;
			case 0x1c: /* AND */ a = uxn_pop(u->src), b = uxn_pop(u->src); uxn_push(u->src, b & a); break;
			case 0x1d: /* ORA */ a = uxn_pop(u->src), b = uxn_pop(u->src); uxn_push(u->src, b | a); break;
			case 0x1e: /* EOR */ a = uxn_pop(u->src), b = uxn_pop(u->src); uxn_push(u->src, b ^ a); break;
			case 0x1f: /* SFT */ a = uxn_pop8(u->src), b = uxn_pop(u->src); uxn_push(u->src, b >> (a & 0x0f) << ((a & 0xf0) >> 4)); break;
		}
		if(u->wst.error) return uxn_halt(u, u->wst.error, "Working-stack", instr);
		if(u->rst.error) return uxn_halt(u, u->rst.error, "Return-stack", instr);
	}
	return 1;
}

/* clang-format on */

int
uxn_boot(Uxn *u, Uint8 *memory)
{
	Uint32 i;
	char *cptr = (char *)u;
	for(i = 0; i < sizeof(*u); ++i)
		cptr[i] = 0x00;
	u->ram.dat = memory;
	return 1;
}

Device *
uxn_port(Uxn *u, Uint8 id, Uint8 (*deifn)(Device *d, Uint8 port), void (*deofn)(Device *d, Uint8 port))
{
	Device *d = &u->dev[id];
	d->addr = id * 0x10;
	d->u = u;
	d->mem = u->ram.dat;
	d->dei = deifn;
	d->deo = deofn;
	return d;
}
