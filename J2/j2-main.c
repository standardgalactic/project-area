#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "j2.h"

char base_fn[32];
int save_output = 1;
int debug_flag = 0;
int exitStatus = 0;
long maxCycles = 0;

#define LAST_OP the_memory[HERE-1]
#define COMMA(val) the_memory[HERE++] = val

#define TIB_SZ 1024
char tib[TIB_SZ];
char *toIn;

DICT_T words[2048];
WORD numWords = 0;

CELL HERE = 0;
CELL STATE = 0;

// ---------------------------------------------------------------------
char getChar() {
	return (*toIn) ? *(toIn++) : 0;
}

// ---------------------------------------------------------------------
char skipWS() {
	char ch = getChar();
	while (ch) {
		if (ch > 0x20) {
			return ch;
		} else {
			ch = getChar();
		}
	}
	return ch;
}

// ---------------------------------------------------------------------
int getWord(char *word) {
	char ch = skipWS();
	int len = 0;
	while (ch > 0x20) {
		*(word++) = ch;
		++len;
		ch = getChar();
	}
	*word = 0;
	return len;
}

// ---------------------------------------------------------------------
int isNumber(char *word, CELL *value) {
	int base = 10;
	int isNeg = 0;
	short num = 0;
	*value = 0;

	if ((strlen(word) == 3) && (*word == '\'') && (*(word+2) == '\'')) {
		*value = *(word+1);
		return 1;
	}
	if (*word == '$') { base = 16; ++word; }
	if (*word == '#') { base = 10; ++word; }
	if (*word == '%') { base = 2; ++word; }
	char ch = *(word++);
	while (ch) {
		short chNum = -1;
		if ((base == 2) && ('0' <= ch) && (ch <= '1')) { chNum = ch - '0'; }
		if ((base == 10) && ('0' <= ch) && (ch <= '9')) { chNum = ch - '0'; }
		if (base == 16) {
			if (('0' <= ch) && (ch <= '9')) { chNum = ch - '0'; }
			if (('A' <= ch) && (ch <= 'F')) { chNum = ch - 'A' + 10; }
			if (('a' <= ch) && (ch <= 'f')) { chNum = ch - 'a' + 10; }
		}
		if (chNum < 0) { return 0; }

		num = (num*base) + chNum;
		ch = *(word++);
	}
	if (isNeg) { num = -num; }
	*value = (WORD)num;
	return 1;
}

// ---------------------------------------------------------------------
void defineWord(char *name) {
	DICT_T *p = &words[numWords++];
	strcpy(p->name, name);
	p->xt = HERE;
	p->flags = 0;
	p->len = 0;
	if (debug_flag) writePort_StringF("\nDefined (%d) [%s] at addr %02X", numWords-1, name, HERE);
}

// ---------------------------------------------------------------------
DICT_T *findWord(char *word) {
	for (int i = numWords-1; i >= 0; i--) {
		if (strcmp(words[i].name, word) == 0) {
			return &words[i];
		}
	}
	return NULL;
}

// ---------------------------------------------------------------------
void parseWord(char *word) {
	// if (debug_flag) writePort_StringF("\n[%s] (HERE=%d), LAST_OP=%04X", word, HERE, LAST_OP);
	CELL num = 0;
	CELL op = LAST_OP;
	if (isNumber(word, &num)) {
		if ((num & 0x8000) == 0) {
			op = MAKE_LIT(num);
			COMMA(op);
		} else {
			// For numbers larger than 0x7FFF
			num = ~num;
			COMMA(MAKE_LIT(num));
			COMMA(MAKE_ALU(0x0030));
		}
		return;
	}
	DICT_T *w = findWord(word);
	if (w) {
		// INLINE is only for words where the last operation is ALU
		if (w->flags & 0x01) {
			WORD a = w->xt;
			for (WORD i = 0; i < w->len; i++) {
				COMMA(the_memory[a++]);
			}
			// Clear the R->PC and --RSP bits
			LAST_OP &= ~(bitRtoPC|bitDecRSP);
		} else if (w->flags & 0x08) {
			// MACRO
			COMMA(w->macroVal);
		} else {
			op = MAKE_CALL(w->xt);
			COMMA(op);
		}
		return;
	}
	if (strcmp(word, ":") == 0) {
		getWord(word);
		defineWord(word);
		STATE = 1;
		return;
	}
	if (strcmp(word, "MACRO") == 0) {
		DICT_T *w = &words[numWords-1];
		if (w->len == 1) {
			op = the_memory[--HERE]; 
			the_memory[HERE] = 0; 
			op &= ~(bitRtoPC|bitDecRSP);
			w->xt = 0x0000;
			w->flags = 0x08;
			w->macroVal = op;
			if (debug_flag) { writePort_StringF("MACRO: [%s], val=%02X", w->name, w->macroVal); }
		} else {
			writePort_StringF("\nWARN: [%s] length must be 1 for MACRO", w->name);
		}
		return;
	}
	if (strcmp(word, "INLINE") == 0) {
		if ((LAST_OP & 0xE000) == opALU) {
			words[numWords-1].flags |= 1;
		}
		return;
	}
	if (strcmp(word, ";") == 0) {
		STATE = 0;
		words[numWords-1].len = (HERE - words[numWords-1].xt);
		// Change last operation to JMP if CALL
		if ((LAST_OP & 0xE000) == opCALL) {
			LAST_OP = (LAST_OP & 0x1FFF) | opJMP;
			if (debug_flag) writePort_StringF("\nchanged op at %d to JMP", HERE-1);
			return;
		}
		int canAddRet = 1;
		if ((LAST_OP & 0xE000) != opALU) canAddRet = 0; // not ALU
		if ((LAST_OP & bitRtoPC) != 0)   canAddRet = 0; // R->PC already set
		if ((LAST_OP & bitIncRSP) != 0)  canAddRet = 0; // R++ set
		if ((LAST_OP & bitDecRSP) != 0)  canAddRet = 0; // R-- already set
		if (canAddRet) {
			LAST_OP |= bitRtoPC;
			LAST_OP |= bitDecRSP;
			if (debug_flag) writePort_StringF("\nAdded %04X to ALU op at %d", (bitDecDSP|bitRtoPC), HERE-1);
			return;
		}
		// cannot include in previous op :(
		op = opALU;
		op |= bitRtoPC;
		op |= bitDecRSP;
		COMMA(op);
		words[numWords-1].len = (HERE - words[numWords-1].xt);
		return;
	}
	if (strcmp(word, "alu") == 0) {
		if (debug_flag) writePort_StringF(" putting ALU %04X to [%d]", HERE);
		op = MAKE_ALU(pop());
		COMMA(op);
		return;
	}
	if (strcmp(word, "T") == 0) {
		push(aluTgetsT);
		return;
	}
	if (strcmp(word, "N") == 0) {
		push(aluTgetsN);
		return;
	}
	if (strcmp(word, "rT") == 0) {
		push(aluTgetsR);
		return;
	}
	if (strcmp(word, "T+N") == 0) {
		push(aluTplusN);
		return;
	}
	if (strcmp(word, "T&N") == 0) {
		push(aluTandN);
		return;
	}
	if (strcmp(word, "T|N") == 0) {
		push(aluTorN);
		return;
	}
	if (strcmp(word, "T^N") == 0) {
		push(aluTxorN);
		return;
	}
	if (strcmp(word, "~T") == 0) {
		push(aluNotT);
		return;
	}
	if (strcmp(word, "N==T") == 0) {
		push(aluTeqN);
		return;
	}
	if (strcmp(word, "N<T") == 0) {
		push(aluTltN);
		return;
	}
	if (strcmp(word, "N>>T") == 0) {
		push(aluSHR);
		return;
	}
	if (strcmp(word, "N<<T") == 0) {
		push(aluSHL);
		return;
	}
	if (strcmp(word, "T-1") == 0) {
		push(aluDecT);
		return;
	}
	if (strcmp(word, "[T]") == 0) {
		push(aluFetch);
		return;
	}
	if (strcmp(word, "dsp") == 0) {
		push(aluDepth);
		return;
	}
	if (strcmp(word, "Nu<T") == 0) {
		push(aluNuLtT);
		return;
	}
	if (strcmp(word, "N->[T]") == 0) {
		T |= bitStore;
		return;
	}
	if (strcmp(word, "R->PC") == 0) {
		T |= bitRtoPC;
		return;
	}
	if (strcmp(word, "T->N") == 0) {
		T |= bitTtoN;
		return;
	}
	if (strcmp(word, "T->R") == 0) {
		T |= bitTtoR;
		return;
	}
	if (strcmp(word, "r+1") == 0) {
		T |= bitIncRSP;
		return;
	}
	if (strcmp(word, "r-1") == 0) {
		T |= bitDecRSP;
		return;
	}
	if (strcmp(word, "d+1") == 0) {
		T |= bitIncDSP;
		return;
	}
	if (strcmp(word, "d-1") == 0) {
		T |= bitDecDSP;
		return;
	}
	if (strcmp(word, ">r") == 0) {
		op = MAKE_ALU(aluTgetsN|bitTtoR|bitIncRSP|bitDecDSP);
		COMMA(op);
		return;
	}
	if (strcmp(word, "r>") == 0) {
		op = MAKE_ALU(aluTgetsR|bitDecRSP|bitIncDSP);
		COMMA(op);
		return;
	}
	if (strcmp(word, "r@") == 0) {
		op = MAKE_ALU(aluTgetsR|bitIncDSP);
		COMMA(op);
		return;
	}
	if (strcmp(word, "XXX") == 0) {
		// do something ...
		return;
	}
	writePort_StringF("\nERROR: unknown word: [%s]\n", word);
	exitStatus = 1;
}

// ---------------------------------------------------------------------
void parseLine(char *line) {
	char word[32];
	toIn = line;
	while (1) {
		int len = getWord(word);
		// writePort_StringF("[%s]", word);
		if (len) {
			if (strcmp(word, "\\") == 0) { return; }
			if (strcmp(word, "//") == 0) { return; }
			parseWord(word);
		} else {
			return;
		}
	}
}

// ---------------------------------------------------------------------
void doCompile(FILE *fp) {
	char *inSave = toIn;
	while (1) {
		if (fgets(tib, TIB_SZ, fp) == tib) {
			parseLine(tib);
		} else {
			toIn = inSave;
			return;
		}
	}
}

// ---------------------------------------------------------------------
int load(char *base) {
	char fn[32];
	sprintf(fn, "%s.src", base_fn);
	FILE *fp = fopen(fn, "rt");
	if (fp) {
		doCompile(fp);
		fclose(fp);
	} else {
		writePort_StringF("ERROR: unable to open '%s'\n", fn);
	}
	return (fp) ? 1 : 0;
}

// ---------------------------------------------------------------------
void dumpStack(int sp, CELL* stk) {
	writePort(emitPort, '(');
	for (int i = 1; i <= sp; i++) {
		writePort(dotPort, stk[i]);
	}
	writePort_String(" )");
}

void disALU(WORD IR, char* output) {
	strcat(output, "\n    ");

	WORD aluOp = IR & 0x0F00;
	if (aluOp == aluTgetsT) { strcat(output, "T"); }
	if (aluOp == aluTgetsN) { strcat(output, "N"); }
	if (aluOp == aluTgetsR) { strcat(output, "R"); }
	if (aluOp == aluTplusN) { strcat(output, "T+N"); }
	if (aluOp == aluTandN) { strcat(output, "T&N"); }
	if (aluOp == aluTorN) { strcat(output, "T|N"); }
	if (aluOp == aluTxorN) { strcat(output, "T^N"); }
	if (aluOp == aluNotT) { strcat(output, "~T"); }
	if (aluOp == aluTeqN) { strcat(output, "N==T"); }
	if (aluOp == aluTltN) { strcat(output, "N<T"); }
	if (aluOp == aluSHR) { strcat(output, "N>>T"); }
	if (aluOp == aluDecT) { strcat(output, "T-1"); }
	if (aluOp == aluFetch) { strcat(output, "[T]"); }
	if (aluOp == aluSHL) { strcat(output, "N<<T"); }
	if (aluOp == aluDepth) { strcat(output, "dsp"); }
	if (aluOp == aluNuLtT) { strcat(output, "Nu<T"); }

	if (IR & bitRtoPC) { strcat(output, "   R->PC"); }
	if (IR & bitStore) { strcat(output, "   N->[T]"); }
	if (IR & bitIncRSP) { strcat(output, "   r+1"); }
	if (IR & bitDecRSP) { strcat(output, "   r-1"); }
	if (IR & bitTtoR) { strcat(output, "   T->R"); }
	if (IR & bitTtoN) { strcat(output, "   T->N"); }
	if (IR & bitUnused) { strcat(output, "   (unused)"); }
	if (IR & bitDecDSP) { strcat(output, "   d-1"); }
	if (IR & bitIncDSP) { strcat(output, "   d+1"); }
}

void disIR(WORD IR, char* output) {
	char buf[128];
	WORD arg;
	sprintf(buf, "Unknown IR %04X", IR);
	if ((IR & opLIT) == opLIT) {
		arg = (IR & 0x7FFF);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "LIT", arg, arg);
	}
	else if ((IR & INSTR_MASK) == opJMP) {
		arg = (IR & ADDR_MASK);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "JMP", arg, arg);
	}
	else if ((IR & INSTR_MASK) == opJMPZ) {
		arg = (IR & ADDR_MASK);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "JMPZ", arg, arg);
	}
	else if ((IR & INSTR_MASK) == opCALL) {
		arg = (IR & ADDR_MASK);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "CALL", arg, arg);
	}
	else if ((IR & INSTR_MASK) == opALU) {
		arg = (IR & ADDR_MASK);
		sprintf(buf, "%-8s %-5d   # (0x%04X)", "ALU", arg, arg);
		disALU(IR, buf);
	}
	if (output) {
		strcpy(output, buf);
	}
	else {
		writePort_String("\n");
		writePort_String(buf);
	}
}

// ---------------------------------------------------------------------
void doDisassemble(int toFile) {
	FILE *fp = NULL;
	if (toFile) {
		char fn[32];
		sprintf(fn, "%s.lst", base_fn);
		fp = fopen(fn, "wt");
		if (!fp) {
			writePort_StringF("\nUnable to create listing file '%s'.", fn);
			return;
		}
		fprintf(fp, "; HERE: 0x%04X (%d)\n", HERE, HERE);
	}

	char buf[256];
	for (int i = 0; i < numWords; i++) {
		DICT_T *p = &words[i];
		sprintf(buf, "; %2d: XT: %04X, Len: %2d, Flags: %02X, MacroVal: %02X, Name: %s\n", i,
			p->xt, p->len, p->flags, p->macroVal, p->name);
		(fp) ? fprintf(fp, "%s", buf) : writePort_String(buf);
	}

	for (int i = 0; i < HERE; i++) {
		WORD ir = the_memory[i];
		disIR(ir, buf);
		if (fp) {
			fprintf(fp, "\n%04X: %04X    ", i, ir);
			fprintf(fp, "%s", buf);
		} else {
			writePort_StringF("\n%04X: %04X    ", i, ir);
			writePort_String(buf);
		}
	}
	if (fp) { fclose(fp); }
}

// ---------------------------------------------------------------------
void saveImage() {
	char fn[32];
	sprintf(fn, "%s.bin", base_fn);
	FILE *fp = fopen(fn, "wb");
	if (fp) {
		fwrite(the_memory, 1, MEM_SZ, fp);
		fclose(fp);
	} else {
		writePort_StringF(" ERROR: unable to open file '%s'", fn);
	}
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
void parse_arg(char *arg) 
{
	if (*arg == 'f') strcpy(base_fn, arg+2);
	if (*arg == 't') save_output = 0;
	if (*arg == 'd') debug_flag  = 1;
	if (*arg == 'c') {
		char buf[24];
		strcpy(buf, arg+2);
		maxCycles = atol(buf);
	}

	if (*arg == '?') {
		writePort_StringF("usage: j2 [options]\n");
		writePort_StringF("\t -f:baseFn     (default: 'j2')\n");
		writePort_StringF("\t -t     (temp:  default: false)\n");
		writePort_StringF("\t -d     (debug: default: false)\n");
		writePort_StringF("\t -c:maxCycles  (default: 0)\n");
		writePort_StringF("\nNotes ...");
		writePort_StringF("\n\n    -f:baseFn defines the base filename for the files in the working set.");
		writePort_StringF(  "\n    -t identfies that j2 should not write a .LST or .BIN file");
		writePort_StringF(  "\n    -c limits the number of CPU cycles, 0 => unlimited");

		exit(0);
	}
}

// ---------------------------------------------------------------------
int main (int argc, char **argv)
{
	strcpy(base_fn, "j2");

	for (int i = 1; i < argc; i++) {
		char *cp = argv[i];
		if (*cp == '-') { parse_arg(++cp); }
	}

	// setDebugMode(debug_flag);
	j2_init();
	COMMA(MAKE_JMP(0));

	load(base_fn);

	if (numWords) { the_memory[0] = MAKE_JMP(words[numWords-1].xt); }
	if (save_output) { doDisassemble(1); }

	j2_emu(0);

	// if (debug_flag) {
		writePort_String("\ndata stack: ");
		dumpStack(DSP, dstk);
		writePort_String("\nreturn stack: ");
		dumpStack(RSP, rstk);
	// } 

	if (save_output) { saveImage(); }
	return exitStatus;
}
