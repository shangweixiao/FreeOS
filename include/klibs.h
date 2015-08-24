#ifndef _KLIBS_H_
#define _KLIB_H_

void out_byte(U32 p,U8 d);
U8 in_byte(U32 p);
void setup_irq(U32 no,void *pf);
void enable_irq(U32 irq);
void disable_irq(U32 irq);
void enable_int(void);
void disable_int(void);
void hwint00(void);
void hwint01(void);
void hwint02(void);
void hwint03(void);
void hwint04(void);
void hwint05(void);
void hwint06(void);
void hwint07(void);
void hwint08(void);
void hwint09(void);
void hwint10(void);
void hwint11(void);
void hwint12(void);
void hwint13(void);
void hwint14(void);
void hwint15(void);
void delay(S32 ms);
void* memcpy(void *dest,void *src,U32 n);
void* memset(void *dest,S32 c,U32 n);

#define CMOS_READ(addr) ({ \
out_byte(0x70,addr|0x80); \
in_byte(0x71); \
})
#define CMOS_WRITE(val, addr) ({ \
out_byte(0x70,addr|0x80); \
out_byte(0x71,val); \
})
#endif
