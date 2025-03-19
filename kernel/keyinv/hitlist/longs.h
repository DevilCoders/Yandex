#pragma once

#include <library/cpp/packedtypes/longs.h>

extern char *i386unp_bufba[256];
#define ASMi386_UNPACK_ADD_64(Cur, Sum)     \
    asm (                                   \
        "movzbl (%0),%%eax; incl %0; \n"    \
        "call *i386unp_bufba(,%%eax,4) \n"  \
        "addl %%eax,%1; \n"                 \
        "adcl %%edx,%2; \n"                 \
        : "+c" (Cur), "=m" (Lo32(Sum)), "=m" (Hi32(Sum)) \
        : /* "0" (Cur), "m" (Lo32(Sum)), "m" (Hi32(Sum)) */ \
        : "cc", "eax", "edx")

#define ASMi386_UNPACK_64(Cur, Sum)         \
    asm (                                   \
        "movzbl (%0),%%eax; incl %0; \n"    \
        "call *i386unp_bufba(,%%eax,4) \n"  \
        "movl %%eax,%1; \n"                 \
        "movl %%edx,%2; \n"                 \
        : "+c" (Cur), "+m" (Lo32(Sum)), "+m" (Hi32(Sum)) \
        : /* "0" (Cur), "1" (Lo32(Sum)), "2" (Hi32(Sum))*/ \
        : "cc", "eax", "edx")

extern char *i64unp_bufba[256];
#define ASMi64_UNPACK_ADD_64(Cur, Sum)      \
    asm (                                   \
        "movzbl (%0),%%eax; incq %0; \n"    \
        "call *i64unp_bufba(,%%rax,8) \n"   \
        "addq %%rax,%1; \n"                 \
        : "+c" (Cur), "+g" (Sum) \
        :  \
        : "cc", "rax", "rdx")

#define ASMi64_UNPACK_64(Cur, Val)          \
    asm (                                   \
        "movzbl (%0),%%eax; incq %0; \n"    \
        "call *i64unp_bufba(,%%rax,8) \n"   \
        : "+c" (Cur), "=a" (Val) \
        : \
        : "cc", "rdx")
