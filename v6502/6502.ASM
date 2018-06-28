	.486
	locals	@@

_DATA	segment dword use32 public 'DATA'

	extrn _asm_6502_info:dword

stackp	dd	0


_DATA	ends

_TEXT	segment para use32 public 'CODE'
	assume cs:_TEXT,ds:_DATA,es:NOTHING,ss:NOTHING

	;
	; X     = dh
	; Y     = dl
	; PC    = ebx
	; EA    = ecx
	; S     = stackp
	; flags = esi (low 8 bits)
	; ticks = esi (high 24 bits)
	;
	
f_negative	equ	080h
f_overflow	equ	040h
f_unused	equ	020h
f_break		equ	010h
f_decimal	equ	008h
f_interrupt	equ	004h
f_zero		equ	002h
f_carry		equ	001h

;
; Read from memory location pointed by ecx
;
mread	macro	reg
	local	@@NoSpecialR
	cmp	ecx,specialstart[ebp]
	jb	@@NoSpecialR
	cmp	ecx,specialend[ebp]
	jae	@@NoSpecialR
	;
	; Special read
	;
	pushad
	mov	word ptr specialea[ebp],cx
	call	readfunc[ebp]
	mov	reg,byte ptr specialvalue[ebp]
	mov	[ecx],reg
	popad
@@NoSpecialR:
	mov	reg,[ecx]
	endm

;
; Write to memory location pointed by ecx
;
mwrite	macro	reg
	cmp	ecx,specialstart[ebp]
	jb	@@NoSpecialW
	cmp	ecx,specialend[ebp]
	jae	@@NoSpecialW
	;
	; Special write
	;
	pushad
	mov	word ptr specialea[ebp],cx
	mov	byte ptr specialvalue[ebp],reg
	call	writefunc[ebp]
	popad
@@NoSpecialW:
	cmp	ecx,romstart[ebp]
	jae	@@NoWrite
	mov	[ecx],reg
@@NoWrite:
	endm

mwriteah macro
	jmp	MWriteAH
	endm

;
; Fetch next byte from [ebx] (incremented)
;
fetch	macro	reg
	mov	reg,[ebx]
	inc	bx
	endm

;
; Address computation: result in ecx
;
lea_nop	macro
	endm

lea_abs macro
	fetch	cl
	fetch	ch
	endm

lea_absy macro
	fetch	cl
	fetch	ch
	add	cl,dl
	adc	ch,0
	endm

lea_absx macro
	fetch	cl
	fetch	ch
	add	cl,dh
	adc	ch,0
	endm

lea_zp macro
	fetch	cl
	xor	ch,ch
	endm

lea_zpx macro
	fetch	cl
	xor	ch,ch
	add	cl,dh
	endm

lea_zpy macro
	fetch	cl
	xor	ch,ch
	add	cl,dl
	endm

lea_zpiy macro
	push	eax
	mov	eax,ecx
	fetch	al
	xor	ah,ah
	mov	cl,[eax]
	inc	al
	mov	ch,[eax]
	add	cl,dl
	adc	ch,0
	pop	eax
	endm

lea_zpxi macro
	push	eax
	mov 	eax,ecx
	fetch	al
	xor	ah,ah
	add	al,dh
	mov	cl,[eax]
	inc	al
	mov	ch,[eax]
	pop	eax
	endm

lea_absi macro
	push	eax
	fetch	cl
	fetch	ch
	mread	al
	inc	cl
	mread	ah
	mov	cx,ax
	pop	eax
	endm

lea_rel	macro
        mov	edi,eax
        fetch   ah
        mov     cx,bx
        sar     ax,8
        add     cx,ax
        mov	eax,edi
	endm

;
; Zero & negative setup
;
setzn	macro	reg
	and	esi,0FFFFFF7Dh
	test	reg,reg
	jne	@@NoZero
	or	esi,f_zero
@@NoZero:
	test	reg,080h
	je	@@NoNeg
	or	esi,f_negative
@@NoNeg:
	endm
;
; Operations
;
op_nop	macro
	endm

op_ldaimm macro
	fetch	al
	setzn	al
	endm

op_lda	macro
	mread	al
	setzn	al
	endm

op_sta	macro
	mwrite	al
	endm

op_ldximm macro
	fetch	dh
	setzn	dh
	endm
	
op_ldx	macro
	mread	dh
	setzn	dh
	endm

op_stx	macro
	mwrite	dh
	endm

op_ldyimm macro
	fetch	dl
	setzn	dl
	endm
	
op_ldy	macro
	mread	dl
	setzn	dl
	endm

op_sty	macro
	mwrite	dl
	endm

op_inx	macro
	inc	dh
	setzn	dh
	endm

op_iny	macro
	inc	dl
	setzn	dl
	endm

op_dex	macro
	dec	dh
	setzn	dh
	endm

op_dey	macro
	dec	dl
	setzn	dl
	endm

op_bpl	macro
	test	esi,f_negative
	jne	@@NoBranch
	mov	bx,cx
	sub	esi,256
@@NoBranch:
	endm

op_bmi	macro
	test	esi,f_negative
	je	@@NoBranch
	mov	bx,cx
	sub	esi,256
@@NoBranch:
	endm

op_bvc	macro
	test	esi,f_overflow
	jne	@@NoBranch
	mov	bx,cx
	sub	esi,256
@@NoBranch:
	endm

op_bvs	macro
	test	esi,f_overflow
	je	@@NoBranch
	mov	bx,cx
	sub	esi,256
@@NoBranch:
	endm

op_bcc	macro
	test	esi,f_carry
	jne	@@NoBranch
	mov	bx,cx
	sub	esi,256
@@NoBranch:
	endm

op_bcs	macro
	test	esi,f_carry
	je	@@NoBranch
	mov	bx,cx
	sub	esi,256
@@NoBranch:
	endm

op_bne	macro
	test	esi,f_zero
	jne	@@NoBranch
	mov	bx,cx
	sub	esi,256
@@NoBranch:
	endm

op_beq	macro
	test	esi,f_zero
	je	@@NoBranch
	mov	bx,cx
	sub	esi,256
@@NoBranch:
	endm

op_oraimm macro
	fetch 	ah
	or	al,ah
	setzn	al
	endm

op_ora	macro
	mread	ah
	or	al,ah
	setzn	al
	endm

op_andimm macro
	fetch 	ah
	and	al,ah
	setzn	al
	endm

op_and	macro
	mread	ah
	and	al,ah
	setzn	al
	endm

op_eorimm macro
	fetch	ah
	xor	al,ah
	setzn	al
	endm

op_eor	macro
	mread	ah
	xor	al,ah
	setzn	al
	endm

op_inc	macro
	mread	ah
	inc	ah
	mwriteah
	endm

op_dec	macro
	mread	ah
	dec	ah
	mwriteah
	endm

op_asl	macro
	mread	ah
	shr	esi,1
	shl	ah,1
	rcl	esi,1
	mwriteah
	endm

op_asla	macro
	shr	esi,1
	shl	al,1
	rcl	esi,1
	setzn	al
	endm

op_lsr	macro
	mread	ah
	shr	esi,1
	shr	ah,1
	rcl	esi,1
	mwriteah
	endm

op_lsra	macro
	shr	esi,1
	shr	al,1
	rcl	esi,1
	setzn	al
	endm

op_bit	macro
	mread	ah
	and	esi,not (f_overflow or f_negative or f_zero)
	test	ah,040h
	je	@@NoSetV
	or	esi,f_overflow
@@NoSetV:
	test	ah,080h
	je	@@NoSetNeg
	or	esi,f_negative
@@NoSetNeg:
	and	ah,al
	jne	@@NoSetZero
	or	esi,f_zero
@@NoSetZero:
	endm

op_clc	macro
	and	esi,not f_carry
	endm

op_sec	macro
	or	esi,f_carry
	endm

op_cld	macro
	and	esi,not f_decimal
	endm

op_sed	macro
	or	esi,f_decimal
	endm

op_cli	macro
	and	esi,not f_interrupt
	endm

op_sei	macro
	or	esi,f_interrupt
	endm

op_clv	macro
	and	esi,not f_overflow
	endm

op_cmpimm macro
	fetch	ah
	push	eax
	shr	esi,1
	sub	al,ah
	cmc
	rcl	esi,1
	setzn	al
	pop	eax
	endm

op_cmp	macro
	mread	ah
	push	eax
	shr	esi,1
	sub	al,ah
	cmc
	rcl	esi,1
	setzn	al
	pop	eax
	endm

op_cpximm macro
	fetch	ah
	push	edx
	shr	esi,1
	sub	dh,ah
	cmc
	rcl	esi,1
	setzn	dh
	pop	edx
	endm

op_cpx macro
	mread	ah
	push	edx
	shr	esi,1
	sub	dh,ah
	cmc
	rcl	esi,1
	setzn	dh
	pop	edx
	endm

op_cpyimm macro
	fetch	ah
	push	edx
	shr	esi,1
	sub	dl,ah
	cmc
	rcl	esi,1
	setzn	dl
	pop	edx
	endm

op_cpy macro
	mread	ah
	push	edx
	shr	esi,1
	sub	dl,ah
	cmc
	rcl	esi,1
	setzn	dl
	pop	edx
	endm

op_jmp	macro
	mov	bx,cx
	endm

op_jsr	macro
	push	eax
	mov	eax,stackp
	dec	bx
	mov	[eax],bh
	dec	al
	mov	[eax],bl
	dec	al
	mov	stackp,eax
	mov	bx,cx
	pop	eax
	endm

op_pha	macro
	push	ebx
	mov	ebx,stackp
	mov	[ebx],al
	dec	bl
	mov	stackp,ebx
	pop	ebx
	endm

op_php	macro
	push	ebx
	push	eax
	mov	ebx,stackp
	mov	eax,esi
	mov	[ebx],al
	dec	bl
	mov	stackp,ebx
	pop	eax
	pop	ebx
	endm

op_pla	macro
	push	ebx
	mov	ebx,stackp
	inc	bl
	mov	al,[ebx]
	mov	stackp,ebx
	pop	ebx
	setzn	al
	endm

op_plp	macro
	push	ebx
	push	eax
	mov	eax,esi
	mov	ebx,stackp
	inc	bl
	mov	al,[ebx]
	or	al,f_unused
	mov	stackp,ebx
	mov	esi,eax
	pop	eax
	pop	ebx
	endm

op_rol	macro
	mread	ah
	shr	esi,1
	rcl	ah,1
	rcl	esi,1
	mwriteah
	endm

op_rola macro
	shr	esi,1
	rcl	al,1
	rcl	esi,1
	setzn	al
	endm

op_ror	macro
	mread	ah
	shr	esi,1
	rcr	ah,1
	rcl	esi,1
	mwriteah
	endm

op_rora	macro
	shr	esi,1
	rcr	al,1
	rcl	esi,1
	setzn	al
	endm

op_rts	macro
	push	eax
	mov	eax,stackp
	inc	al
	mov	bl,[eax]
	inc	al
	mov	bh,[eax]
	mov	stackp,eax
	pop	eax
	inc	bx
	endm

op_rti	macro
	push	eax
	mov	eax,stackp
	inc	al
	mov	bl,[eax]
	inc	al
	mov	bh,[eax]
	xchg	esi,ebx
	inc	al
	mov	bl,[eax]
	xchg	esi,ebx
	mov	stackp,eax
	pop	eax
	endm

op_brk	macro
	mov	ecx,stackp
	mov	[ecx],bl
	dec	cl
	mov	[ecx],bh
	dec	cl
	push	eax
	mov	eax,esi
	mov	[ecx],al
	dec	cl
	pop	eax
	mov	stackp,ecx
	mov	cx,0FFFEh
	mov	bx,[ecx]
	or	esi,f_break or f_interrupt
	endm

op_txa	macro
	mov	al,dh
	setzn	al
	endm

op_tya	macro
	mov	al,dl
	setzn	al
	endm

op_tax	macro
	mov	dh,al
	setzn	al
	endm

op_tay	macro
	mov	dl,al
	setzn	al
	endm

op_tsx	macro
	mov	dh,byte ptr stackp
	setzn	dh
	endm

op_txs	macro
	mov	byte ptr stackp,dh
	endm

op_adcimm macro
	fetch	ah
	test	esi,f_decimal
	je	@@NoDecAdjust
	shr	esi,1
	adc	al,ah
	daa
	seto	ah
	rcl	esi,1
	jmp	@@DoneAdd
@@NoDecAdjust:
	shr	esi,1
	adc	al,ah
	seto	ah
	rcl	esi,1
@@DoneAdd:
	setzn	al
	and	esi,not f_overflow
	test	ah,ah
	je	@@NoOverflow
	or	esi,f_overflow
@@NoOverflow:
	endm

op_adc	macro
	mread	ah
	test	esi,f_decimal
	je	@@NoDecAdjust
	shr	esi,1
	adc	al,ah
	daa
	seto	ah
	rcl	esi,1
	jmp	@@DoneAdd
@@NoDecAdjust:
	shr	esi,1
	adc	al,ah
	seto	ah
	rcl	esi,1
@@DoneAdd:
	setzn	al
	and	esi,not f_overflow
	test	ah,ah
	je	@@NoOverflow
	or	esi,f_overflow
@@NoOverflow:
	endm

op_sbcimm macro
	fetch	ah
	test	esi,f_decimal
	je	@@NoDecAdjust
	shr	esi,1
	cmc
	sbb	al,ah
	das
	seto	ah
	cmc
	rcl	esi,1
	jmp	@@DoneSub
@@NoDecAdjust:
	shr	esi,1
	cmc
	sbb	al,ah
	seto	ah
	cmc
	rcl	esi,1
@@DoneSub:
	setzn	al
	and	esi,not f_overflow
	test	ah,ah
	je	@@NoOverflow
	or	esi,f_overflow
@@NoOverflow:
	endm

op_sbc	macro
	mread	ah
	test	esi,f_decimal
	je	@@NoDecAdjust
	shr	esi,1
	cmc
	sbb	al,ah
	das
	seto	ah
	cmc
	rcl	esi,1
	jmp	@@DoneSub
@@NoDecAdjust:
	shr	esi,1
	cmc
	sbb	al,ah
	seto	ah
	cmc
	rcl	esi,1
@@DoneSub:
	setzn	al
	and	esi,not f_overflow
	test	ah,ah
	je	@@NoOverflow
	or	esi,f_overflow
@@NoOverflow:
	endm

defop	macro	opcode,cycles,operation,addrmode
	align	8
op_&opcode proc near
	sub esi,cycles*256
	lea_&addrmode
	op_&operation
	jmp MainLoop
op_&opcode endp
	endm

	defop	00,7,brk,abs
	defop	10,2,bpl,rel
	defop	20,6,jsr,abs
	defop	30,2,bmi,rel
	defop	40,6,rti,nop
	defop	50,2,bvc,rel
	defop	60,6,rts,nop
	defop	70,2,bvs,rel
	defop	90,2,bcc,rel
	defop	A0,2,ldyimm,nop
	defop	B0,2,bcs,rel
	defop	C0,2,cpyimm,nop
	defop	D0,2,bne,rel
	defop	E0,2,cpximm,nop
	defop	F0,2,beq,rel
	
	defop	01,4,ora,zpxi
	defop	11,3,ora,zpiy
	defop	21,4,and,zpxi
	defop	31,3,and,zpiy
	defop	41,4,eor,zpxi
	defop	51,3,eor,zpiy
	defop	61,4,adc,zpxi
	defop	71,3,adc,zpiy
	defop	81,4,sta,zpxi
	defop	91,3,sta,zpiy
	defop	A1,4,lda,zpxi
	defop	B1,3,lda,zpiy
	defop	C1,4,cmp,zpxi
	defop	D1,3,cmp,zpiy
	defop	E1,4,sbc,zpxi
	defop	F1,3,sbc,zpiy

	defop	A2,2,ldximm,nop
	
	defop	24,3,bit,zp
	defop	84,3,sty,zp
	defop	94,4,sty,zpx
	defop	A4,3,ldy,zp
	defop	B4,4,ldy,zpx
	defop	C4,3,cpy,zp
	defop	E4,3,cpx,zp

	defop	05,3,ora,zp
	defop	15,4,ora,zpx
	defop	25,3,and,zp
	defop	35,4,and,zpx
	defop	45,3,eor,zp
	defop	55,4,eor,zpx
	defop	65,3,adc,zp
	defop	75,4,adc,zpx
	defop	85,3,sta,zp
	defop	95,4,sta,zpx
	defop	A5,3,lda,zp
	defop	B5,4,lda,zpx
	defop	C5,3,cmp,zp
	defop	D5,4,cmp,zpx
	defop	E5,3,sbc,zp
	defop	F5,4,sbc,zpx

	defop	06,5,asl,zp
	defop	16,6,asl,zpx
	defop	26,5,rol,zp
	defop	36,6,rol,zpx
	defop	46,5,lsr,zp
	defop	56,6,lsr,zpx
	defop	66,5,ror,zp
	defop	76,6,ror,zpx
	defop	86,4,stx,zp
	defop	96,3,stx,zpy
	defop	A6,4,ldx,zp
	defop	B6,3,ldx,zpy
	defop	C6,5,dec,zp
	defop	D6,6,dec,zpx
	defop	E6,5,inc,zp
	defop	F6,6,inc,zpx

	defop	08,3,php,nop
	defop	18,2,clc,nop
	defop	28,4,plp,nop
	defop	38,2,sec,nop
	defop	48,3,pha,nop
	defop	58,2,cli,nop
	defop	68,4,pla,nop
	defop	78,2,sei,nop
	defop	88,2,dey,nop
	defop	98,2,tya,nop
	defop	A8,2,tay,nop
	defop	B8,2,clv,nop
	defop	C8,2,iny,nop
	defop	D8,2,cld,nop
	defop	E8,2,inx,nop
	defop	F8,2,sed,nop

	defop	09,2,oraimm,nop
	defop	19,4,ora,absy
	defop	29,2,andimm,nop
	defop	39,4,and,absy
	defop	49,2,eorimm,nop
	defop	59,4,eor,absy
	defop	69,2,adcimm,nop
	defop	79,4,adc,absy
	defop	99,4,sta,absy
	defop	A9,2,ldaimm,nop
	defop	B9,4,lda,absy
	defop	C9,2,cmpimm,nop
	defop	D9,4,cmp,absy
	defop	E9,2,sbcimm,nop
	defop	F9,4,sbc,absy

	defop	0A,2,asla,nop
	defop	2A,2,rola,nop
	defop	4A,2,lsra,nop
	defop	6A,2,rora,nop
	defop	8A,2,txa,nop
	defop	9A,2,txs,nop
	defop	AA,2,tax,nop
	defop	BA,2,tsx,nop
	defop	CA,2,dex,nop
	defop	EA,2,nop,nop

	defop	2C,4,bit,abs
	defop	4C,3,jmp,abs
	defop	6C,5,jmp,absi
	defop	8C,4,sty,abs
	defop	AC,4,ldy,abs
	defop	BC,4,ldy,absx
	defop	CC,4,cpy,abs
	defop	EC,4,cpx,abs

	defop	0D,4,ora,abs
	defop	1D,4,ora,absx
	defop	2D,4,and,abs
	defop	3D,4,and,absx
	defop	4D,4,eor,abs
	defop	5D,4,eor,absx
	defop	6D,4,adc,abs
	defop	7D,4,adc,absx
	defop	8D,4,sta,abs
	defop	9D,4,sta,absx
	defop	AD,4,lda,abs
	defop	BD,4,lda,absx
	defop	CD,4,cmp,abs
	defop	DD,4,cmp,absx
	defop	ED,4,sbc,abs
	defop	FD,4,sbc,absx

	defop	0E,4,asl,abs
	defop	1E,4,asl,absx
	defop	2E,4,rol,abs
	defop	3E,4,rol,absx
	defop	4E,4,lsr,abs
	defop	5E,4,lsr,absx
	defop	6E,4,ror,abs
	defop	7E,4,ror,absx
	defop	8E,4,stx,abs
	defop	AE,4,ldx,abs
	defop	BE,4,ldx,absy
	defop	CE,6,dec,abs
	defop	DE,6,dec,absx
	defop	EE,6,inc,abs
	defop	FE,6,inc,absx

op_illegal proc near
	jmp	MainLoop
	endp

op_80	equ	op_illegal
op_9E	equ	op_illegal

	irp	a,<02,12,22,32,42,52,62,72,82,92,B2,C2,D2,E2,F2>
op_&a	equ	op_illegal
	endm
	
	irp	a,<04,14,34,44,54,64,74,D4,F4>
op_&a	equ	op_illegal
	endm

op_89	equ	op_illegal

	irp	a,<1A,3A,5A,7A,DA,FA>
op_&a	equ	op_illegal
	endm

	irp	a,<0C,1C,3C,5C,7C,9C,DC,FC>
op_&a	equ	op_illegal
	endm

	irp	a,<0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F>
op_&a&7	equ	op_illegal
op_&a&3	equ	op_illegal
op_&a&B	equ	op_illegal
op_&a&F	equ	op_illegal
	endm

JumpTable:
	irp	a,<0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F>
	irp	b,<0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F>
	dd	offset op_&a&&b
	endm
	endm

;
; Main entry point
;
; A virtual 6502 is a data structure containing (each field is a dword):
;
; - address of virtual address space (64Kb aligned on a 64Kb boundary)
; - special range begin (absolute address)
; - special range end+1 (absolute address)
; - non-RAM begin (RAM starts with the address space; if a location is not
;   special and is not RAM then it's considered to be ROM)
; - number of 6502 clock ticks to execute
; - address of write callback function for special addresses
; - address of read callback function for special addresses
; - effective address (valid during a callback)
; - data value (valid during a callback)
; - initial value for PC (0x0000->0xFFFF)
; - initial value for A  (0x00->0xFF)
; - initial valye for X  (0x00->0xFF)
; - initial value for Y  (0x00->0xFF)
; - initial value for S  (0x00->0xFF)
; - initial value for P  (0x00->0xFF)
;

addrspace	equ	0
specialstart	equ	4
specialend	equ	8
romstart	equ	12
ticks		equ	16
writefunc	equ	20
readfunc	equ	24
specialea	equ	28
specialvalue	equ	32
initialPC	equ	36
initialA	equ	40
initialX	equ	44
initialY	equ	48
initialS	equ	52
initialP	equ	56

	public _asm_6502_Execute
_asm_6502_Execute proc near
	pushad

	mov	ebp,_asm_6502_info
	mov	ebx,addrspace[ebp]
	mov	ecx,ebx
	add	ebx,initialPC[ebp]
	mov	eax,initialS[ebp]
	add	eax,addrspace[ebp]
	add	eax,0100h
	mov	stackp,eax
	mov	eax,ticks[ebp]
	shl	eax,8
	mov	esi,initialP[ebp]
	add	esi,eax
	mov	dl,initialY[ebp]
	mov	dh,initialX[ebp]
	mov	al,initialA[ebp]
	jmp	MainLoop

MWriteAH:
	mwrite	ah
SetZNAH:
	setzn	ah
MainLoop:
	cmp	esi,0
	jle	@@Done
	fetch	ah
	mov	edi,eax
	shr	edi,8
	and	edi,0FFh
	mov	edi,dword ptr JumpTable[edi*4]
	cmp	edi,offset op_illegal
	je	@@Illegal
	jmp	edi
	;jmp	dword ptr JumpTable[edi*4]
@@Illegal:
	or	esi,080000000h
@@Done:
	mov	initialPC[ebp],bx
	mov	initialA[ebp],al
	mov	initialX[ebp],dh
	mov	initialY[ebp],dl
	mov	eax,esi
	mov	initialP[ebp],al
	mov	eax,stackp
	mov	initialS[ebp],al
	sar	esi,8
	mov	ticks[ebp],esi
	popad
	ret
	endp

_TEXT	ends
	end
