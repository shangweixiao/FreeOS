#ifndef _CLOCK_H_
#define _CLOCK_H_

#define TIMER0 0x40
#define TIMER_MODE 0x43
#define RATE_GENERATOR 0x34
#define TIMER_FREQ 1193182L
#define HZ 1000

struct timer_list {
	struct timer_list *next;
	struct timer_list *prev;
	S32 expires;
	U32 data;
	void (*function)(U32);
};

void init_clock(void);
void schedule(void);
void add_timer(struct timer_list *timer);
S32 del_timer(struct timer_list *timer);
#endif
