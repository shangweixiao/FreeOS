BOOTSEG = 0x07C0
INITSEG = 0x9000
SETUPSEG = 0x9020
SYSSEG = 0x1000
ROOT_DIR_CNT = 14
ROOT_DIR_SEC = 3962 /*BPB_FATSz32*2+BPB_RsvdSecCnt*/
ROOT_BUF_SEG = 0x8000
ROOT_BUF_OFG = 0x00   /*0x8000:0000*/
FAT_BUF_SEG = 0x8800 /*0x8800:0000*/
FAT_BUF_OFT = 0x00
FILE_NAME_LEN = 11
.code16
.text
.global _start
_start:
	jmp start2
	nop

BS_OEMName:
	.ascii "12345678"
BPB_BytsPerSec:
	.word 512
BPB_SecPerClus:
	.byte 8
BPB_RsvdSecCnt:
	.word 32
BPB_NumFATs:
	.byte 2
BPB_RootEntCnt:
	.word 0
BPB_TotSec16:
	.word 0
BPB_Media:
	.byte 0xF8
BPB_FATSz16:
	.word 0
BPB_SecPerTrk:
	.word 62
BPB_NumHeads:
	.word 32
BPB_HiddSec:
	.long 0
BPB_TotSec32:
	.long 2015232
BPB_FATSz32:
	.long 1965
BPB_ExtFlags:
	.word 0
BPB_FSVer:
	.word 0
BPB_RootClus:
	.long 2
BPB_FSInfo:
	.word 1
BPB_BkBootSec:
	.word 6
BPB_Reserved:
	.ascii "0123456789AB"
BS_DrvNum:
	.byte 0x80
BS_Reserved1:
	.byte 0
BS_BootSig:
	.byte 29
BS_VolID:
	.long 1256610946
BS_VolLab:
	.ascii "shangwx    "
BS_FilSysType:
	.ascii "FAT32   "

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
	movw $BOOTSEG,%ax
	movw %ax,%ds
	movw $INITSEG,%ax
	movw %ax,%es
	movw $256,%cx
	subw %si,%si
	subw %di,%di
	rep
	movsw
	jmpl $INITSEG,$go
go:
	movw %cs,%ax
	movw %ax,%ds
	movw %ax,%ss
	movw %ax,%fs
	movw $0xB000,%sp

display_boot_msg:
	movw $boot_msg,%ax
	movw %ax,%bp
	movw $10,%cx
	movw $0x1301,%ax
	movw $0x0F,%bx
	subw %dx,%dx
	int  $0x10

root_dir_find:
	/*读出根目录区到8000:0000*/
	movw $ROOT_BUF_SEG,%bx
	movw %bx,%es
	movw $ROOT_BUF_OFG,%bx
	movl current_data_sec,%eax
	movb BPB_SecPerClus,%cl
	call read_sector

	movw $ROOT_BUF_OFG,%di /*es:di*/
	movw $loader_file,%si  /*fs:si*/
	subw %cx,%cx
	movb BPB_SecPerClus,%cl
	shlw $9,%cx            /*总字节数 512*BPB_SecPerClus */
	movw $FILE_NAME_LEN,%bx

file_name_cmp:
	jcxz   next_root_clus
	movb %es:(%di),%al
	movb %fs:(%si),%dl
	cmpb %al,%dl
	jz   go_on_cmp
	subw $0x20,%cx
	andw $0xFFE0,%di
	addw $0x20,%di
	movw $FILE_NAME_LEN,%bx
	movw $loader_file,%si
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

	movw $SETUPSEG,%bx
	movw %bx,%es
	subw %bx,%bx          /*es:bx == 0x9000:0000*/
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
	jmpl $0x9020,$0x0000
stop:
	jmp stop
/*
 * 从第eax个sector开始，读cl个sector到es:bx
 */
read_sector:
	pushl %edx
	pushl %ecx

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

	popl %ecx
	popl %edx
	ret

/* eax: 当前簇号*/
get_next_entery:
	pushl %ebx
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
	popl %ebx
	ret
/*
display:
	pushw %bx
	movw $0xB800,%bx
	movw %bx,%gs
	movb display_data,%bl
	movb $0x0C,%bh
	movw %bx,%gs:0
	popw %bx
	ret

display_data:
	.byte 0
*/
boot_msg:
	.ascii "Booting..."
loader_file:
	.asciz "LOADER  BIN"

	.org 510
	.word 0xAA55

