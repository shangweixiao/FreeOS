#ifndef _PROCESS_H_
#define _PROCESS_H_
typedef struct {
	U32 gs;
	U32 fs;
	U32 es;
	U32 ds;
	U32 edi;
	U32 esi;
	U32 ebp;
	U32 kernel_esp;
	U32 ebx;
	U32 edx;
	U32 ecx;
	U32 eax;
	U32 retaddr;
	U32 eip;
	U32 cs;
	U32 eflags;
	U32 esp;
	U32 ss;
}STACK_FRAME;

typedef struct {
//	STACK_FRAME regs;
	U32 esp;
	process init_eip;
	U32 ticks;
	U32 priority;
	U32 pid;
	S8 name[32];
	S8 state;
}PROCESS;

typedef struct {
	process init_eip;
	U32 stack_size;
	S8 name[16];
}TASK;

#define NR_PROCESS 64
#define TASK_STACK_SIZE (64*1024)
#define TASK_STACK_START 0xFFFFF0
#define PROCESS_DEAD 0
#define PROCESS_RUNNING 1
#define PROCESS_INTERRUPTIBLE 2
#define PROCESS_UNINTERRUPTIBLE 4
void creat_process(process pro,U32 stack_size);

#endif
