#include <type.h>
#include <8259A.h>
#include <klibs.h>
#include <protect.h>




irq_handler irq_table[NR_IRQ];


void init_8259A(void)
{
	out_byte((U32)INT_M_CTL,  0x11);			// Master 8259, ICW1.
	out_byte((U32)INT_S_CTL,	0x11);			// Slave  8259, ICW1.
	out_byte((U32)INT_M_CTLMASK,	INT_VECTOR_IRQ0);	// Master 8259, ICW2.
	out_byte((U32)INT_S_CTLMASK,	INT_VECTOR_IRQ8);	// Slave  8259, ICW2.
	out_byte((U32)INT_M_CTLMASK,	0x4);			// Master 8259, ICW3.
	out_byte((U32)INT_S_CTLMASK,	0x2);			// Slave  8259, ICW3.
	out_byte((U32)INT_M_CTLMASK,	0x1);			// Master 8259, ICW4.
	out_byte((U32)INT_S_CTLMASK,	0x1);			// Slave  8259, ICW4.
	out_byte((U32)INT_M_CTLMASK,	0xFF);	// Master 8259, OCW1. 
	out_byte((U32)INT_S_CTLMASK,	0xFF);	// Slave  8259, OCW1.	
	
}
void set_irq_handler(S32 irq, irq_handler handler)
{
	disable_irq(irq);
	irq_table[irq] = handler;
}

