#include <type.h>
#include <klibs.h>
#include <klibc.h>
#include <8259A.h>
#include <protect.h>
#include <segment.h>
#include <keyboard.h>
#include <floppy.h>
#include <clock.h>
#include <process.h>

extern GATE _idt[];

void static init_idt_desc(U32 vector, U8 desc_type, int_handler handler)
{
	GATE	*p_gate	= &_idt[vector];
	U32	base	= (U32)handler;
	p_gate->offset_low	= base & 0xFFFF;
	p_gate->selector	= CODE_SEGMENT;
	p_gate->dcount		= 0;
	p_gate->attr		= desc_type;
	p_gate->offset_high	= (base >> 16) & 0xFFFF;
}

void init_sys(void)
{
	init_8259A();
	init_idt_desc(INT_VECTOR_IRQ0 + 0,DA_386IGate, hwint00);
	init_idt_desc(INT_VECTOR_IRQ0 + 1,DA_386IGate, hwint01);
	init_idt_desc(INT_VECTOR_IRQ0 + 2,DA_386IGate, hwint02);
	init_idt_desc(INT_VECTOR_IRQ0 + 3,DA_386IGate, hwint03);
	init_idt_desc(INT_VECTOR_IRQ0 + 4,DA_386IGate, hwint04);
	init_idt_desc(INT_VECTOR_IRQ0 + 5,DA_386IGate, hwint05);
	init_idt_desc(INT_VECTOR_IRQ0 + 6,DA_386IGate, hwint06);
	init_idt_desc(INT_VECTOR_IRQ0 + 7,DA_386IGate, hwint07);
	init_idt_desc(INT_VECTOR_IRQ8 + 0,DA_386IGate, hwint08);
	init_idt_desc(INT_VECTOR_IRQ8 + 1,DA_386IGate, hwint09);
	init_idt_desc(INT_VECTOR_IRQ8 + 2,DA_386IGate, hwint10);
	init_idt_desc(INT_VECTOR_IRQ8 + 3,DA_386IGate, hwint11);
	init_idt_desc(INT_VECTOR_IRQ8 + 4,DA_386IGate, hwint12);
	init_idt_desc(INT_VECTOR_IRQ8 + 5,DA_386IGate, hwint13);
	init_idt_desc(INT_VECTOR_IRQ8 + 6,DA_386IGate, hwint14);
	init_idt_desc(INT_VECTOR_IRQ8 + 7,DA_386IGate, hwint15);
//	init_idt_desc(INT_VECTOR_SYS_CALL,DA_386IGate, sys_call);
	
	init_keyboard();
	init_floppy();
	init_clock();
}
