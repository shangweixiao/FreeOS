#ifndef _TYPE_H_
#define _TYPE_H_

typedef unsigned int U32;
typedef int          S32;
typedef unsigned short U16;
typedef short        S16;
typedef unsigned char U8;
typedef char         S8;

typedef void (*int_handler)(void);
typedef void (*irq_handler)(S32 irq);
typedef void (*process)(void);
#define NULL ((void*)0)
#define TRUE 1
#define FALSE 0
#endif
