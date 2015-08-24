KERNEL_SEG = 0x1000
KERNEL_OFT = 0x00
ROOT_DIR_SEC = 3962 /*BPB_FATSz32*2+BPB_RsvdSecCnt*/
ROOT_BUF_SEG = 0x8000
ROOT_BUF_OFG = 0x00   /*0x8000:0000*/
FAT_BUF_SEG = 0x8800 /*0x8800:0000*/
FAT_BUF_OFT = 0x00
FILE_NAME_LEN = 11
KERNEL_SEG = 0x1000
KERNEL_OFT = 0x00
.code16
.text
.global _start
_start:
	jmpl $0x9020,$start2
stop:	
	jmp stop
BPB_SecPerClus:
	.byte 8
BS_DrvNum:
	.byte 0x80
BPB_RsvdSecCnt:
	.long 32
current_data_sec:
	.long 3962
current_data_clus:
	.long 2
disk_address_packt:
DAP_size:
	.byte 0x10
DAP_Resverd:
	.byte 0
DAP_block_cnt:
	.word 0
DAP_buffer_oft:
	.word 0
DAP_buffer_seg:
	.word 0
DAP_block_num_l:
	.long 0
DAP_block_num_h:
	.long 0

start2:
	movw %cs,%ax
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%ss
	movw %ax,%fs
	movw $0xB000,%sp /*设置堆栈指针，任意地址,不冲突就行*/

	call display_load_msg
root_dir_find:
	/*读出根目录区到8000:0000*/
	movw $ROOT_BUF_SEG,%bx
	movw %bx,%es
	movw $ROOT_BUF_OFG,%bx
	movl current_data_sec,%eax
	movb BPB_SecPerClus,%cl
	call read_sector

	movw $ROOT_BUF_OFG,%di /*es:di*/
	movw $kernel_file,%si  /*fs:si*/
	subw %cx,%cx
	movb BPB_SecPerClus,%cl
	shlw $9,%cx            /*总字节数 512*BPB_SecPerClus */
	movw $FILE_NAME_LEN,%bx

file_name_cmp:
	cmpw  $0,%cx
	jz   next_root_clus
	movb %es:(%di),%al
	movb %fs:(%si),%dl
	cmpb %al,%dl
	jz   go_on_cmp
	subw $0x20,%cx
	andw $0xFFE0,%di
	addw $0x20,%di
	movw $FILE_NAME_LEN,%bx
	movw $kernel_file,%si
	jmp file_name_cmp
go_on_cmp:
	decw %bx
	cmpw $0,%bx
	jz   found_file
	incw %si
	incw %di
	jmp  file_name_cmp
next_root_clus:
	movl current_data_clus,%eax
	call get_next_entery
	cmpl $0x0FFFFFF7,%eax
	jae  stop
	movl %eax,current_data_clus
	decl %eax
	decl %eax     /* eax -= 2 */
	shll $3,%eax  /* eax *= BPB_SecPerClus */
	addl %eax,current_data_sec
	jmp  root_dir_find

found_file:
	andw $0xFFE0,%di
	addw $0x14,%di
	movw %es:(%di),%ax
	shll $16,%eax
	addw $0x06,%di
	movw %es:(%di),%ax

	movw $KERNEL_SEG,%bx
	movw %bx,%es
	movw $KERNEL_OFT,%bx /*[es:bx]: 0x9000:0200 == 0x9020:0000*/
read_file:
	pushl %eax

	decl %eax
	decl %eax  /* eax -= 2 */
	shll $3,%eax
	addl $ROOT_DIR_SEC,%eax

	movb BPB_SecPerClus,%cl
	call read_sector
	addw $(8*512),%bx /* BPB_SecPerClus * 8 */

	popl %eax
	call get_next_entery
	cmpl $0x0FFFFFF7,%eax
	jae  read_ok
	jmp read_file

read_ok:
	/* reset disc */
	xorw %ax,%ax
	xorw %dx,%dx
	int  $0x13

	/*获取内存数*/
	call get_memery

	/*打开A20地址线,Linux中使用的方法比较复杂*/
open_a20:
	cli
	inb  $0x92,%al
	orb  $0x02,%al
	outb %al,$0x92
	/*计算出gdt的绝对地址，并将其写到gdt_ptr+2位置，然后加载gdt*/
load_gdt: 
	xorl %eax,%eax
	movw %ds,%ax
	shll $4,%eax
	addl $gdt,%eax
	movl %eax,(gdt_ptr+2)
	lgdt gdt_ptr
move_code32:
	movw $KERNEL_SEG,%ax
	movw %ax,%ds
	movw $0,%ax
	movw %ax,%es
	xorl %esi,%esi
	xorl %edi,%edi
	movl $0x14000,%ecx /*移动320KB*/
	rep
	movsl
	/*设置PE位，跳转到32位代码*/
jump_to_code32:
	movl %cr0,%eax
	or $1,%eax
	movl %eax,%cr0
	/*使用jmpl和下面的直接写指令码方法似乎都可行
	 *Linux中使用的是直接写指令码的方法
	 */
#	jmpl $0x08,$0x1000 

	.byte 0x66,0xea
	.long 0x0000
	.word 0x8


/*
 * 获取内存数,放到内存0x5000:0000之后的1KB空间内
 */
 MemChkBufSeg=0x5000
 MemChkBufOft=0
 dwMCRNumber=(1020)
get_memery:
	movw $MemChkBufSeg,%ax
	movw %ax,%es

clear_zone:
	movw $MemChkBufOft,%di
	movw $1024,%cx
	movw $0,%es:(%di)
	subw $1,%cx
	addw $2,%di
	cmpw $512,%cx
	jbe  clear_zone

	movl $0,%ebx
	movw $MemChkBufOft,%di
continue:
	xorl %eax,%eax
	movl $0x0000E820,%eax
	movl $20,%ecx
	movl $0x534D4150,%edx
	int  $0x15
	jc   memery_get_failure
	addw $20,%di
	addl $1,%es:dwMCRNumber
	cmpl $0,%ebx
	jne  continue
	jmp  memery_get_ok
memery_get_failure:
	movl $0,%es:dwMCRNumber
	jmp memery_get_ok
memery_get_ok:
	ret

/*
 * 从第eax个sector开始，读cl个sector到es:bx
 */
read_sector:
	pushw %si
	pushw %cx
	pushw %dx

	subb %ch,%ch
	movw %cx,DAP_block_cnt
	movl %eax,DAP_block_num_l
	movw %es,DAP_buffer_seg
	movw %bx,DAP_buffer_oft
	movw $disk_address_packt,%si

	movb  BS_DrvNum,%dl
go_on_reading:
	movb  $0x42,%ah
	int   $0x13
	jc   go_on_reading

	call display_point /*显示一个点*/

	popw %dx
	popw %cx
	popw %si
	ret

/* eax: 当前簇号*/
get_next_entery:
	pushl %ecx
	pushl %ebx
	pushl %edx
	pushw %es

	movw $FAT_BUF_SEG,%cx
	movw %cx,%es
	movw $FAT_BUF_OFT,%bx  /*读出FAT1*/
	shll $2,%eax      /* eax*4,每条记录占四个字节 */
	movl %eax,%edx
	shrl $9,%eax      /* eax>>9 == eax/512 */
	andl $511,%edx    /* %eax % (512-1) */
	subl %ecx,%ecx
	movw BPB_RsvdSecCnt,%cx
	addl %ecx,%eax
	movb $1,%cl
	call read_sector

	movl %es:(%edx),%eax

	popw %es
	popl %edx
	popl %ebx
	popl %ecx
	ret

display_load_msg:
	pushw %ax
	pushw %bx
	pushw %cx
	pushw %dx
	pushw %bp

	movw $load_msg,%ax
	movw %ax,%bp
	movw $7,%cx
	movw $0x1301,%ax
	movw $0x0F,%bx
	movw $0x0100,%dx /* dh:行号，dl:列号*/
	int  $0x10

	popw %bp
	popw %dx
	popw %cx
	popw %bx
	popw %ax
	ret
display_point:
	pushw %ax
	pushw %bx
	movb  $0x0E,%ah
	movb  $'.',%al
	movw  $0x0F,%bx
	int   $0x10
	popw %bx
	popw %ax
	ret

kernel_file:
	.ascii "KERNEL  BIN"
load_msg:
	.ascii "Loading"
display_data:
	.byte 0

	.align 16
gdt:
	.word 0,0,0,0 /*这一项不用，Intel要求这样做*/
label_desc_code32: /*代码段，起始地址0x0000,大小4GB,只读，可执行*/
	.word 0xFFFF,0x0000,0x9A00,0x00CF #Code
label_desc_data32:/*数据段，起始地址0x0000,大小4GB,可读写*/
	.word 0xFFFF,0x0000,0x9200,0x00CF #Data
label_desc_video:/*视频段，起始地址0xB8000(BIOS视频内存地址),大小1MB，可读写*/
	.word 0x0100,0x8000,0x920B,0x00C0 #Video
label_desc_stack:/*堆栈段，起始地址0x00000,大小4GB，向下扩展，可读写*/
	.word 0xFFFF,0x0000,0x9200,0x00CF #stack
gdt_end:
	.align 4
gdt_ptr:
	.word gdt_end-gdt-1
	.long 0
