.text
/* 8259A interrupt controller ports. */
INT_M_CTL	= 0x20	/* I/O port for interrupt controller         <Master> */
INT_M_CTLMASK	= 0x21	/* setting bits in this port disables ints   <Master> */
INT_S_CTL	= 0xA0	/* I/O port for second interrupt controller  <Slave>  */
INT_S_CTLMASK	= 0xA1	/* setting bits in this port disables ints   <Slave>  */
INT_VECTOR_IRQ0	= 0x20
INT_VECTOR_IRQ8	= 0x28
EOI = 0x20

.global out_byte
.global in_byte
.global setup_irq
.global enable_irq
.global disable_irq
.global enable_int
.global disable_int
.global hwint00
.global hwint01
.global hwint02
.global hwint03
.global hwint04
.global hwint05
.global hwint06
.global hwint07
.global hwint08
.global hwint09
.global hwint10
.global hwint11
.global hwint12
.global hwint13
.global hwint14
.global hwint15
.global delay
.global memcpy
.global memset
.extern irq_table
.extern p_process_ready
.extern int_reenter

/* void output_byte( U32 p,U8 d)*/
out_byte:
	movl 4(%esp),%edx
	movb 8(%esp),%al
	outb %al,%dx
	nop
	nop

	ret

/* U8 in_byte(U32 port)*/
in_byte:
	movl 4(%esp),%edx
	xorl %eax,%eax
	inb  %dx,%al
	nop
	nop
	ret

/* void enable_irq(U32 irq) */
/*
 * if(irq < 8)
 * 	out_byte(INT_M_CTLMASK,in_byte(INT_M_CTLMASK) & ~(1<<irq))
 * else
 *	out_byte(INT_S_CTLMASK,in_byte(INT_S_CTLMASK) & ~(1<<irq))
 */
enable_irq:
	cli
	movl 4(%esp),%ecx
	pushf
	movb $0xFE,%ah
	rolb %cl,%ah
	cmpb $8,%cl
	jae  enable_8
enable_0:
	inb $INT_M_CTLMASK,%al
	andb %ah,%al
	outb %al,$INT_M_CTLMASK
	popf
	ret
enable_8:
	inb $INT_S_CTLMASK,%al
	andb %ah,%al
	outb %al,$INT_S_CTLMASK
	popf
	ret

/* void disable_irq(int irq)*/
disable_irq:
	movl 4(%esp),%ecx
	pushf
	cli
	movb $1,%ah
	rolb %cl,%ah
	cmpb $8,%cl
	jae disable_8
disable_0:
	inb $INT_M_CTLMASK,%al
	testb %ah,%al
	jnz dis_already
	orb %ah,%al
	outb %al,$INT_M_CTLMASK
	popf
	movl $1,%eax
	ret
disable_8:
	inb $INT_S_CTLMASK,%al
	testb %ah,%al
	jnz dis_already
	orb %ah,%al
	outb %al,$INT_S_CTLMASK
	popf
	movl $1,%eax
	ret
dis_already:
	popf
	xorl %eax,%eax
	ret
/*void enable_int(void)*/
enable_int:
	sti
	ret

/*void disable_int(void)*/
disable_int:
	cli
	ret

.macro hwint_master arg
	pushal
	movl  p_process_ready,%edi
	movl  %esp,(%edi)

	inb $(INT_M_CTLMASK),%al
	orb $(1 << (\arg)),%al
	outb %al,$(INT_M_CTLMASK)
	movb $(EOI),%al
	outb %al,$(INT_M_CTL)
	pushl $(\arg)
	call *(irq_table+(4*(\arg)))
	popl %ecx       /*恢复堆栈*/
	inb $(INT_M_CTLMASK),%al
	andb $~(1 << (\arg)),%al
	outb %al,$(INT_M_CTLMASK)

	movl p_process_ready,%edi
	movl (%edi),%esp
	popal
	decl int_reenter
	iret
.endm
.align 16
hwint00:
	hwint_master 0
.align 16
hwint01:
	hwint_master 1
.align 16
hwint02:
	hwint_master 2
.align 16
hwint03:
	hwint_master 3
.align 16
hwint04:
	hwint_master 4
.align 16
hwint05:
	hwint_master 5
.align 16
hwint06:
	hwint_master 6
.align 16
hwint07:
	hwint_master 7

.macro hwint_slave arg
	inb   $(INT_S_CTLMASK),%al
	orb   $(1 << (\arg-8)),%al
	outb  %al,$(INT_S_CTLMASK)
	movb  $(EOI),%al
	outb  %al,$(INT_S_CTL)
#	sti
	pushl $\arg
	call  *(irq_table+(4*(\arg)))
	popl  %ecx       /*恢复堆栈*/
	cli
	inb   $(INT_M_CTLMASK),%al
	andb  $~(1 << (\arg-8)),%al
	outb  %al,$(INT_S_CTLMASK)
	iret
.endm
.align 16
hwint08:
	hwint_slave 8
.align 16
hwint09:
	hwint_slave 9
.align 16
hwint10:
	hwint_slave 10
.align 16
hwint11:
	hwint_slave 11
.align 16
hwint12:
	hwint_slave 12
.align 16
hwint13:
	hwint_slave 13
.align 16
hwint14:
	hwint_slave 14
.align 16
hwint15:
	hwint_slave 15

/*void delay(S32 ms)*/
delay:
	pushl %ebp
	movl  %esp,%ebp
	subl  $8,%esp
	pushl  %ecx
	movl 4(%ebp),%ecx
repeat:
	nop
	nop
	subl $1,%ecx
	cmpl $1,%ecx
	jae  repeat
delay_ok:
	popl %ecx
	movl %ebp,%esp
	popl %ebp
	ret

/*void* memcpy(void *dest,void *src,U32 n)*/
memcpy:
	pushl %ebp
	movl  %esp,%ebp
	pushl %ecx
	pushl %edi
	pushl %esi
	movl  8(%ebp),%eax
	movl  %eax,%edi
	movl  12(%ebp),%esi
	movl  16(%ebp),%ecx
	rep
	movsb
	popl  %esi
	popl  %edi
	popl  %ecx
	movl  %esp,%ebp
	popl  %ebp
	ret
/*void *memset(void *dest,S32 c,U32 n)*/
memset:
	pushl %ebp
	movl  %esp,%ebp
	pushl %ecx
	pushl %edi
	movl  8(%ebp),%edi
	movl  12(%ebp),%eax
	movl  16(%ebp),%ecx
	rep
	stosb
	movl  8(%ebp),%eax
	popl  %edi
	popl  %ecx
	movl  %ebp,%esp
	popl  %ebp
	ret
	
	
