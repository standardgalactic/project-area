// MINT - A Minimal Interpreter - for details, see https://github.com/monsonite/MINT

#include "config.h"
#include <stdio.h>
#include <stdarg.h>

#define CELL        long
#define UCELL       unsigned CELL
#define CELL_SZ     sizeof(CELL)
#define ushort      unsigned short
#define byte        unsigned char
typedef byte *addr;

#define INDEX      reg[8]
#define T          dstack[dsp]
#define N          dstack[dsp-1]
#define R          rstack[dsp]
#define DROP1      pop()
#define DROP2      pop(); pop()
#define BetweenI(n, x, y) (((x) <= (n)) && ((n) <= (y)))
#define isReg(n) ((0 <= (n)) && ((n) < NUM_REGS))
#define isFunc(n) ((0 <= (n)) && ((n) < NUM_FUNCS))
#define ABS(x) ((x < 0) ? -x : x)

typedef struct {
    addr start;
    CELL from;
    CELL to;
    addr end;
} LOOP_ENTRY_T;

extern byte isBye;
extern byte isError;
extern addr HERE;
extern CELL dstack[];
extern ushort dsp;

extern void vmInit();
extern CELL pop();
extern void push(CELL);
extern addr run(addr);
extern addr doCustom(byte, addr);
extern void printChar(const char);
extern void printString(const char*);
extern void printStringF(const char*, ...);
extern void dumpStack();
extern CELL getSeed();
