# Uxn

A stack-based VM, written in ANSI C.

## Build

```
cc uxn.c -std=c89 -Os -DNDEBUG -g0 -s -Wall -Wno-unknown-pragmas -o uxn
```

## Assembly Syntax

### Write

- `;variable`, set a name to address on the zero-page
- `:label`, set a name to an address

### Read

- `,literal`, get a literal pointer
- `.pointer`, get a raw pointer

### Special

- `@0010`, move to position in the program
- `( comment )`

```
;value ( alloc a zero-page variable )

@0010 ( start at page 1 )

,03 ,02 ADD ,05 EQU 
,there ROT JMC

:here
	( when not equal )
	,ee
	BRK

:there
	( when is equal )
	,ff
	BRK
```

## Mission

### Assembler

- Catch overflow/underflow
- Constants
- Jumps should be relative

### CPU

- Pointers/Literals
- A Three-Way Decision Routine(http://www.6502.org/tutorials/compare_instructions.html)
- Print word to stdout
- Draw pixel to screen
- Detect mouse click
- SDL Layer Emulator
- Build PPU
- Add flags..

## Refs

https://code.9front.org/hg/plan9front/file/a7f9946e238f/sys/src/games/nes/cpu.c
http://www.w3group.de/stable_glossar.html
http://www.emulator101.com/6502-addressing-modes.html
http://forth.works/8f0c04f616b6c34496eb2141785b4454
https://justinmeiners.github.io/lc3-vm/