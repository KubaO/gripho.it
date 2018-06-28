#include <array>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm_6502.h"

enum {
   f_negative	=	0x80,
   f_overflow	=	0x40,
   f_unused    =	0x20,
   f_break		=	0x10,
   f_decimal	=	0x08,
   f_interrupt	=	0x04,
   f_zero		=	0x02,
   f_carry		=	0x01
};

struct V6502;
using JumpEntry = void (*)(V6502*);
using JumpImpl = void (V6502::*)();

struct V6502 : Virtual_6502 {
   unsigned char *pc, *ea, *stk;
   uint8_t flags, y, x, a;
   void execute();
   uint16_t pc_val() const { return (uintptr_t)pc; }

   template <int cycles, JumpImpl Op, JumpImpl Addr>
   void op_impl() {
      ticks -= cycles;
      (*this.*Addr)();
      (*this.*Op)();
   }

   constexpr static uint16_t make_u16(uint8_t lo, uint16_t hi) {
      return hi << 8 | lo;
   }

   // Memory Transfers

   uint16_t mreadw(uint16_t addr) const {
      return make_u16(address_space[addr+0], address_space[addr+1]);
   }

   uint8_t mread() {
      if (ea < special_start || ea >= special_end)
         return *ea;
      special_ea = ea;
      special_read(this);
      return *ea = special_value;
   }

   void mwrite(uint8_t val) {
      if (ea < special_start || ea >= special_end) {
         if (ea < rom_start)
            *ea = val;
      } else {
         special_ea = ea;
         special_value = val;
         special_write(this);
      }
   }

   uint8_t  fetch()  { return *pc++; }
   int8_t   fetchi() { return *pc++; }
   uint16_t fetchw() { return pc += 2, mreadw(pc[-2]); }


   // Address Calculations

   void lea_nop() {}

   void lea_abs()  { ea = address_space + fetchw(); }
   void lea_absy() { ea = address_space + fetchw() + y; }
   void lea_absx() { ea = address_space + fetchw() + x; }
   void lea_zp()   { ea = address_space + fetch(); }
   void lea_zpx()  { ea = address_space + ((fetch() + x) & 0xFF); }
   void lea_zpy()  { ea = address_space + ((fetch() + y) & 0xFF); }
   void lea_zpiy() { ea = address_space + mreadw(fetch()) + y; }
   void lea_zpxi() { ea = address_space + mreadw(fetch() + x); }
   void lea_absi() { ea = address_space + mreadw(fetchw()); }
   void lea_rel()  { ea = pc + fetchi(); }

   // Zero & Negative Setup

   void setzn(int8_t val) {
      flags &= ~(f_zero | f_negative);
      if (!val)
         flags |= f_zero;
      if (val < 0)
         flags |= f_negative;
   }

   // Operations

   void op_nop() {}

   void op_ldaimm() { setzn(a = fetch()); }
   void op_lda()    { setzn(a = mread()); }
   void op_sta()    { mwrite(a); }

   void op_ldximm() { setzn(x = fetch()); }
   void op_ldx()    { setzn(x = mread()); }
   void op_stx()    { mwrite(x); }

   void op_ldyimm() { setzn(y = fetch()); }
   void op_ldy()    { setzn(y = mread()); }
   void op_sty()    { mwrite(y); }

   void op_inx()    { setzn(++x); }
   void op_iny()    { setzn(++y); }
   void op_dex()    { setzn(--x); }
   void op_dey()    { setzn(--y); }

   inline void jump_if(bool c) {
      if (c) {
         pc = ea;
         ticks --;
      }
   }

   void op_bpl()  { jump_if(!(flags & f_negative)); }
   void op_bmi()  { jump_if(flags & f_negative); }
   void op_bvc()  { jump_if(!(flags & f_overflow)); }
   void op_bvs()  { jump_if(flags & f_overflow); }
   void op_bcc()  { jump_if(!(flags & f_carry)); }
   void op_bcs()  { jump_if(flags & f_carry); }
   void op_bne()  { jump_if(!(flags & f_zero)); }
   void op_beq()  { jump_if(flags & f_zero); }

   void op_oraimm() { setzn(a |= fetch()); }
   void op_ora()    { setzn(a |= mread()); }
   void op_andimm() { setzn(a &= fetch()); }
   void op_and()    { setzn(a &= mread()); }
   void op_eorimm() { setzn(a ^= fetch()); }
   void op_eor()    { setzn(a ^= mread()); }
   void op_inc()    { mwrite(mread() + 1); }
   void op_dec()    { mwrite(mread() - 1); }

   void op_asl()    { auto v = mread(); flags = (flags & ~f_carry) | (v >> 7); mwrite(v << 1); }
   void op_asla()   {                   flags = (flags & ~f_carry) | (a >> 7); setzn(a << 1); }
   void op_lsr()    { auto v = mread(); flags = (flags & ~f_carry) | (a & 1); mwrite(v >> 1); }
   void op_lsra()   {                   flags = (flags & ~f_carry) | (a & 1); setzn(a >> 1); }

   void op_bit() {
      auto v = mread();
      flags &= ~(f_overflow | f_negative | f_zero);
      if (v & 0x40) flags |= f_overflow;
      if (v & 0x80) flags |= f_negative;
      if (!(v & a)) flags |= f_zero;
   }
   void op_clc() { flags &= ~f_carry; }
   void op_sec() { flags |=  f_carry; }
   void op_cld() { flags &= ~f_decimal; }
   void op_sed() { flags |=  f_decimal; }
   void op_cli() { flags &= ~f_interrupt; }
   void op_sei() { flags |=  f_interrupt; }
   void op_clv() { flags &= ~f_overflow; }

   void cmp(int16_t v) {
      flags = (flags &= ~f_carry);
      if (v >= 0) flags |= f_carry;
      setzn(v);
   }

   void op_cmpimm() { cmp(a - fetch()); }
   void op_cmp()    { cmp(a - mread()); }
   void op_cpximm() { cmp(x - fetch()); }
   void op_cpx()    { cmp(x - mread()); }
   void op_cpyimm() { cmp(y - fetch()); }
   void op_cpy()    { cmp(y - mread()); }

   void op_jmp()    { pc = ea; }
   void op_jsr() {
      pc--;
      *stk-- = pc_val() >> 8;
      *stk-- = pc_val();
      pc = ea;
   }

   void op_pha() { *stk-- = a;     }
   void op_php() { *stk-- = flags; }

   void op_pla() { a = *++stk; setzn(a); }
   void op_plp() { flags = (*++stk | f_unused); }

   uint8_t rol(uint8_t v) {
      uint8_t carry = flags & f_carry;
      flags = (flags & ~f_carry) | (v >> 7);
      return (v << 1) | carry;
   }
   void op_rol()  { mwrite(rol(mread())); }
   void op_rola() { setzn(a = rol(a));    }

   uint8_t ror(uint8_t v) {
      uint8_t carry = flags & f_carry;
      flags = (flags & ~f_carry) | (v & 1);
      return (carry << 7) | (v >> 1);
   }
   void op_ror()  { mwrite(ror(mread())); }
   void op_rora() { setzn(a = ror(a));    }

   void pop_pc() {
      pc = address_space + make_u16(stk[1], stk[2]);
      stk += 2;
   }
   void op_rts() { pop_pc(); pc++; }
   void op_rti() { pop_pc(); flags = *++stk; }

   void op_brk() {
      *stk-- = pc_val();
      *stk-- = pc_val() >> 8;
      *stk-- = flags;
      pc = address_space + mreadw(0xFFFE);
      flags |= f_break | f_interrupt;
   }

   void op_txa() { setzn(a = x); }
   void op_tya() { setzn(a = y); }
   void op_tax() { setzn(x = a); }
   void op_tay() { setzn(y = a); }
   void op_tsx() { setzn(x = (uintptr_t)stk); }
   void op_txs() { stk = address_space + 0x100 + x; }

   // according to http://www.6502.org/tutorials/decimal_mode.html
   void adc(uint8_t b) {
      uint8_t const carry_in = flags & f_carry;
      flags &= ~(f_carry | f_negative | f_overflow);
      int16_t as;
      if (flags & f_decimal) {
         uint8_t al = (a & 0x0F) + (b & 0x0F) + carry_in;
         if (al >= 10) al = ((al + 6) & 0x0F) + 0x10;
         // C
         uint16_t au = (a & 0xf0) + (b & 0xf0) + al;
         if (au >= 0xA0) au += 0x60;
         a = au;
         if (au >= 0x100) flags |= f_carry;
         // N, V
         as = (int8_t)(a & 0xf0);
         as += (int8_t)(b & 0xf0);
         as += (int8_t)al;
      } else {
         as = (int8_t)a;
         as += (int8_t)b;
         as += carry_in;
         a = as;
      }
      if (!a) flags |= f_zero;
      if (as & 0x80) flags |= f_negative;
      if (as < -128 || as > 127) flags |= f_overflow;
   }
   void op_adcimm() { adc(fetch()); }
   void op_adc()    { adc(mread()); }

   // according to http://www.6502.org/tutorials/decimal_mode.html
   void sbc(uint8_t b) { //tbd
      uint8_t const carry_in = flags & f_carry;
      flags &= ~(f_carry | f_negative | f_overflow);
      int16_t as;
      if (flags & f_decimal) {
         int8_t al = (a & 0x0F) - (b & 0x0F) + carry_in - 1;
         if (al < 0) al = ((al - 6) & 0x0F) - 0x10;
         as = (a & 0xf0) - (b & 0xf0) + al;
         if (as < 0) as -= 0x60;
      } else {
         as = (int8_t)a;
         as -= (int8_t)b;
         as += carry_in;
         as -= 1;
      }
      a = as;
      if (!a) flags |= f_zero;
      if (as & 0x80) flags |= f_negative;
      if (as < -128 || as > 127) flags |= f_overflow;
   }
   void op_sbcimm() { sbc(fetch()); }
   void op_sbc()    { sbc(mread()); }

   void op_illegal() {}
};

Virtual_6502 *New6502(void)
{
   int size = sizeof(V6502)+65536*2;
   auto *const v6502 = (V6502*)malloc(size);
   if (!v6502)
      return {};
   memset(v6502,0,size);
   v6502->address_space=
         (unsigned char *)(((uintptr_t)v6502+sizeof(V6502)+65535)&~65535);
   memset(v6502->address_space,0xFF,65536);
   v6502->special_start=v6502->address_space;
   v6502->special_end=v6502->address_space;
   v6502->rom_start=v6502->address_space;
   return v6502;
}

void Free6502(Virtual_6502 *v6502)
{
   free(v6502);
}

int Execute6502(Virtual_6502 *v6502, int nticks)
{
   v6502->ticks = nticks;
   static_cast<V6502*>(v6502)->execute();
   return nticks+v6502->ticks;
}

std::array<JumpImpl, 256> JumpTableInit() {
   std::array<JumpImpl, 256> op;
   using V = V6502;
   op.fill(&V6502::op_illegal);

#define defop1(oper,cycles,operation,addrmode) \
   def<&V6502::op_##operation, &V6502::lea_##addrmode>(op[0x##oper], cycles)

#define defop(oper,cycles,operation,addrmode) \
   (op[0x##oper] = &V6502::op_impl<cycles, &V6502::op_##operation, &V6502::lea_##addrmode>)

   defop(00,7,brk,abs);
   defop(10,2,bpl,rel);
   defop(20,6,jsr,abs);
   defop(30,2,bmi,rel);
   defop(40,6,rti,nop);
   defop(50,2,bvc,rel);
   defop(60,6,rts,nop);
   defop(70,2,bvs,rel);
   defop(90,2,bcc,rel);
   defop(A0,2,ldyimm,nop);
   defop(B0,2,bcs,rel);
   defop(C0,2,cpyimm,nop);
   defop(D0,2,bne,rel);
   defop(E0,2,cpximm,nop);
   defop(F0,2,beq,rel);

   defop(01,4,ora,zpxi);
   defop(11,3,ora,zpiy);
   defop(21,4,and,zpxi);
   defop(31,3,and,zpiy);
   defop(41,4,eor,zpxi);
   defop(51,3,eor,zpiy);
   defop(61,4,adc,zpxi);
   defop(71,3,adc,zpiy);
   defop(81,4,sta,zpxi);
   defop(91,3,sta,zpiy);
   defop(A1,4,lda,zpxi);
   defop(B1,3,lda,zpiy);
   defop(C1,4,cmp,zpxi);
   defop(D1,3,cmp,zpiy);
   defop(E1,4,sbc,zpxi);
   defop(F1,3,sbc,zpiy);

   defop(A2,2,ldximm,nop);

   defop(24,3,bit,zp);
   defop(84,3,sty,zp);
   defop(94,4,sty,zpx);
   defop(A4,3,ldy,zp);
   defop(B4,4,ldy,zpx);
   defop(C4,3,cpy,zp);
   defop(E4,3,cpx,zp);

   defop(05,3,ora,zp);
   defop(15,4,ora,zpx);
   defop(25,3,and,zp);
   defop(35,4,and,zpx);
   defop(45,3,eor,zp);
   defop(55,4,eor,zpx);
   defop(65,3,adc,zp);
   defop(75,4,adc,zpx);
   defop(85,3,sta,zp);
   defop(95,4,sta,zpx);
   defop(A5,3,lda,zp);
   defop(B5,4,lda,zpx);
   defop(C5,3,cmp,zp);
   defop(D5,4,cmp,zpx);
   defop(E5,3,sbc,zp);
   defop(F5,4,sbc,zpx);

   defop(06,5,asl,zp);
   defop(16,6,asl,zpx);
   defop(26,5,rol,zp);
   defop(36,6,rol,zpx);
   defop(46,5,lsr,zp);
   defop(56,6,lsr,zpx);
   defop(66,5,ror,zp);
   defop(76,6,ror,zpx);
   defop(86,4,stx,zp);
   defop(96,3,stx,zpy);
   defop(A6,4,ldx,zp);
   defop(B6,3,ldx,zpy);
   defop(C6,5,dec,zp);
   defop(D6,6,dec,zpx);
   defop(E6,5,inc,zp);
   defop(F6,6,inc,zpx);

   defop(08,3,php,nop);
   defop(18,2,clc,nop);
   defop(28,4,plp,nop);
   defop(38,2,sec,nop);
   defop(48,3,pha,nop);
   defop(58,2,cli,nop);
   defop(68,4,pla,nop);
   defop(78,2,sei,nop);
   defop(88,2,dey,nop);
   defop(98,2,tya,nop);
   defop(A8,2,tay,nop);
   defop(B8,2,clv,nop);
   defop(C8,2,iny,nop);
   defop(D8,2,cld,nop);
   defop(E8,2,inx,nop);
   defop(F8,2,sed,nop);

   defop(09,2,oraimm,nop);
   defop(19,4,ora,absy);
   defop(29,2,andimm,nop);
   defop(39,4,and,absy);
   defop(49,2,eorimm,nop);
   defop(59,4,eor,absy);
   defop(69,2,adcimm,nop);
   defop(79,4,adc,absy);
   defop(99,4,sta,absy);
   defop(A9,2,ldaimm,nop);
   defop(B9,4,lda,absy);
   defop(C9,2,cmpimm,nop);
   defop(D9,4,cmp,absy);
   defop(E9,2,sbcimm,nop);
   defop(F9,4,sbc,absy);

   defop(0A,2,asla,nop);
   defop(2A,2,rola,nop);
   defop(4A,2,lsra,nop);
   defop(6A,2,rora,nop);
   defop(8A,2,txa,nop);
   defop(9A,2,txs,nop);
   defop(AA,2,tax,nop);
   defop(BA,2,tsx,nop);
   defop(CA,2,dex,nop);
   defop(EA,2,nop,nop);

   defop(2C,4,bit,abs);
   defop(4C,3,jmp,abs);
   defop(6C,5,jmp,absi);
   defop(8C,4,sty,abs);
   defop(AC,4,ldy,abs);
   defop(BC,4,ldy,absx);
   defop(CC,4,cpy,abs);
   defop(EC,4,cpx,abs);

   defop(0D,4,ora,abs);
   defop(1D,4,ora,absx);
   defop(2D,4,and,abs);
   defop(3D,4,and,absx);
   defop(4D,4,eor,abs);
   defop(5D,4,eor,absx);
   defop(6D,4,adc,abs);
   defop(7D,4,adc,absx);
   defop(8D,4,sta,abs);
   defop(9D,4,sta,absx);
   defop(AD,4,lda,abs);
   defop(BD,4,lda,absx);
   defop(CD,4,cmp,abs);
   defop(DD,4,cmp,absx);
   defop(ED,4,sbc,abs);
   defop(FD,4,sbc,absx);

   defop(0E,4,asl,abs);
   defop(1E,4,asl,absx);
   defop(2E,4,rol,abs);
   defop(3E,4,rol,absx);
   defop(4E,4,lsr,abs);
   defop(5E,4,lsr,absx);
   defop(6E,4,ror,abs);
   defop(7E,4,ror,absx);
   defop(8E,4,stx,abs);
   defop(AE,4,ldx,abs);
   defop(BE,4,ldx,absy);
   defop(CE,6,dec,abs);
   defop(DE,6,dec,absx);
   defop(EE,6,inc,abs);
   defop(FE,6,inc,absx);
#undef defop

   return op;
}

const std::array<JumpImpl, 256> JumpTable = JumpTableInit();

void V6502::execute()
{
   ea = address_space;
   pc = address_space + PC;
   stk = address_space + 0x100 + S;
   flags = P;
   a = A;
   x = X;
   y = Y;

   while (ticks != 0 && flags != 0) {
      auto fun = JumpTable[fetch()];
      if (fun == &V6502::op_illegal) {
         ticks |= 0x800000;
         break;
      }
      (*this.*fun)();
   }

   S = (uintptr_t)stk  & 0xFF;
   PC = pc_val();
   P = flags ;
   A = a;
   X = x;
   Y = y;
}
