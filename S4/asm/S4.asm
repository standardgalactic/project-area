format PE console
include 'win32ax.inc'

;section '.idata' data readable import

; library kernel32, 'kernel32', msvcrt, 'msvcrt.dll'
; library msvcrt, 'msvcrt.dll'
; import kernel32, ReadConsole, 'ReadConsole', WriteConsole, 'WriteConsole', \
;        ExitProcess, 'ExitProcess', GetStdHandle, 'GetStdHandle'
; import msvcrt, printf, 'printf'

.data
; library crtdll, 'crtdll.dll'
; import crtdll, printf, 'printf'

hStdIn      dd  0
hStdOut     dd  0
hello       db  "hello.", 0
okStr       db  "s4:()>", 0
dotStr      db  "%ld", 0
crlf        db  13, 10
tib         db  128 dup ?
outBuf      db  256 dup ?
bytesRead   dd  0
unkOP       db  "-unk-"
isNum       db  "-num-"
isReg       db  "-reg-"
rDepth      dd   ?
rStackPtr   dd   ?
fStartAddr  dd   ?
fEndAddr    dd   ?
fI          dd   ?
fEndI       dd   ?


; ******************************************************************************
; CODE 
; ******************************************************************************
section '.code' code readable executable

; ------------------------------------------------------------------------------
; macros ...
; ------------------------------------------------------------------------------

CELL_SIZE equ 4
REG1 equ eax         ; Free register #1
REG2 equ ebx         ; Free register #2
REG3 equ ecx         ; Free register #3
REG4 equ edx         ; Free register #4
TOS  equ edi         ; Top-Of-Stack
PCIP equ esi         ; Program-Counter/Instruction-Pointer
STKP equ ebp         ; Stack-Pointer


macro m_setTOS val
{
       mov TOS, val
}

macro m_push val
{
       ; inc [dDepth]
       add STKP, CELL_SIZE
       mov [STKP], TOS
       m_setTOS val
}

; ------------------------------------------------------------------------------
macro m_get2ND val
{
       mov val, [STKP]
}
macro m_set2ND val
{
       mov [STKP], val
}

; ------------------------------------------------------------------------------
macro m_getTOS val
{
       mov val, TOS
}

macro m_drop
{
       ; dec [dDepth]
       mov TOS, [STKP]
       sub STKP, CELL_SIZE
}

macro m_pop val
{
       m_getTOS val
       m_drop
}

; ------------------------------------------------------------------------------
macro m_toVmAddr reg
{
       add reg, edx
}

macro m_fromVmAddr reg
{
       sub reg, edx
}

; ------------------------------------------------------------------------------

macro m_rpush reg
{
       push TOS
       add [rStackPtr], CELL_SIZE
       mov TOS, [rStackPtr]
       mov [TOS], reg
       pop TOS
}
 
macro m_rpop reg
{
       push TOS
       mov TOS, [rStackPtr]
       mov reg, [TOS]
       sub [rStackPtr], CELL_SIZE
       pop TOS
}

; -------------------------------------------------------------------------------------
macro m_NEXT
{
        lodsb
        cmp   al, $7E
        jg    s0
        movzx ecx, al
        shl   ecx, 2
        mov   ebx, [jmpTable+ecx]
        jmp   ebx
}

; ******************************************************************************
doNop:  m_NEXT

; ******************************************************************************
iToA:   mov     ecx, outBuf+16
        mov     ebx, 0
        mov     BYTE [ecx], 0
i2a1:   push    ebx
        mov     ebx, 10
        mov     edx, 0
        div     ebx
        add     dl, '0'
        dec     ecx 
        mov     BYTE [ecx], dl
        pop     ebx
        inc     ebx
        cmp     eax, 0
        jne     i2a1
        ret

; ******************************************************************************
; register
reg:    sub     al, 'a'
        and     eax, $ff
        mov     edx, eax
reg1:   mov     al, [esi]
        mov     bx, 'az'
        call    betw
        cmp     bl, 0
        je      regN
        sub     al, 'a'
        imul    edx, edx, 26
        add     edx, eax
        inc     esi
        mov     al, [esi]
regN:   shl     edx, 2
        add     edx, regs
        cmp     al, ';'
        je      regSet
        m_push  [edx]
        cmp     al, '+'
        je      regInc
        cmp     al, '-'
        je      regDec
regX:   m_NEXT
regInc: inc     DWORD [edx]
        inc     esi
        jmp     regX
regDec: dec     DWORD [edx]
        inc     esi
        jmp     regX
regSet: m_pop   eax
        mov     [edx], eax
        inc     esi
        jmp     regX

; ******************************************************************************
; Number input
num:    sub     al, '0'
        and     eax, $FF
        mov     edx, eax
n1:     mov     al, [esi]
        mov     bx, '09'
        call    betw
        cmp     bl, 0
        je      nx
        sub     al, '0'
        imul    edx, edx, 10
        add     edx, eax
        inc     esi
        jmp     n1
nx:     m_push  edx
        m_NEXT

; ******************************************************************************
; Quote
qt:     lodsb
        cmp     al, '"'
        je      qx
        call    p1
        jmp     qt
qx:     inc     esi
        m_NEXT

; ******************************************************************************
betw:   cmp     al, bl
        jl      betF
        cmp     al, bh
        jg      betF
        mov     bl, 1
        ret
betF:   mov     bl, 0
        ret

; ******************************************************************************
doFor:  m_pop   REG2
        m_pop   REG1
        .if     REG2 < REG1
            push    REG1
            mov     REG1, REG2
            pop     REG2
        .endif
        mov     [fEndAddr], 0
        mov     [fStartAddr], esi
        mov     [fEndI], REG2
        mov     [fI], REG1
        jmp     s4Next

; ******************************************************************************
doNext: mov     REG1, [fI]
        mov     REG2, [fEndI]
        .if REG1 < REG2
            inc     DWORD [fI]
            mov     esi, [fStartAddr]
            mov     [fEndAddr], esi
        .endif
        jmp     s4Next

doI:    m_push  [fI]
        jmp     s4Next

; ******************************************************************************
s4Next: m_NEXT

; ******************************************************************************
f_UnknownOpcode:
        ; invoke WriteConsole, [hStdOut], unkOP, 5, NULL, NULL
        jmp s0

; ******************************************************************************
bye:    invoke  ExitProcess, 0

; ******************************************************************************
ok:
        mov al, 13
        call p1
        mov al, 10
        call p1
        invoke WriteConsole, [hStdOut], okStr, 6, NULL, NULL
        ret

; ******************************************************************************
doMult: m_pop   eax
        imul    TOS, eax
        jmp     s4Next

; ******************************************************************************
doSub:  m_pop   eax
        sub     TOS, eax
        jmp     s4Next

; ******************************************************************************
doAdd:  m_pop   eax
        add     TOS, eax
        jmp     s4Next

; ******************************************************************************
emit:   m_pop   eax
        call    p1
        jmp     s4Next

; ******************************************************************************
doBlank: mov    al, 32
        call    p1
        jmp     s4Next

; ******************************************************************************
doCrLf: mov     al, 13
        call    p1
        mov     al, 10
        call    p1
        jmp     s4Next

; ******************************************************************************
doDot:  m_pop   eax
        call    iToA
        invoke  WriteConsole, [hStdOut], ecx, ebx, NULL, NULL
        jmp     s4Next

; ******************************************************************************
doTimer: invoke GetTickCount
        m_push  eax
        jmp     s4Next

; ******************************************************************************
doDup:  m_push  TOS
        jmp     s4Next

; ******************************************************************************
doSwap: m_get2ND    eax
        m_set2ND    TOS
        m_setTOS    eax
        jmp     s4Next

; ******************************************************************************
doOver: m_get2ND    eax
        m_push      eax
        jmp         s4Next

; ******************************************************************************
doDrop: m_drop
        jmp         s4Next

; ******************************************************************************
p1:     mov     [outBuf], al
        invoke  WriteConsole, [hStdOut], outBuf, 1, NULL, NULL
        ret

; ******************************************************************************
s_SYS_INIT:
            ; Return stack
            mov eax, rStack - CELL_SIZE
            mov [rStackPtr], eax
            mov [rDepth], 0
            ; Data stack
            mov STKP, dStack
            ; PCIP = IP/PC
            mov PCIP, THE_MEMORY
            ret        

; ******************************************************************************
start:
        invoke GetStdHandle, STD_INPUT_HANDLE
        mov    [hStdIn], eax
        
        invoke GetStdHandle, STD_OUTPUT_HANDLE
        mov    [hStdOut], eax
    
        invoke WriteConsole, [hStdOut], hello, 6, NULL, NULL

s0:     call   ok
        invoke ReadConsole, [hStdIn], tib, 128, bytesRead, 0
        cld
        mov    esi, tib
        m_NEXT
    
        invoke ExitProcess, 0


; ******************************************************************************
section '.mem' data readable writable

jmpTable:
dd f_UnknownOpcode            ; # 00 ()
dd f_UnknownOpcode            ; # 01 (☺)
dd f_UnknownOpcode            ; # 02 (☻)
dd f_UnknownOpcode            ; # 03 (♥)
dd f_UnknownOpcode            ; # 04 (♦)
dd f_UnknownOpcode            ; # 05 (♣)
dd f_UnknownOpcode            ; # 06 (♠)
dd f_UnknownOpcode            ; # 07 ()
dd f_UnknownOpcode            ; # 08 ()
dd doNop                      ; # 09 ()
dd f_UnknownOpcode            ; # 010 ()
dd f_UnknownOpcode            ; # 011 ()
dd f_UnknownOpcode            ; # 012 ()
dd f_UnknownOpcode            ; # 013 ()
dd f_UnknownOpcode            ; # 014 ()
dd f_UnknownOpcode            ; # 015 ()
dd f_UnknownOpcode            ; # 016 (►)
dd f_UnknownOpcode            ; # 017 (◄)
dd f_UnknownOpcode            ; # 018 (↕)
dd f_UnknownOpcode            ; # 019 (‼)
dd f_UnknownOpcode            ; # 020 (¶)
dd f_UnknownOpcode            ; # 021 (§)
dd f_UnknownOpcode            ; # 022 (▬)
dd f_UnknownOpcode            ; # 023 (↨)
dd f_UnknownOpcode            ; # 024 (↑)
dd f_UnknownOpcode            ; # 025 (↓)
dd f_UnknownOpcode            ; # 026 (→)
dd f_UnknownOpcode            ; # 027 (
dd f_UnknownOpcode            ; # 028 (∟)
dd f_UnknownOpcode            ; # 029 (↔)
dd f_UnknownOpcode            ; # 030 (▲)
dd f_UnknownOpcode            ; # 031 (▼)
dd doNop                      ; # 032 ( )
dd f_UnknownOpcode            ; # 033 (!)
dd qt                         ; # 034 (")
dd doDup                      ; # 035 (#)
dd doSwap                     ; # 036 ($)
dd doOver                     ; # 037 (%)
dd f_UnknownOpcode            ; # 038 (&)
dd f_UnknownOpcode            ; # 039 (')
dd f_UnknownOpcode            ; # 040 (()
dd f_UnknownOpcode            ; # 041 ())
dd doMult                     ; # 042 (*)
dd doAdd                      ; # 043 (+)
dd emit                       ; # 044 (,)
dd doSub                      ; # 045 (-)
dd doDot                      ; # 046 (.)
dd f_UnknownOpcode            ; # 047 (/)
dd num                        ; # 048 (0)
dd num                        ; # 049 (1)
dd num                        ; # 050 (2)
dd num                        ; # 051 (3)
dd num                        ; # 052 (4)
dd num                        ; # 053 (5)
dd num                        ; # 054 (6)
dd num                        ; # 055 (7)
dd num                        ; # 056 (8)
dd num                        ; # 057 (9)
dd f_UnknownOpcode            ; # 058 (:)
dd f_UnknownOpcode            ; # 059 (;)
dd f_UnknownOpcode            ; # 060 (<)
dd f_UnknownOpcode            ; # 061 (=)
dd f_UnknownOpcode            ; # 062 (>)
dd f_UnknownOpcode            ; # 063 (?)
dd f_UnknownOpcode            ; # 064 (@)
dd f_UnknownOpcode            ; # 065 (A)
dd doBlank                    ; # 066 (B)
dd f_UnknownOpcode            ; # 067 (C)
dd f_UnknownOpcode            ; # 068 (D)
dd f_UnknownOpcode            ; # 069 (E)
dd f_UnknownOpcode            ; # 070 (F)
dd f_UnknownOpcode            ; # 071 (G)
dd f_UnknownOpcode            ; # 072 (H)
dd doI                        ; # 073 (I)
dd f_UnknownOpcode            ; # 074 (J)
dd f_UnknownOpcode            ; # 075 (K)
dd f_UnknownOpcode            ; # 076 (L)
dd f_UnknownOpcode            ; # 077 (M)
dd doCrLf                     ; # 078 (N)
dd f_UnknownOpcode            ; # 079 (O)
dd f_UnknownOpcode            ; # 080 (P)
dd f_UnknownOpcode            ; # 081 (Q)
dd f_UnknownOpcode            ; # 082 (R)
dd f_UnknownOpcode            ; # 083 (S)
dd doTimer                    ; # 084 (T)
dd f_UnknownOpcode            ; # 085 (U)
dd f_UnknownOpcode            ; # 086 (V)
dd f_UnknownOpcode            ; # 087 (W)
dd bye                        ; # 088 (X)
dd f_UnknownOpcode            ; # 089 (Y)
dd f_UnknownOpcode            ; # 090 (Z)
dd doFor                      ; # 091 ([)
dd doDrop                     ; # 092 (\)
dd doNext                     ; # 093 (])
dd f_UnknownOpcode            ; # 094 (^)
dd f_UnknownOpcode            ; # 095 (_)
dd f_UnknownOpcode            ; # 096 (`)
dd reg                        ; # 097 (a)
dd reg                        ; # 098 (b)
dd reg                        ; # 099 (c)
dd reg                        ; # 100 (d)
dd reg                        ; # 101 (e)
dd reg                        ; # 102 (f)
dd reg                        ; # 103 (g)
dd reg                        ; # 104 (h)
dd reg                        ; # 105 (i)
dd reg                        ; # 106 (j)
dd reg                        ; # 107 (k)
dd reg                        ; # 108 (l)
dd reg                        ; # 109 (m)
dd reg                        ; # 110 (n)
dd reg                        ; # 111 (o)
dd reg                        ; # 112 (p)
dd reg                        ; # 113 (q)
dd reg                        ; # 114 (r)
dd reg                        ; # 115 (s)
dd reg                        ; # 116 (t)
dd reg                        ; # 117 (u)
dd reg                        ; # 118 (v)
dd reg                        ; # 119 (w)
dd reg                        ; # 120 (x)
dd reg                        ; # 121 (y)
dd reg                        ; # 122 (z)
dd f_UnknownOpcode            ; # 123 ({)
dd f_UnknownOpcode            ; # 124 (|)
dd f_UnknownOpcode            ; # 125 (})
dd f_UnknownOpcode            ; # 126 (~)

dstackB     dd    ?           ; Buffer between stack and regs
dStack      dd   32 dup 0
dstackE     dd    ?           ; Buffer between stacks
rStack      dd   32 dup 0
tmpBuf3     dd    ?           ; Buffer for return stack

funcs       dd  676 dup 0
regs        dd  676 dup 0

THE_MEMORY  rb 1024*1024
MEM_END:

.end start
