/*
 * 编译：as -o code32test.o code32test.s
 * 连接：ld -Ttext 0x0  -s --oformat binary -o code32test code32test.o test.o
 * 将code32test文件从IMG文件偏移2048Byte处开始写入
 */
DATA_SEGMENT = 0x08
CODE_SEGMENT = 0x10
VIDEO_SEGMENT = 0x18
STACK_SEGMENT = 0x20

.section .text
.extern main
.extern put_num
.global test_var
.global _idt

.global _start
_start:
label_seg_code32:
	movw $0x10,%ax
	movw %ax,%es
	movw %ax,%ds

	call setup_idt
	lidt idt_ptr
	lgdt gdt_ptr
	ljmp $CODE_SEGMENT,$new
new:
	movw $DATA_SEGMENT,%ax
	movw %ax,%es
	movw %ax,%ds
	movw %ax,%fs
	movw %ax,%gs
	movw %ax,%ss
	movl $0xFFFFF0,%esp

	pushl $repeat
	pushl $main
	ret
repeat:	jmp repeat
/*设置IDT表*/
setup_idt:
	leal ignore_int,%edx
	movl $(CODE_SEGMENT<<16),%eax
	movw %dx,%ax
	movw $0x8E00,%dx
	leal _idt,%edi
	movl $256,%ecx
rp_sidt:
	movl %eax,(%edi)
	movl %edx,4(%edi)
	addl $8,%edi
	dec  %ecx
	jne  rp_sidt
	ret
ignore_int:
	cld
	iret
	movb  $0x0C,%ah
	movb  $0x41,%al
	movw  %ax,%gs:0
	movb  $0x20,%al
	outb  %al,$0x20
	iret

	.align 8
_idt:
	.fill 256,8,0
_gdt:
	.word 0,0,0,0 /*这一项不用，Intel要求这样做*/
label_desc_data32:/*数据段，起始地址0x0000,大小4GB,可读写*/
	.word 0xFFFF,0x0000,0x9200,0x00CF #Data
label_desc_code32: /*代码段，起始地址0x0000,大小4GB,只读，可执行*/
	.word 0xFFFF,0x0000,0x9A00,0x00CF #Code
label_desc_other:
	.fill 253,8,0
gdt_end:

	.align 4
idt_ptr:
	.word 256*8-1
	.long _idt
gdt_ptr:
	.word 256*8-1
	.long _gdt
