Here we go..

After hours fighting with the linking of a standard OBJ file to a Delphi
program i gave up resorting to a 32 bit DLL. There should be no performance
hit since the emulator has the main emulation loop in asm.

The included files are:

-- ASM stuff --

6502.OBJ       The result of assembling the 6502 emulator; i got this with
               tasm -ml -m 6502
6502.ASM       The emulator core source. It's written with an heavy use of
               macros so i wasn't able to port it to Delphi using the internal
	       assembler.

-- C stuff --

v6502.def      The definition file required to build the DLL
asm_6502.h     The interface file for compiling the DLL
asm_6502.c     The source of the C code making the DLL
v6502.dll      The virtual 6502 DLL (compiled with VC++)

-- Apple ][ stuff --

try.c          A text-only Apple ][ emulator i wrote to test the virtual
               CPU: it emulates only the screen and the keyboard so it's
	       nothing funny at all. It's for DOS/Watcom C and uses the
	       obj file (static link)
try.exe	       The executable of the Apple ][ emulator. There's no help
               so try to memorize these keys...

	       F1  = Ends the program
	       F2  = Reset
	       F3  = Step mode on
	       F4  = Step mode off (freerun)
	       F5  = Fast execution (emulates 100,000 cycles per video update)
	       F6  = Slow execution (emulates 1 instruction per video update)
	       F10 = Toggle showing of  disassembly, registers & zero page

apple2.rom     A ROM image of the Apple ][. I included it as it's used in
               my test programs and I've a real Apple ][ at home (so there's
	       no problem for me).
	       However you should not use this file unless you're the owner
	       of an original Apple ][. The test programs really doesn't do
	       anything nice so don't bother using it... just look what the
	       code does...

apple2.img     This is a full snapshot of 64K of memory of the virtual
               Apple ][. There's a little program you can see using RUN (use
	       F2 to break it).
	       The snapshot is taken every time you exit from the program and
	       if no snapshot is present then a normal boot starts (you have
	       to quit from the boot with F2 since the Apple ][ is trying to
	       read the boot sector from the disk).

-- Delphi stuff --

u6502.pas      The delphi interface unit to the DLL
test.pas       A delphi test program
m6502.dpr      The delphi project
u6502.dcu      Delphi stuff
test.dcu            "
test.dfm            "
m6502.dof           "
m6502.res           "
m6502.exe      The Delphi executable

