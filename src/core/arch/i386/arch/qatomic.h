/****************************************************************************
**
** Definition of q_cas_* functions.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the tools module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QATOMIC_P_H
#define QATOMIC_P_H

#ifndef QT_H
#  include <qglobal.h>
#include <stdio.h>
#endif // QT_H

#if defined(Q_CC_GNU) || (defined(Q_OS_UNIX) && defined(Q_CC_INTEL))

inline int q_cas_32(volatile int *ptr, int expected, int newval)
{
    asm volatile ("lock cmpxchgl %1,%2"
	: "=a" (newval)
	: "q" (newval), "m" (*ptr), "0" (expected)
	: "memory");
    return newval;
}

inline void *q_cas_ptr(void * volatile *ptr, void *expected, void *newval)
{
    asm volatile ("lock cmpxchgl %1,%2"
	: "=a" (newval)
	: "q" (newval), "m" (*ptr), "0" (expected)
	: "memory");
    return newval;
}

#define Q_HAVE_ATOMIC_INCDEC

inline int q_atomic_increment(volatile int *ptr)
{
    unsigned char ret;
    asm volatile("lock incl %0; sete %1"
		 : "=m" (*ptr), "=qm" (ret)
		 : "m" (*ptr)
		 : "memory");
    return ret == 0;
}

inline int q_atomic_decrement(volatile int *ptr)
{
    unsigned char ret;
    asm volatile("lock decl %0; sete %1"
		 : "=m" (*ptr), "=qm" (ret)
		 : "m" (*ptr)
		 : "memory");
    return ret == 0;
}

#define Q_HAVE_ATOMIC_SETPOINTER

inline void *q_atomic_set_pointer(void * volatile *ptr, void *newval)
{
    asm volatile("xchgl %0,%1"
		 : "=r" (newval)
		 : "m" (*ptr), "0" (newval)
		 : "memory");
    return newval;
}


#elif defined(Q_OS_WIN) && (defined(Q_CC_MSVC) || defined(Q_CC_INTEL))

inline int q_cas_32(volatile int *pointer, int expected, int newval)
{
    __asm {
	mov EBX,pointer
	mov EAX,expected
	mov ECX,newval
        lock cmpxchg dword ptr[EBX],ECX
        mov newval,EAX
    }
    return newval;
}

inline void *q_cas_ptr(void * volatile *pointer, void *expected, void *newval)
{
    __asm {
	mov EBX,pointer
	mov EAX,expected
	mov ECX,newval
        lock cmpxchg dword ptr[EBX],ECX
	mov newval,EAX
    }
    return newval;
}

#else

// compiler doesn't support inline assembly
extern "C" {
    Q_KERNEL_EXPORT int q_cas_32(volatile int *ptr, int expected, int newval);
    Q_KERNEL_EXPORT void *q_cas_ptr(void * volatile *ptr, void *expected, void *newval);
}

#endif

#endif // QATOMIC_P_H
