#include <type.h>
#include <klibs.h>
#include <klibc.h>
#include <8259A.h>
#include <clock.h>
#include <process.h>

/* from processs.c */
extern PROCESS process_table[NR_PROCESS];
extern PROCESS *p_process_ready;

struct timer_list *next_timer = NULL;
U32 ticks;
static void do_timer(void);

void clock_handler(S32 irq)
{
	ticks += 1;
	p_process_ready->ticks -= 1;
	do_timer();
	if(0 != p_process_ready->ticks)
	{
		return;
	}
	schedule();
}
void init_clock(void)
{
	out_byte((U32)TIMER_MODE,RATE_GENERATOR);
	out_byte((U32)TIMER0,(U8)(TIMER_FREQ/HZ));
	out_byte((U32)TIMER0,(U8)((TIMER_FREQ/HZ)>>8));

	set_irq_handler(CLOCK_IRQ,clock_handler);
	enable_irq(CLOCK_IRQ);
}

void schedule()
{
	U32 i,greatest_ticks = 0;
	U32 found;

	while(0 == greatest_ticks)
	{
		/*查找ticks最大的那个进程*/
		for(i=1;i<NR_PROCESS;i++)
		{
			if(PROCESS_RUNNING == process_table[i].state)
			{
				if(process_table[i].ticks > greatest_ticks)
				{
					greatest_ticks = process_table[i].ticks;
					p_process_ready = &process_table[i];
				}
			}
		}

		/*所有进程的ticks都为0，重新给ticks赋值*/
		if(0 == greatest_ticks)
		{
			for(i=1;i<NR_PROCESS;i++)
			{
				found = 0;
				if(PROCESS_RUNNING == process_table[i].state)
				{
					process_table[i].ticks = process_table[i].priority;
					found = 1;
				}
			}
		}
		if(0 == found)
		{
			process_table[0].ticks = process_table[0].priority;
			p_process_ready = &process_table[0];

			break;
		}
	}	
}

void add_timer(struct timer_list *timer)
{
	struct timer_list **p;

	if(NULL == timer)
	{
		return;
	}
	timer->next = NULL;
	p = &next_timer;
	disable_int();
	while(*p)
	{
		if((*p)->expires > timer->expires)
		{
			(*p)->expires -= timer->expires;
			timer->next = *p;
			break;
		}
		timer->expires -= (*p)->expires;
		p = &(*p)->next;
	}
	*p = timer;

	enable_int();
}

S32 del_timer(struct timer_list *timer)
{
	struct timer_list **p;
	U32 expires = 0;

	p = &next_timer;
	disable_int();
	while(*p)
	{
		if(*p == timer)
		{
			if(NULL != (*p = timer->next))
			{
				(*p)->expires += timer->expires;
			}
			timer->expires += expires;
			return(1);
		}
		expires += (*p)->expires;
		p = &(*p)->next;
	}
	enable_int();
	return(0);
}

static void do_timer(void)
{
	if(next_timer)
	{
		next_timer->expires -= 1;
		while(next_timer && (next_timer->expires <= 0))
		{
			next_timer->function(next_timer->data);
			next_timer->function = NULL;
			next_timer = next_timer->next;
		}
	}
}
