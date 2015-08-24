#include <type.h>
#include <8259A.h>
#include <klibc.h>
#include <klibs.h>
#include <protect.h>
#include <floppy.h>
#include <process.h>
#include <clock.h>

#define MCRNumber (0x50000+1020)//(394236) // 0x500+1020
#define MemChkBuf (0x50000)

S8 *title="BaseAddrLow   BaseAddrHigh   LengthLow   LengthHigh   Type";
S8 *mem_size="memery size: ";

static void delay_tsk(U32 m)
{
	int i,j;
	for(i=0;i<m;i++)
	{
		for(j=0;j<10000;j++)
		{
			delay(1000);
		}
	}
}
void put_memery_info(U32 no)
{
	S32 i,j;
	S32 sum=0;

	put_str(title);
	put_char('\n');

	for(i=0;i<*(S32*)MCRNumber;i++)
	{
		for(j=0;j<5;j++)
		{
			put_num(((U32*)MemChkBuf+i*5)[j],4);
			put_char(' ');
			put_char(' ');
		}
		if(1 == (((U32*)MemChkBuf+i*5)[4]))
		{
			sum += (((U32*)MemChkBuf+i*5)[2]);
		}
		put_char('\n');
	}
	put_char('\n');
	sum >>= 20;
	put_str(mem_size);
	put_num(sum,2);
	put_str(" MB\n");
}
struct timer_list timer;
void process1(void);
void process2(void);
int main()
{
	put_str("=============== main.c ===============\n");
	//put_memery_info(0);
	init_sys();

	creat_process(process1,TASK_STACK_SIZE);
	creat_process(process2,TASK_STACK_SIZE);

	enable_int();
	timer.expires = 10000;
	timer.function = put_memery_info;
	add_timer(&timer);
	while(1)
	{
		asm("pause\n\t");
	}
}

void process1(void)
{
	put_str("process1 start\n");
	while(1)
	{
		put_str(" process1 running ");
		delay_tsk(13);
	}


}

void process2(void)
{
	put_str("process2 start\n");
	while(1)
	{
		put_str(" process2 running ");
		delay_tsk(33);
	}

}
