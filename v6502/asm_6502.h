#if !defined(ASM_6502_H)
#define ASM_6502_H

typedef struct VIRTUAL_6502
{
   unsigned char *address_space;
   unsigned char *special_start;
   unsigned char *special_end;
   unsigned char *rom_start;
   int ticks;
   void (*special_write)(void);
   void (*special_read)(void);
   int special_ea;
   int special_value;
   int PC;
   int A;
   int X;
   int Y;
   int S;
   int P;
} Virtual_6502;

extern Virtual_6502 *asm_6502_info;
extern void cdecl asm_6502_Execute(void);

Virtual_6502 * __stdcall New6502(void);
void __stdcall Free6502(Virtual_6502 *v6502);
int __stdcall Execute6502(Virtual_6502 *v6502,int nticks);

#endif
