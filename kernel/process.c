#include <type.h>
#include <klibs.h>
#include <klibc.h>
#include <process.h>
#include <segment.h>
#include <clock.h>

PROCESS process_table[NR_PROCESS]={
{TASK_STACK_START,NULL,1,1,0,"main",1},
};
PROCESS *p_process_ready = process_table;
U32 int_reenter = 0;

typedef struct {
//	U16 gs;
//	U16 fs;
//	U16 es;
//	U16 ds;
	U32 edi;
	U32 esi;
	U32 ebp;
	U32 esp;
	U32 ebx;
	U32 edx;
	U32 ecx;
	U32 eax;
//	U32 err_code;
	U32 eip;
	U16 cs;
	U32 eflags;
}STACK_AFTER_INT;
static void init_process(PROCESS *p)
{
	U32 stack= TASK_STACK_START-p->pid*TASK_STACK_SIZE;
	
	stack -= sizeof(STACK_AFTER_INT);
	p->ticks = 5;
	p->priority = 5;
	p->state = PROCESS_RUNNING;
	((STACK_AFTER_INT*)stack)->eflags = 0x1202;
	((STACK_AFTER_INT*)stack)->cs = CODE_SEGMENT;
	((STACK_AFTER_INT*)stack)->eip = (U32)p->init_eip;
	p->esp = stack;
}
void creat_process(process proc,U32 stack_size)
{
	U32 i;
	PROCESS *p=NULL;

	for(i=1;i<NR_PROCESS;i++)
	{
		if(PROCESS_DEAD == process_table[i].state)
		{
			p = &process_table[i];
			p->pid = i;
			p->init_eip = proc;
			break;
		}
	}

	if(NULL == p)
	{
		put_str("creat_process error\n");
		return;
	}
	init_process(p);
}
