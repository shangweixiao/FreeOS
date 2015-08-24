#include <type.h>
#include <klibc.h>
#include <klibs.h>

static U8 *vidmem = (U8*)0xB8000;
#define LINES 25
#define COLS  80
struct {
	S32 orig_x;
	S32 orig_y;
}screen_info={0,0};
/*
void *memcpy(void *dest,void *src,U32 n)
{
	S32 i;
	S8 *d=(S8*)dest,*s=(S8*)src;

	for(i=0;i<n;i++)
	{
		d[i] = s[i];
	}
	return(dest);
}
*/
static void scroll(void)
{
	S32 i;

	memcpy(vidmem,vidmem+COLS*2,(LINES-1)*COLS*2);
	for(i=(LINES-1)*COLS*2;i<LINES*COLS*2;i+=2)
	{
		vidmem[i] = ' ';
	}
}
S32 put_str(const S8 *s)
{
	S32 x,y;
	U8 c;

	x = screen_info.orig_x;
	y = screen_info.orig_y;

	while ( ( c = *s++ ) != '\0' )
	{
		if ( c == '\n' )
		{
			x = 0;
			if ( ++y >= LINES )
			{
				scroll();
				y--;
			}
		} 
		else 
		{
			vidmem [ ( x + COLS * y ) *2 + 1 ] = 0x0F;
			vidmem [ ( x + COLS * y ) * 2 ] = c; 
			if ( ++x >= COLS )
			{
				x = 0;
				if ( ++y >= LINES )
				{
					scroll();
					y--;
				}
			}
		}
	}

	screen_info.orig_x = x;
	screen_info.orig_y = y;
	return(0);
}
S32 put_char(U8 c)
{
	int x,y;

	x = screen_info.orig_x;
	y = screen_info.orig_y;

	if('\n' != c)
	{
		vidmem [ ( x + COLS * y ) *2 + 1 ] = 0x0F;
		vidmem [ ( x + COLS * y ) * 2 ] = c;
		x ++;
		if(x >= COLS)
		{
			y ++;
			x = 0;
		}
	}
	else
	{
		y ++;
		x = 0;
	}
	
	if(LINES <= y)
	{
		scroll();
		y --;
	}
	screen_info.orig_x = x;
	screen_info.orig_y = y;
	return(0);
}
S32 put_num(S32 d,U8 nbyte)
{
	S32 i,t;
	S32 f=(4-nbyte)*2;

	if(4 < nbyte)
	{
		f = 0;
	}
	else if(0 == nbyte)
	{
		f = -1;
	}
	put_char('0');
	put_char('x');
	for(i=0;i<8;i++)
	{
		t = (d << (i*4)) & 0xF0000000;
		t = (t >> 28) & 0x0F;
		if(1 <= f)
		{
			f -= 1;
			continue;
		}
		if((0 == nbyte) && (-1 == f))
		{
			if(0 == t)
			{
				continue;
			}
		}
		f = 0;
		if(t < 10)
		{
			t += '0';
		}
		else
		{
			t += ('A'-10);
		}
		put_char(t);
	}
	if(-1 == f)
	{
		put_char('0');
		put_char('0');
	}
	return(0);
}
/*================= REBOOT ==========================*/
struct desc_ptr {
	unsigned short size;
	unsigned long address;
} __attribute__((packed)) ;
static unsigned long long
real_mode_gdt_entries [3] =
{
	0x0000000000000000ULL,	/* Null descriptor */
	0x00009a000000ffffULL,	/* 16-bit real-mode 64k code at 0x00000000 */
	0x000092000100ffffULL	/* 16-bit real-mode 64k data at 0x00000100 */
};

static struct desc_ptr
real_mode_gdt = { sizeof (real_mode_gdt_entries) - 1, (long)real_mode_gdt_entries },
real_mode_idt = { 0x3ff, 0 };
static unsigned char real_mode_switch [] =
{
	0x66, 0x0f, 0x20, 0xc0,			/*    movl  %cr0,%eax        */
	0x66, 0x83, 0xe0, 0x11,			/*    andl  $0x00000011,%eax */
	0x66, 0x0d, 0x00, 0x00, 0x00, 0x60,	/*    orl   $0x60000000,%eax */
	0x66, 0x0f, 0x22, 0xc0,			/*    movl  %eax,%cr0        */
	0x66, 0x0f, 0x22, 0xd8,			/*    movl  %eax,%cr3        */
	0x66, 0x0f, 0x20, 0xc3,			/*    movl  %cr0,%ebx        */
	0x66, 0x81, 0xe3, 0x00, 0x00, 0x00, 0x60,	/*    andl  $0x60000000,%ebx */
	0x74, 0x02,				/*    jz    f                */
	0x0f, 0x09,				/*    wbinvd                 */
	0x24, 0x10,				/* f: andb  $0x10,al         */
	0x66, 0x0f, 0x22, 0xc0			/*    movl  %eax,%cr0        */
};
static unsigned char jump_to_bios [] =
{
	0xea, 0x00, 0x00, 0xff, 0xff		/*    ljmp  $0xffff,$0x0000  */
};
static inline void native_load_gdt(const struct desc_ptr *dtr)
{
	asm volatile("lgdt %0"::"m" (*dtr));
}

static inline void native_load_idt(const struct desc_ptr *dtr)
{
	asm volatile("lidt %0"::"m" (*dtr));
}
void reboot(void)
{
	disable_int();
	CMOS_WRITE(0x00,0x0F);

	memcpy((void *)(0x1000 - sizeof(real_mode_switch) - 100),
		real_mode_switch, sizeof (real_mode_switch));
	memcpy((void *)(0x1000 - 100), jump_to_bios, sizeof(jump_to_bios));
	native_load_idt(&real_mode_idt);

	native_load_gdt(&real_mode_gdt);
	__asm__ __volatile__ ("movl $0x0010,%%eax\n"
				"\tmovl %%eax,%%ds\n"
				"\tmovl %%eax,%%es\n"
				"\tmovl %%eax,%%fs\n"
				"\tmovl %%eax,%%gs\n"
				"\tmovl %%eax,%%ss" : : : "eax");

	__asm__ __volatile__ ("ljmp $0x0008,%0"
				:
				: "i" ((void *)(0x1000 - sizeof (real_mode_switch) - 100)));
}
/*================= REBOOT ==========================*/


