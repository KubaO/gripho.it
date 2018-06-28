#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm_6502.h"

struct V6502 : Virtual_6502 {
   unsigned char *pc, *ea, *s;
   int flags, y, x, a;
   void (V6502::* JumpTable[])();
   int fetch();
   void execute();

   void op_illegal();
};

Virtual_6502 *asm_6502_info;

Virtual_6502 *New6502(void)
{
   V6502 *v6502;
   int size = sizeof(V6502)+65536*2;
   if ((v6502=(V6502*)malloc(size))==NULL)
      return NULL;
   memset(v6502,0,size);
   v6502->address_space=
         (unsigned char *)(((uintptr_t)v6502+sizeof(V6502)+65535)&~65535);
   memset(v6502->address_space,0xFF,65536);
   v6502->special_start=v6502->address_space;
   v6502->special_end=v6502->address_space;
   v6502->rom_start=v6502->address_space;
   v6502->special_write=NULL;
   v6502->special_read=NULL;
   v6502->PC=0;
   v6502->A=0;
   v6502->X=0;
   v6502->Y=0;
   v6502->S=0;
   v6502->P=0;
   return v6502;
}

void Free6502(Virtual_6502 *v6502)
{
   free(v6502);
}

/*
; X     = dh
; Y     = dl
; PC    = ebx
; EA    = ecx
; S     = stackp
; flags = esi (low 8 bits)
; ticks = esi (high 24 bits)
*/

int Execute6502(Virtual_6502 *v6502, int nticks)
{
   v6502->ticks = nticks;
   asm_6502_info=v6502;
   asm_6502_Execute();
   return nticks+v6502->ticks;
}

void asm_6502_Execute(void)
{
   static_cast<V6502*>(asm_6502_info)->execute();
}

void V6502::execute()
{
   pc = address_space + PC;
   ea = address_space;
   s = address_space + 0x100 + S;
   flags = P;
   y = Y;
   x = X;
   a = A;

// MWriteAH:
//   mwrite	ah
// SetZNAH:
//   setzn	ah
   while (ticks != 0 && flags != 0) {
      int ah = fetch();
      auto fun = JumpTable[ah & 0xFF];
      if (fun == &V6502::op_illegal) {
         ticks |= 0x800000;
         break;
      }
      (*this.*fun)();
   }

   PC = (uintptr_t)pc & 0xFFFF;
   A = a & 0xFF;
   X = x & 0xFF;
   Y = y & 0xFF;
   P = flags & 0xFF;
   S = (uintptr_t)s  & 0xFF;
}

int V6502::fetch() {}

void V6502::op_illegal() {}
