#include "R4.h"

#ifndef __FILES__
void noFile() { printString("-noFile-"); }
void fileInit() { noFile(); }
void fileOpen() { noFile(); }
void fileClose() { noFile(); }
void fileDelete() { noFile(); }
void fileRead() { noFile(); }
void fileWrite() { noFile(); }
addr fileLoad(addr x) { noFile(); return x; }
void fileSave(addr x, addr y) { noFile(); }
void blockLoad(CELL num) { noFile(); }
#else
#if __BOARD__ == PC
void fileInit() {}

// fO (nm md--fh) - File Open
// fh: File handle, nm: File name, md: mode
// fh=0: File not found or error
void fileOpen() {
    char* md = (char *)pop();
    char* fn = (char *)T;
    T = (CELL)fopen(fn, md);
}

// fC (fh--) - File Close
// fh: File handle
void fileClose() {
    FILE* fh = (FILE*)pop();
    if (fh) { fclose(fh); }
}

// fD (nm--) - File Delete
// nm: File name
// n=0: End of file or file error
void fileDelete() {
    char* fn = (char*)T;
    T = remove(fn) == 0 ? 1 : 0;
}

// fR (fh--c n) - File Read
// fh: File handle, c: char read, n: num chars read
// n=0: End of file or file error
void fileRead() {
    FILE* fh = (FILE*)T;
    N = T = 0;
    push(0);
    if (fh) {
        char c;
        T = fread(&c, 1, 1, fh);
        N = T ? c : 0;
    }
}

// fW (c fh--n) - File Write
// fh: File handle, c: char to write, n: num chars written
// n=0: File not open or error
void fileWrite() {
    FILE* fh = (FILE*)pop();
    char c = (char)T;
    T = 0;
    if (fh) {
        T = fwrite(&c, 1, 1, fh);
    }
}

// fL (--) - File Load code
addr fileLoad(addr user) {
    FILE *fh = fopen("Code.S4", "rt");
    addr here = HERE;
    if (fh) {
        vmInit();
        int num = fread(user, 1, USER_SZ, fh);
        fclose(fh);
        here = user + num;
        *(here) = 0;
        run(user);
        printStringF("-loaded, (%d bytes)-", num);
    }
    else {
        printString("-loadFail-");
    }
    return here;
}

// fS (--) - File Save code
void fileSave(addr user, addr here) {
    FILE* fh = fopen("Code.S4", "wt");
    if (fh) {
        int count = here - user;
        fwrite(user, 1, count, fh);
        fclose(fh);
        printStringF("-saved (%d)-", count);
    }
    else {
        printString("-saveFail-");
    }
}

void blockLoad(CELL num) {
    char buf[24];
    sprintf(buf, "Block-%03ld.r4", num);
    FILE* newFp = fopen(buf, "rb");
    if (newFp) {
        if (input_fp) { fpush(input_fp); }
        input_fp = newFp;
    }
}

#endif // PC

#ifdef __LITTLEFS__
#include "LittleFS.h"

#define MAX_FILES 10
File *files[MAX_FILES];
int numFiles = 0;
File f;

int freeFile() {
  for (int i = 1; i <= MAX_FILES; i++) {
    if (files[i-1] == NULL) { return i; }
  }
  isError = 1;
  printString("-fileFull-");
  return 0;
}

void fileInit() {
    for (int i = 0; i < MAX_FILES; i++) { files[i] = NULL; }
    LittleFS.begin();
    FSInfo fs_info;
    LittleFS.info(fs_info);
    printStringF("\r\nLittleFS: Total: %ld", fs_info.totalBytes);
    printStringF("\r\nLittleFS: Used: %ld", fs_info.usedBytes);
}

void fileOpen() {
    char* md = (char*)pop();
    char* fn = (char*)pop();
    int i = freeFile();
    if (i) {
        f = LittleFS.open(fn, md);
        if (f) { files[i-1] = &f; } 
        else { 
            i = 0;
            isError = 1; 
            printString("-openFail-");
        }
    }
    push(i);
}

void fileClose() {
    int fn = (int)pop();
    if ((0 < fn) && (fn <= MAX_FILES) && (files[fn-1] != NULL)) {
        files[fn-1]->close();
        files[fn-1] = NULL;
    }
}

void fileDelete() {
    char* fn = (char*)T;
    T = LittleFS.remove(fn) ? 1 : 0;
}

void fileRead() {
    int fn = (int)pop();
    push(0);
    push(0);
    if ((0 < fn) && (fn <= MAX_FILES) && (files[fn-1] != NULL)) {
        byte c;
        T = files[fn-1]->read(&c, 1);
        N = (CELL)c;
    }
}

void fileWrite() {
    int fn = (int)pop();
    byte c = (byte)T;
    T = 0;
    if ((0 < fn) && (fn <= MAX_FILES) && (files[fn - 1] != NULL)) {
        T = files[fn-1]->write(&c, 1);
    }
}

void fileLoad() {
    File f = LittleFS.open("/Code.S4", "r");
    if (f) {
        vmInit();
        int num = f.read(HERE, USER_SZ);
        f.close();
        HERE += num;
        *(HERE+1) = 0;
        run(USER);
        printStringF("-loaded, (%d)-", num);
    }
    else {
        printString("-loadFail-");
    }
}

void fileSave() {
    File f = LittleFS.open("/Code.S4", "w");
    if (f) {
        int count = HERE - USER;
        f.write(USER, count);
        f.close();
        printString("-saved-");
    }
    else {
        printString("-saveFail-");
    }
}

void blockLoad(CELL num) { printString("-noBlock-"); }

#endif // __LITTLEFS__
#endif // __FILES__
