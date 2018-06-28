#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm_6502.h"

#include <windows.h>

Virtual_6502 *asm_6502_info;

Virtual_6502 * __stdcall New6502(void)
{
  Virtual_6502 *v6502;
  if ((v6502=malloc(sizeof(Virtual_6502)+65536*2))==NULL)
    return NULL;
  memset(v6502,0,sizeof(Virtual_6502)+65536*2);
  v6502->address_space=
    (unsigned char *)(((int)v6502+sizeof(Virtual_6502)+65535)&~65535);
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

void __stdcall Free6502(Virtual_6502 *v6502)
{
  free(v6502);
}

int __stdcall Execute6502(Virtual_6502 *v6502,int nticks)
{
  v6502->ticks=nticks;
  asm_6502_info=v6502;
  asm_6502_Execute();
  return nticks+v6502->ticks;
}
