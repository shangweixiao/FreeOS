#ifndef _KLIBC_H_
#define _KLIBC_H_

#include "type.h"

S32 put_str(const S8 *s);
S32 put_char(U8 c);
S32 put_num(S32 d,U8 nbyte);
void reboot(void);
#endif
