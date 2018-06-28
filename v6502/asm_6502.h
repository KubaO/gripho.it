#ifndef ASM_6502_H
#define ASM_6502_H

typedef struct VIRTUAL_6502
{
   // address of virtual address space (64Kb aligned on a 64Kb boundary)
   unsigned char *address_space;
   // special range begin (absolute address)
   unsigned char *special_start;
   // special range end+1 (absolute address)
   unsigned char *special_end;
   // non-RAM begin (RAM starts with the address space; if a location is not
   // special and is not RAM then it's considered to be ROM)
   unsigned char *rom_start;
   // number of 6502 clock ticks to execute
   int ticks;
   // address of write callback function for special addresses
   void (*special_write)(VIRTUAL_6502*);
   // address of read callback function for special addresses
   void (*special_read)(VIRTUAL_6502*);
   // effective address (valid during a callback)
   unsigned char *special_ea;
   // effective address space address (valid during a callback)
   int special_eai() const { return special_ea - address_space; }
   // data value (valid during a callback)
   int special_value;
   // initial value for PC (0x0000->0xFFFF)
   int PC;
   // initial value for A  (0x00->0xFF)
   int A;
   // initial valye for X  (0x00->0xFF)
   int X;
   // initial value for Y  (0x00->0xFF)
   int Y;
   // initial value for S  (0x00->0xFF)
   int S;
   // initial value for P  (0x00->0xFF)
   int P;
} Virtual_6502;

Virtual_6502 *New6502(void);
void Free6502(Virtual_6502 *v6502);
int Execute6502(Virtual_6502 *v6502,int nticks);

#endif
