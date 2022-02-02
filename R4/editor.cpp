// A simple block editor
// NOTE: this editor was inspired by Alain Theroux's editor
//       and it is a shameless reverse-engineering of it.

#include "R4.h"

#ifndef __EDITOR__
void doEditor() { printString("-noEdit-"); }
#else
#define LLEN        64
#define MAX_X       (LLEN-1)
#define MAX_Y       15
#define BLOCK_SZ    (LLEN)*(MAX_Y+1)
#define MAX_CUR     (BLOCK_SZ-1)
int line, off, blkNum;
int cur, isDirty = 0;
char theBlock[BLOCK_SZ];

void saveBlock() {
    char fn[24];
    sprintf(fn, "block-%03d.R4", blkNum);
    FILE* fp = fopen(fn, "wt");
    if (fp) {
        int n = fwrite(theBlock, 1, BLOCK_SZ, fp);
        fclose(fp);
    }
    isDirty = 0;
}

void readBlock() {
    char fn[24];
    sprintf(fn, "block-%03d.R4", blkNum);
    for (int i = 0; i < BLOCK_SZ; i++) { theBlock[i] = 32; }
    FILE* fp = fopen(fn, "rt");
    if (fp) {
        int n = fread(theBlock, 1, BLOCK_SZ, fp);
        fclose(fp);
    }
    isDirty = 0;
}

void GotoXY(int x, int y) { printStringF("\x1B[%d;%dH", y, x); }
void CLS() { printString("\x1B[2J"); GotoXY(1, 1); }
void CursonOn() { printString("\x1B[?25h"); }
void CursonOff() { printString("\x1B[?25l"); }

void showGuide() {
    printString("\r\n     +"); 
    for (int i = 0; i <= MAX_X; i++) { printChar('-'); } 
    printChar('+');
}

void showFooter() {
    printString("\r\n      (q)home (w)up (e)end (a)left (s)down (d)right (t)top (l)last");
    printString("\r\n      (x)del (i)insert (r)replace (T)Type (n)CrLf");
    printString("\r\n      (S)save (R)reload (+)next (-)prev (Q)quit");
    printString("\r\n-> \x8");
}

void showEditor() {
    int cp = 0;
    CursonOff();
    GotoXY(1, 1);
    printString("Block Editor v0.1 - ");
    printStringF("Block# %03d %c", blkNum, isDirty ? '*' : ' ');
    showGuide();
    for (int i = 0; i <= MAX_Y; i++) {
        printStringF("\r\n %2d  |", i);
        // printStringF("\r\n %2d %c|", i, (i == line) ? '>' : ' ');
        for (int j = 0; j <= MAX_X; j++) {
            if (cur == cp) {
                printChar((char)178);
            }
            unsigned char c = theBlock[cp++];
            if (c == 13) { c = 174; }
            if (c == 10) { c = 241; }
            if (c ==  9) { c = 242; }
            if (c <  32) { c = '.'; }
            printChar((char)c);
        }
        printString("| ");
    }
    showGuide();
    CursonOn();
}

void replaceChar(char c) {  }
void CrLf() {
    // theBlock[cur++] = 13;
    theBlock[cur++] = 10;
}

void doType() {
    CursonOff();
    while (1) {
        char c= getChar();
        if (c == 27) { --cur;  return; }
        int isBS = ((c == 127) || (c == 8));
        if (isBS) {
            if (cur) {
                theBlock[--cur] = ' ';
                printString("\x8 \x8");
                showEditor();
            }
            continue;
        }
        if (c == 13) { CrLf(); }
        else { theBlock[cur++] = c; }
        showEditor();
        CursonOff();
    }
}

void deleteChar() {
    for (int i = cur; i < MAX_CUR; i++) { theBlock[i] = theBlock[i+1]; }
    //theBlock[MAX_CUR - 2] = 32;
    theBlock[MAX_CUR - 1] = 32;
    theBlock[MAX_CUR] = 10;
}

void insertChar(char c) {
    for (int i = MAX_CUR; cur < i; i--) { theBlock[i] = theBlock[i-1]; }
    theBlock[cur] = c;
    //theBlock[MAX_CUR - 1] = 13;
    theBlock[MAX_CUR] = 10;
}

int doEditorChar(char c) {
    printChar(c);
    switch (c) {
    case 'Q': return 0;                                  break;
    case 'a': --cur;                                     break;
    case 'd': ++cur;                                     break;
    case 'w': cur -= LLEN;                               break;
    case 's': cur += LLEN;                               break;
    case 'q': cur -= (cur % LLEN);                       break;
    case 'e': cur -= (cur % LLEN); cur += MAX_X;         break;
    case 't': cur = 0;                                   break;
    case 'l': cur = MAX_CUR - MAX_X;                     break;
    case 'r': isDirty = 1; theBlock[cur++] = getChar();  break;
    case 'T': isDirty = 1; doType();                     break;
    case 'n': isDirty = 1; CrLf();                       break;
    case 'x': isDirty = 1; deleteChar();                 break;
    case 'i': isDirty = 1; insertChar(' ');              break;
    case 'L': readBlock();                               break;
    case 'S': saveBlock();                               break;
    case '+': if (isDirty) { saveBlock(); }
            ++blkNum;
            readBlock(); cur = 0;
            break;
    case '-': if (isDirty) { saveBlock(); }
            blkNum -= (blkNum) ? 1 : 0;
            readBlock(); cur = 0;
            break;
    }
    return 1;
}

void doEditor() {
    blkNum = pop();
    cur = 0;
    CLS();
    readBlock();
    showEditor();
    showFooter();
    while (doEditorChar(getChar())) {
        if (cur < 0) { cur = 0; }
        if (MAX_CUR < cur) { cur = MAX_CUR; }
        showEditor();
        showFooter();
    }
}
#endif
