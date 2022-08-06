// A genetic algorithm

1 load
// 223 load

200 constant maxC
 55 constant maxR
maxC 1+ maxR 1+ * constant world-sz
variable world world-sz allot

500 constant #crits
  8 constant #conns

: rand-neu rand ;

: worldClr 0 world world-sz fill-n ;
: T0 ( c r--a ) maxC * + world + ;
: w-set ( n c r--) T0 c! ;
: w-get ( c r--n ) T0 c@ ;
: w-paintR ( r-- ) 1 swap T0
	1 maxC for dup c@ dup if FG '*' else drop bl then emit 1+ next
	drop cr ;
: w-paint ( -- ) 1 1 ->XY 1 maxR for i w-paintR next ;

// connection
// [from:8][to:8][weight:16]
// from: [type:1][id:7] - type: 0=>input, 1=>hidden
// to:   [type:1][id:7] - type: 0=>output, 1=>hidden
// weight: normalized to -400 to 400
4 constant conn-sz

// critter:
// [c:1][r:1][color:1][unused:1][connections:#conns]
#conns conn-sz * 4 +      constant critter-sz
#crits 1+ critter-sz *   constant critters-sz
variable critters #crits 1+ critter-sz * allot
: critters-end critters critters-sz + ;
: next-conn ( a--b ) 4 + ;

// r6: the current critter
: set-crit ( n--r6 ) critter-sz * critters + s6 ;

: getC  ( --c ) r6 c@ ;
: setC  ( c-- ) r6 c! ;
: getR  ( --r ) r6 1+ c@ ;
: setR  ( r-- ) r6 1+ c! ;
: getCR ( --c r ) getC getR ;
: setCR ( c r-- ) setR setC ;
: setCLR ( n-- ) r6 2+ c! ;
: getCLR ( --n ) r6 2+ c@ ;
: setliving ( f-- ) r6 3 + ! ; // 0 == dead
: isAlive? ( --f ) r6 3 + c@ ;
: isDead? ( --f ) r6 3 + c@ 0= ;
: crit-conns ( --a ) r6 4 + ;

// : +rand rand abs ;
: rand-cr rand maxC mod 1+ setC rand maxR mod 1+ setR ;
: rand-clr rand 7 mod 31 + setCLR ;
: rand-crit rand-cr rand-clr 1 setLiving crit-conns 0 #conns for rand-neu over ! next-conn next drop ;
: rand-crits 0 #crits for i set-crit rand-crit next ;

: normC ( c--c1 ) maxC min 1 max ;
: normR ( r--r1 ) maxR min 1 max ;

: up    getR 1- normR setR ;
: down  getR 1+ normR setR ;
: left  getC 1- normC setC ;
: right getC 1+ normC setC ;

: up?    dup 0 = if up    then ;
: down?  dup 1 = if down  then ;
: left?  dup 2 = if left  then ;
: right?     3 = if right then ;

: unpaint-crit getCR ->XY space ;
: paint-crit getCLR FG getCR ->XY '*' emit ;
: paint-crits 1 #crits for i set-crit paint-crit next ;

: move rand 3 and up? down? left? right? ;
: work-conn ( a-- ) drop move ;
: work-conns ( a-- ) 0 #conns for dup work-conn next-conn next drop ;
: work-crit unpaint-crit crit-conns work-conns paint-crit ;
: one-day 0 #crits for i set-crit work-crit next ;

: dump-crit getCLR getCR swap i ." %d: (%d,%d) %d%n" ;
: dump-crits 1 #crits for i set-crit dump-crit next ;

: die? maxC getC - 10 <= setLiving ;
: next-alive ( --a ) begin 
		r7 critter-sz + s7
		r7 critters-end > if critters s7 then
		r7 3 + c@ if unloop-w exit then
	while ;
: copy-mutate ( n1--n2 ) ;
: copy-cr rand-cr 1 setLiving crit-conns s9
	s7 4 + s8
	1 #conns for 
		r8 @ copy-mutate r9 !
		r8 4 + s8 r9 4 + s9
	next ;
: T1 isDead? if rand-cr else next-alive copy-cr then ;
: regen critters s7 1 #crits for i set-crit T1 next ;
// : go rand-crits C-OFF begin live die regen key? until key drop 37 FG C-ON 1 maxR ->XY ;
: go CLS CURSOR-OFF rand-crits
	begin one-day key? until key drop 0 FG CURSOR-ON ;
: reload 222 load ;
