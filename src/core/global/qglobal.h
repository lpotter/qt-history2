/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QGLOBAL_H
#define QGLOBAL_H

#include <stddef.h>

#if defined(__cplusplus)
#include <new>
#endif


#define QT_VERSION_STR   "4.0.0-tp2"
/*
   QT_VERSION is (major << 16) + (minor << 8) + patch.
 */
#define QT_VERSION 0x040000

#if !defined(QT_BUILD_MOC)
#include "qconfig.h"
#endif

/*
   The operating system, must be one of: (Q_OS_x)

     DARWIN   - Darwin OS (synonym for Q_OS_MAC)
     MSDOS    - MS-DOS and Windows
     OS2      - OS/2
     OS2EMX   - XFree86 on OS/2 (not PM)
     WIN32    - Win32 (Windows 95/98/ME and Windows NT/2000/XP)
     CYGWIN   - Cygwin
     SOLARIS  - Sun Solaris
     HPUX     - HP-UX
     ULTRIX   - DEC Ultrix
     LINUX    - Linux
     FREEBSD  - FreeBSD
     NETBSD   - NetBSD
     OPENBSD  - OpenBSD
     BSDI     - BSD/OS
     IRIX     - SGI Irix
     OSF      - HP Tru64 UNIX
     SCO      - SCO OpenServer 5
     UNIXWARE - UnixWare 7, Open UNIX 8
     AIX      - AIX
     HURD     - GNU Hurd
     DGUX     - DG/UX
     RELIANT  - Reliant UNIX
     DYNIX    - DYNIX/ptx
     QNX      - QNX
     QNX6     - QNX RTP 6.1
     LYNX     - LynxOS
     BSD4     - Any BSD 4.4 system
     UNIX     - Any UNIX BSD/SYSV system
*/

#if defined(__APPLE__) && (defined(__GNUC__) || defined(__xlC__) || defined(__xlc__))
#  define Q_OS_DARWIN
#elif defined(__CYGWIN__)
#  define Q_OS_CYGWIN
#elif defined(MSDOS) || defined(_MSDOS)
#  define Q_OS_MSDOS
#elif defined(__OS2__)
#  if defined(__EMX__)
#    define Q_OS_OS2EMX
#  else
#    define Q_OS_OS2
#  endif
#elif !defined(SAG_COM) && (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
#  define Q_OS_WIN32
#  define Q_OS_WIN64
#elif !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#  define Q_OS_WIN32
#elif defined(__MWERKS__) && defined(__INTEL__)
#  define Q_OS_WIN32
#elif defined(__sun) || defined(sun)
#  define Q_OS_SOLARIS
#elif defined(hpux) || defined(__hpux)
#  define Q_OS_HPUX
#elif defined(__ultrix) || defined(ultrix)
#  define Q_OS_ULTRIX
#elif defined(sinix)
#  define Q_OS_RELIANT
#elif defined(__linux__) || defined(__linux)
#  define Q_OS_LINUX
#elif defined(__FreeBSD__) || defined(__DragonFly__)
#  define Q_OS_FREEBSD
#  define Q_OS_BSD4
#elif defined(__NetBSD__)
#  define Q_OS_NETBSD
#  define Q_OS_BSD4
#elif defined(__OpenBSD__)
#  define Q_OS_OPENBSD
#  define Q_OS_BSD4
#elif defined(__bsdi__)
#  define Q_OS_BSDI
#  define Q_OS_BSD4
#elif defined(__sgi)
#  define Q_OS_IRIX
#elif defined(__osf__)
#  define Q_OS_OSF
#elif defined(_AIX)
#  define Q_OS_AIX
#elif defined(__Lynx__)
#  define Q_OS_LYNX
#elif defined(__GNU_HURD__)
#  define Q_OS_HURD
#elif defined(__DGUX__)
#  define Q_OS_DGUX
#elif defined(__QNXNTO__)
#  define Q_OS_QNX6
#elif defined(__QNX__)
#  define Q_OS_QNX
#elif defined(_SEQUENT_)
#  define Q_OS_DYNIX
#elif defined(_SCO_DS) /* SCO OpenServer 5 + GCC */
#  define Q_OS_SCO
#elif defined(__USLC__) /* all SCO platforms + UDK or OUDK */
#  define Q_OS_UNIXWARE
#elif defined(__svr4__) && defined(i386) /* Open UNIX 8 + GCC */
#  define Q_OS_UNIXWARE
#elif defined(__MAKEDEPEND__)
#else
#  error "Qt has not been ported to this OS - talk to qt-bugs@trolltech.com"
#endif

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
#  define Q_OS_WIN
#endif

#if defined(Q_OS_DARWIN)
#  define Q_OS_MAC /* Q_OS_MAC is mostly for compatiblity, but also more clear */
#endif

#if defined(Q_OS_MSDOS) || defined(Q_OS_OS2) || defined(Q_OS_WIN)
#  undef Q_OS_UNIX
#elif !defined(Q_OS_UNIX)
#  define Q_OS_UNIX
#endif

#if defined(Q_OS_DARWIN) && !defined(QT_LARGEFILE_SUPPORT)
#  define QT_LARGEFILE_SUPPORT 64
#endif


/*
   The compiler, must be one of: (Q_CC_x)

     SYM      - Digital Mars C/C++ (used to be Symantec C++)
     MPW      - MPW C++
     MWERKS   - Metrowerks CodeWarrior
     MSVC     - Microsoft Visual C/C++, Intel C++ for Windows
     BOR      - Borland/Turbo C++
     WAT      - Watcom C++
     GNU      - GNU C++
     COMEAU   - Comeau C++
     EDG      - Edison Design Group C++
     OC       - CenterLine C++
     SUN      - Forte Developer, or Sun ONE Studio C++
     MIPS     - MIPSpro C++
     DEC      - DEC C++
     HP       - HPUX C++
     HPACC    - HPUX ANSI C++
     USLC     - SCO OUDK, UDK, and UnixWare 2.X C++
     CDS      - Reliant C++
     KAI      - KAI C++
     INTEL    - Intel C++ for Linux, Intel C++ for Windows
     HIGHC    - MetaWare High C/C++
     PGI      - Portland Group C++
     GHS      - Green Hills Optimizing C++ Compilers

   Should be sorted most to least authoritative.
*/

/* Symantec C++ is now Digital Mars */
#if defined(__DMC__) || defined(__SC__)
#  define Q_CC_SYM
/* "explicit" semantics implemented in 8.1e but keyword recognized since 7.5 */
#  if defined(__SC__) && __SC__ < 0x750
#    define Q_NO_EXPLICIT_KEYWORD
#  endif
#  define Q_NO_USING_KEYWORD

#elif defined(applec)
#  define Q_CC_MPW
#  define Q_NO_BOOL_TYPE
#  define Q_NO_EXPLICIT_KEYWORD
#  define Q_NO_USING_KEYWORD

#elif defined(__MWERKS__)
#  define Q_CC_MWERKS
/* "explicit" recognized since 4.0d1 */

#elif defined(_MSC_VER)
#  define Q_CC_MSVC
/* proper support of bool for _MSC_VER >= 1100 */
#  define Q_CANNOT_DELETE_CONSTANT
#  define Q_OUTOFLINE_TEMPLATE inline
#  define QT_NO_PARTIAL_TEMPLATE_SPECIALIZATION
/* Visual C++.Net issues for _MSC_VER >= 1300 */
#  if _MSC_VER >= 1300
#    define Q_CC_MSVC_NET
#    if _MSC_VER < 1310 || defined(Q_OS_WIN64)
#      define Q_TYPENAME
#    else
#      undef QT_NO_PARTIAL_TEMPLATE_SPECIALIZATION
#    endif
#  else
#    define Q_NO_USING_KEYWORD
#  endif
/* Intel C++ disguising as Visual C++: the `using' keyword avoids warnings */
#  if defined(__INTEL_COMPILER)
#    define Q_CC_INTEL
#  endif

#elif defined(__BORLANDC__) || defined(__TURBOC__)
#  define Q_CC_BOR
#  define Q_INLINE_TEMPLATE
#  if __BORLANDC__ < 0x502
#    define Q_NO_BOOL_TYPE
#    define Q_NO_EXPLICIT_KEYWORD
#  endif
#  define Q_NO_USING_KEYWORD /* ### check "using" status */

#elif defined(__WATCOMC__)
#  define Q_CC_WAT
#  if defined(Q_OS_QNX4)
/* compiler flags */
#    define Q_TYPENAME
#    define Q_NO_BOOL_TYPE
#    define Q_CANNOT_DELETE_CONSTANT
#    define mutable
/* ??? */
#    define Q_BROKEN_TEMPLATE_SPECIALIZATION
/* no template classes in QVariant */
#    define QT_NO_TEMPLATE_VARIANT
/* Wcc does not fill in functions needed by valuelists, maps, and
   valuestacks implicitly */
#    define Q_FULL_TEMPLATE_INSTANTIATION
/* can we just compare the structures? */
#    define Q_FULL_TEMPLATE_INSTANTIATION_MEMCMP
/* these are not useful to our customers */
#    define QT_QWS_NO_SHM
#    define QT_NO_QWS_MULTIPROCESS
#    define QT_NO_SQL
#    define QT_NO_QWS_CURSOR
#  endif

#elif defined(__GNUC__)
#  define Q_CC_GNU
#  define Q_C_CALLBACKS
#  if defined(__INTEL_COMPILER)
/* Intel C++ also masquerades as GCC 3.2.0 */
#    define Q_CC_INTEL
#  else
#    ifdef __APPLE__
#      define Q_NO_COMPAT_CONSTRUCTORS
#    endif
#    if __GNUC__ == 2 && __GNUC_MINOR__ <= 7
#      define Q_FULL_TEMPLATE_INSTANTIATION
#    endif
/* GCC 2.95 knows "using" but does not support it correctly */
#    if __GNUC__ == 2 && __GNUC_MINOR__ <= 95
#      define Q_NO_USING_KEYWORD
#    endif
/* GCC 3.1 and GCC 3.2 wrongly define _SB_CTYPE_MACROS on HP-UX */
#    if defined(Q_OS_HPUX) && __GNUC__ == 3 && __GNUC_MINOR__ >= 1
#      define Q_WRONG_SB_CTYPE_MACROS
#    endif
/* Apple's GCC 3.1 chocks on our streaming qDebug() :( */
#    if defined(Q_OS_DARWIN) && __GNUC__ == 3 && (__GNUC_MINOR__ >= 1 && __GNUC_MINOR__ < 3)
#      define Q_NO_STREAMING_DEBUG
#    endif
#    if (defined(__arm__) || defined(__ARMEL__)) && !defined(QT_MOC_CPP)
#      define Q_PACKED __attribute__ ((__packed__))
#    endif
#  endif // __INTEL_COMPILER

/* IBM compiler versions are a bit messy. There are actually two products:
   the C product, and the C++ product. The C++ compiler is always packaged
   with the latest version of the C compiler. Version numbers do not always
   match. This little table (I'm not sure it's accurate) should be helpful:

   C++ product                C product

   C Set 3.1                  C Compiler 3.0
   ...                        ...
   C++ Compiler 3.6.6         C Compiler 4.3
   ...                        ...
   Visual Age C++ 4.0         ...
   ...                        ...
   Visual Age C++ 5.0         C Compiler 5.0
   ...                        ...
   Visual Age C++ 6.0         C Compiler 6.0

   Now:
   __xlC__    is the version of the C compiler in hexadecimal notation
              is only an approximation of the C++ compiler version
   __IBMCPP__ is the version of the C++ compiler in decimal notation
              but it is not defined on older compilers like C Set 3.1 */
#elif defined(__xlC__)
#  define Q_CC_XLC
#  define Q_FULL_TEMPLATE_INSTANTIATION
#  if __xlC__ < 0x400
#    define Q_NO_BOOL_TYPE
#    define Q_NO_EXPLICIT_KEYWORD
#    define Q_NO_USING_KEYWORD
#    define Q_TYPENAME
#    define Q_OUTOFLINE_TEMPLATE inline
#    define Q_BROKEN_TEMPLATE_SPECIALIZATION
#    define Q_CANNOT_DELETE_CONSTANT
#  endif

/* Older versions of DEC C++ do not define __EDG__ or __EDG - observed
   on DEC C++ V5.5-004. New versions do define  __EDG__ - observed on
   Compaq C++ V6.3-002.
   This compiler is different enough from other EDG compilers to handle
   it separately anyway. */
#elif defined(__DECCXX)
#  define Q_CC_DEC
/* Compaq C++ V6 compilers are EDG-based but I'm not sure about older
   DEC C++ V5 compilers. */
#  if defined(__EDG__)
#    define Q_CC_EDG
#  endif
/* Compaq have disabled EDG's _BOOL macro and use _BOOL_EXISTS instead
   - observed on Compaq C++ V6.3-002.
   In any case versions prior to Compaq C++ V6.0-005 do not have bool. */
#  if !defined(_BOOL_EXISTS)
#    define Q_NO_BOOL_TYPE
#  endif
/* Spurious (?) error messages observed on Compaq C++ V6.5-014. */
#  define Q_NO_USING_KEYWORD
/* Apply to all versions prior to Compaq C++ V6.0-000 - observed on
   DEC C++ V5.5-004. */
#  if __DECCXX_VER < 60060000
#    define Q_TYPENAME
#    define Q_BROKEN_TEMPLATE_SPECIALIZATION
#    define Q_CANNOT_DELETE_CONSTANT
#  endif
/* avoid undefined symbol problems with out-of-line template members */
#  define Q_OUTOFLINE_TEMPLATE inline

/* Compilers with EDG front end are similar. To detect them we test:
   __EDG documented by SGI, observed on MIPSpro 7.3.1.1 and KAI C++ 4.0b
   __EDG__ documented in EDG online docs, observed on Compaq C++ V6.3-002 */
#elif defined(__EDG) || defined(__EDG__)
#  define Q_CC_EDG
/* From the EDG documentation (does not seem to apply to Compaq C++):
   _BOOL
        Defined in C++ mode when bool is a keyword. The name of this
        predefined macro is specified by a configuration flag. _BOOL
        is the default.
   __BOOL_DEFINED
        Defined in Microsoft C++ mode when bool is a keyword. */
#  if !defined(_BOOL) && !defined(__BOOL_DEFINED)
#    define Q_NO_BOOL_TYPE
#  endif

/* The Comeau compiler is based on EDG and does define __EDG__ */
#  if defined(__COMO__)
#    define Q_CC_COMEAU
#    define Q_C_CALLBACKS

/* The `using' keyword was introduced to avoid KAI C++ warnings
   but it's now causing KAI C++ errors instead. The standard is
   unclear about the use of this keyword, and in practice every
   compiler is using its own set of rules. Forget it. */
#  elif defined(__KCC)
#    define Q_CC_KAI
#    define Q_NO_USING_KEYWORD

/* Using the `using' keyword avoids Intel C++ for Linux warnings */
#  elif defined(__INTEL_COMPILER)
#    define Q_CC_INTEL

/* The Portland Group compiler is based on EDG and does define __EDG__ */
#  elif defined(__PGI)
#    define Q_CC_PGI

/* Never tested! */
#  elif defined(__ghs)
#    define Q_CC_GHS

/* The UnixWare 7 UDK compiler is based on EDG and does define __EDG__ */
#  elif defined(__USLC__) && defined(__SCO_VERSION__)
#    define Q_CC_USLC
/* The latest UDK 7.1.1b does not need this, but previous versions do */
#    if !defined(__SCO_VERSION__) || (__SCO_VERSION__ < 302200010)
#      define Q_OUTOFLINE_TEMPLATE inline
#    endif
#    define Q_NO_USING_KEYWORD /* ### check "using" status */

/* Never tested! */
#  elif defined(CENTERLINE_CLPP) || defined(OBJECTCENTER)
#    define Q_CC_OC
#    define Q_NO_USING_KEYWORD

/* CDS++ defines __EDG__ although this is not documented in the Reliant
   documentation. It also follows conventions like _BOOL and this documented */
#  elif defined(sinix)
#    define Q_CC_CDS
#    define Q_NO_USING_KEYWORD

/* The MIPSpro compiler in o32 mode is based on EDG but disables features
   such as template specialization nevertheless */
#  elif defined(__sgi)
#    define Q_CC_MIPS
#    if defined(_MIPS_SIM) && (_MIPS_SIM == _ABIO32) /* o32 ABI */
#      define Q_TYPENAME
#      define Q_BROKEN_TEMPLATE_SPECIALIZATION
#      define Q_NO_EXPLICIT_KEYWORD
#      define Q_OUTOFLINE_TEMPLATE inline
#    elif defined(_COMPILER_VERSION) && (_COMPILER_VERSION < 730) /* 7.2 */
#      define Q_TYPENAME
#      define Q_BROKEN_TEMPLATE_SPECIALIZATION
#    endif
#    define Q_NO_USING_KEYWORD /* ### check "using" status */
#    if defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 740)
#      pragma set woff 3624,3625, 3649 /* turn off some harmless warnings */
#    endif
#  endif

/* The older UnixWare 2.X compiler? */
#elif defined(__USLC__)
#  define Q_CC_USLC
#  define Q_TYPENAME
#  define Q_NO_BOOL_TYPE
#  define Q_NO_EXPLICIT_KEYWORD
#  define Q_NO_USING_KEYWORD
#  define Q_OUTOFLINE_TEMPLATE inline

/* Never tested! */
#elif defined(__HIGHC__)
#  define Q_CC_HIGHC

#elif defined(__SUNPRO_CC) || defined(__SUNPRO_C)
#  define Q_CC_SUN
/* 5.0 compiler or better
    'bool' is enabled by default but can be disabled using -features=nobool
    in which case _BOOL is not defined
        this is the default in 4.2 compatibility mode triggered by -compat=4 */
#  if __SUNPRO_CC >= 0x500
#    if !defined(_BOOL)
#      define Q_NO_BOOL_TYPE
#    endif
#    if defined(__SUNPRO_CC_COMPAT) && (__SUNPRO_CC_COMPAT <= 4)
#      define Q_NO_USING_KEYWORD
#    endif
#    define Q_C_CALLBACKS
/* 4.2 compiler or older */
#  else
#    define Q_NO_BOOL_TYPE
#    define Q_NO_EXPLICIT_KEYWORD
#    define Q_NO_USING_KEYWORD
#  endif

/* CDS++ does not seem to define __EDG__ or __EDG according to Reliant
   documentation but nevertheless uses EDG conventions like _BOOL */
#elif defined(sinix)
#  define Q_CC_EDG
#  define Q_CC_CDS
#  if !defined(_BOOL)
#    define Q_NO_BOOL_TYPE
#  endif
#  define Q_BROKEN_TEMPLATE_SPECIALIZATION

#elif defined(Q_OS_HPUX)
/* __HP_aCC was not defined in first aCC releases */
#  if defined(__HP_aCC) || __cplusplus >= 199707L
#    define Q_CC_HPACC
#  else
#    define Q_CC_HP
#    define Q_NO_BOOL_TYPE
#    define Q_FULL_TEMPLATE_INSTANTIATION
#    define Q_BROKEN_TEMPLATE_SPECIALIZATION
#    define Q_NO_EXPLICIT_KEYWORD
#  endif
#  define Q_NO_USING_KEYWORD /* ### check "using" status */

#else
#  error "Qt has not been tested with this compiler - talk to qt-bugs@trolltech.com"
#endif

#ifndef Q_PACKED
#  define Q_PACKED
#endif


/*
   The window system, must be one of: (Q_WS_x)

     MACX     - Mac OS X
     MAC9     - Mac OS 9
     QWS      - Qt/Embedded
     WIN32    - Windows
     X11      - X Window System
     PM       - unsupported
     WIN16    - unsupported
*/

#if defined(Q_OS_MSDOS)
#  define Q_WS_WIN16
#  error "Qt requires Win32 and does not work with Windows 3.x"
#elif defined(_WIN32_X11_)
#  define Q_WS_X11
#elif defined(Q_OS_WIN32)
#  define Q_WS_WIN32
#  if defined(Q_OS_WIN64)
#    define Q_WS_WIN64
#  endif
#elif defined(Q_OS_OS2)
#  define Q_WS_PM
#  error "Qt does not work with OS/2 Presentation Manager or Workplace Shell"
#elif defined(Q_OS_UNIX)
#  if defined(Q_OS_DARWIN) && !defined(__USE_WS_X11__)
#    define Q_WS_MAC
#  elif !defined(Q_WS_QWS)
#    define Q_WS_X11
#  endif
#endif

#if defined(Q_WS_WIN16) || defined(Q_WS_WIN32)
#  define Q_WS_WIN
#endif

#if defined(__cplusplus)

//
// Useful type definitions for Qt
//

#if defined(Q_NO_BOOL_TYPE)
#error "Compiler doesn't support the 'bool' type!"
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef char *pchar;
typedef uchar *puchar;
typedef const char *pcchar;

//
// Constant bool values
//

#ifndef TRUE
#define TRUE true
#define FALSE false
#endif

//
// Proper for-scoping in VC++6 and MIPSpro CC
//

#if (defined(Q_CC_MSVC) && !defined(Q_CC_MSVC_NET)) || defined(Q_CC_MIPS)
#  define for if(0){}else for
#endif


//
// Use the "explicit" keyword on platforms that support it.
//

#if !defined(Q_NO_EXPLICIT_KEYWORD)
#  define Q_EXPLICIT explicit
#else
#  define Q_EXPLICIT
#endif


//
// Workaround for static const members on MSVC++.
//

#if defined(Q_CC_MSVC)
#  define QT_STATIC_CONST static
#  define QT_STATIC_CONST_IMPL
#else
#  define QT_STATIC_CONST static const
#  define QT_STATIC_CONST_IMPL const
#endif

//
// Warnings and errors when using deprecated methods
//
#if defined(QT_COMPAT_WARNINGS)
#  ifdef QT_COMPAT
#    undef QT_COMPAT
#  endif
#  if defined(Q_MOC_RUN)
#    define QT_COMPAT QT_COMPAT
#  elif defined(Q_CC_GNU) && !defined(Q_CC_INTEL) && (__GNUC__ - 0 > 3 || (__GNUC__ - 0 == 3 && __GNUC_MINOR__ - 0 >= 2))
#    define QT_COMPAT __attribute__ ((__deprecated__))
#  elif defined(Q_CC_MSVC) && (_MSC_VER >= 1300)
#    define QT_COMPAT __declspec(deprecated)
#  else
#    define QT_COMPAT
#  endif
#elif defined(QT_COMPAT) // make sure QT_COMPAT is void
#  undef QT_COMPAT
#  define QT_COMPAT
#endif

#ifndef QT_COMPAT_CONSTRUCTOR
#  if defined(Q_MOC_RUN)
#    define QT_COMPAT_CONSTRUCTOR QT_COMPAT_CONSTRUCTOR
#  elif defined(Q_NO_COMPAT_CONSTRUCTORS)
#    define QT_COMPAT_CONSTRUCTOR explicit
#  elif defined(QT_COMPAT)
#    define QT_COMPAT_CONSTRUCTOR explicit QT_COMPAT
#  endif
#endif

#ifdef QT_MOC_COMPAT //for marking signals/slots
# undef QT_MOC_COMPAT
#endif
# if defined(Q_MOC_RUN)
#   define QT_MOC_COMPAT QT_MOC_COMPAT
#else
#   define QT_MOC_COMPAT
#endif

#if defined(QT_DEPRECATED_WARNINGS)
#  if defined(Q_MOC_RUN)
#    define QT_DEPRECATED QT_DEPRECATED
#  elif defined(Q_CC_GNU) && (__GNUC__ - 0 > 3 || (__GNUC__ - 0 == 3 && __GNUC_MINOR__ - 0 >= 2))
#    define QT_DEPRECATED __attribute__ ((__deprecated__))
#  elif defined(Q_CC_MSVC) && (_MSC_VER >= 1300)
#    define QT_DEPRECATED __declspec(deprecated)
#  else
#    define QT_DEPRECATED
#  endif
#else
#  define QT_DEPRECATED
#endif

#ifdef __i386__
#  if defined(Q_CC_GNU)
#    define QT_FASTCALL __attribute__((regparm(3)))
#  elif defined(Q_CC_MSVC)
#    define QT_FASTCALL __fastcall
#  else
#     define QT_FASTCALL
#  endif
#else
#  define QT_FASTCALL
#endif


//
// Size-dependent types (architechture-dependent byte order)
//

typedef signed char Q_INT8;         // 8 bit signed
typedef unsigned char Q_UINT8;      // 8 bit unsigned
typedef short Q_INT16;              // 16 bit signed
typedef unsigned short Q_UINT16;    // 16 bit unsigned
typedef int Q_INT32;                // 32 bit signed
typedef unsigned int Q_UINT32;      // 32 bit unsigned
#if defined(Q_OS_WIN64)
typedef __int64 Q_LONG;             // word up to 64 bit signed
typedef unsigned __int64 Q_ULONG;   // word up to 64 bit unsigned
#else
typedef long Q_LONG;                // word up to 64 bit signed
typedef unsigned long Q_ULONG;      // word up to 64 bit unsigned
#endif
#if defined(Q_OS_WIN) && !defined(Q_CC_GNU)
#  define Q_INT64_C(c) c ## i64    // signed 64 bit constant
#  define Q_UINT64_C(c) c ## ui64  // unsigned 64 bit constant
typedef __int64 Q_INT64;            // 64 bit signed
typedef unsigned __int64 Q_UINT64;  // 64 bit unsigned
#else
#  define Q_INT64_C(c) c ## LL     // signed 64 bit constant
#  define Q_UINT64_C(c) c ## ULL   // unsigned 64 bit constant
typedef long long Q_INT64;          // 64 bit signed
typedef unsigned long long Q_UINT64;// 64 bit unsigned
#endif

typedef Q_INT64 Q_LONGLONG;            // signed long long
typedef Q_UINT64 Q_ULONGLONG;          // unsigned long long
#ifdef QT_COMPAT
typedef Q_INT64 Q_LLONG;
typedef Q_UINT64 Q_ULLONG;
#endif

typedef int QNoImplicitBoolCast;

//
// Utility macros and inline functions
//

#define QABS(a) ((a) >= 0  ? (a) : -(a))

inline int qRound(double d)
{ return d >= 0.0 ? int(d + 0.5) : int(d - int(d-1) + 0.5) + int(d-1); }

template <typename T>
inline T qMin(T a, T b) { return (a < b) ? a : b; }
template <typename T>
inline T qMax(T a, T b) { return (b < a) ? a : b; }

#ifdef QT_COMPAT
#  define QMAX(a, b) qMax((a), (b))
#  define QMIN(a, b) qMin((a), (b))
#endif

inline Q_LONGLONG qMin(Q_LONGLONG a, long b) { return (a < b) ? a : b; }
inline Q_LONGLONG qMin(long a, Q_LONGLONG b) { return (a < b) ? a : b; }
inline Q_LONGLONG qMin(Q_LONGLONG a, int b) { return (a < b) ? a : b; }
inline Q_LONGLONG qMin(int a, Q_LONGLONG b) { return (a < b) ? a : b; }
inline Q_LONGLONG qMin(Q_LONGLONG a, short b) { return (a < b) ? a : b; }
inline Q_LONGLONG qMin(short a, Q_LONGLONG b) { return (a < b) ? a : b; }
inline Q_LONGLONG qMin(Q_LONGLONG a, char b) { return (a < b) ? a : b; }
inline Q_LONGLONG qMin(char a, Q_LONGLONG b) { return (a < b) ? a : b; }
inline Q_ULONGLONG qMin(Q_ULONGLONG a, ulong b) { return (a < b) ? a : b; }
inline Q_ULONGLONG qMin(ulong a, Q_ULONGLONG b) { return (a < b) ? a : b; }
inline Q_ULONGLONG qMin(Q_ULONGLONG a, uint b) { return (a < b) ? a : b; }
inline Q_ULONGLONG qMin(uint a, Q_ULONGLONG b) { return (a < b) ? a : b; }
inline Q_ULONGLONG qMin(Q_ULONGLONG a, ushort b) { return (a < b) ? a : b; }
inline Q_ULONGLONG qMin(ushort a, Q_ULONGLONG b) { return (a < b) ? a : b; }
inline Q_ULONGLONG qMin(Q_ULONGLONG a, uchar b) { return (a < b) ? a : b; }
inline Q_ULONGLONG qMin(uchar a, Q_ULONGLONG b) { return (a < b) ? a : b; }
inline double qMin(double a, int b) { return (a < b) ? a : b; }
inline double qMin(int a, double b) { return (a < b) ? a : b; }
inline double qMin(double a, long b) { return (a < b) ? a : b; }
inline double qMin(long a, double b) { return (a < b) ? a : b; }
inline double qMin(double a, short b) { return (a < b) ? a : b; }
inline double qMin(short a, double b) { return (a < b) ? a : b; }
inline double qMin(double a, char b) { return (a < b) ? a : b; }
inline double qMin(char a, double b) { return (a < b) ? a : b; }
inline double qMin(double a, float b) { return (a < b) ? a : b; }
inline double qMin(float a, double b) { return (a < b) ? a : b; }
inline float qMin(float a, int b) { return (a < b) ? a : b; }
inline float qMin(int a, float b) { return (a < b) ? a : b; }
inline float qMin(float a, short b) { return (a < b) ? a : b; }
inline float qMin(short a, float b) { return (a < b) ? a : b; }
inline float qMin(float a, char b) { return (a < b) ? a : b; }
inline float qMin(char a, float b) { return (a < b) ? a : b; }
inline float qMin(float a, long b) { return (a < b) ? a : b; }
inline float qMin(long a, float b) { return (a < b) ? a : b; }
inline long qMin(long a, int b) { return (a < b) ? a : b; }
inline long qMin(int a, long b) { return (a < b) ? a : b; }
inline long qMin(long a, short b) { return (a < b) ? a : b; }
inline long qMin(short a, long b) { return (a < b) ? a : b; }
inline long qMin(long a, char b) { return (a < b) ? a : b; }
inline long qMin(char a, long b) { return (a < b) ? a : b; }
inline ulong qMin(ulong a, uint b) { return (a < ulong(b)) ? ulong(a) : b; }
inline ulong qMin(uint a, ulong b) { return (ulong(a) < b) ? ulong(a) : b; }
inline ulong qMin(ulong a, ushort b) { return (a < ulong(b)) ? a : ulong(b); }
inline ulong qMin(ushort a, ulong b) { return (ulong(a) < b) ? ulong(a) : b; }
inline ulong qMin(ulong a, uchar b) { return (a < ulong(b)) ? a : ulong(b); }
inline ulong qMin(uchar a, ulong b) { return (ulong(a) < b) ? ulong(a) : b; }
inline int qMin(int a, short b) { return (a < b) ? a : b; }
inline int qMin(short a, int b) { return (a < b) ? a : b; }
inline int qMin(int a, char b) { return (a < b) ? a : b; }
inline int qMin(char a, int b) { return (a < b) ? a : b; }
inline uint qMin(uint a, ushort b) { return (a < uint(b)) ? a : uint(b); }
inline uint qMin(ushort a, uint b) { return (uint(a) < b) ? uint(a) : b; }
inline uint qMin(uint a, uchar b) { return (a < uint(b)) ? a : uint(b); }
inline uint qMin(uchar a, uint b) { return (uint(a) < b) ? uint(a) : b; }
inline short qMin(short a, char b) { return (a < b) ? a : b; }
inline short qMin(char a, short b) { return (a < b) ? a : b; }
inline ushort qMin(ushort a, uchar b) { return (a < b) ? a : b; }
inline ushort qMin(uchar a, ushort b) { return (a < b) ? a : b; }

inline Q_LONGLONG qMax(Q_LONGLONG a, long b) { return (b < a) ? a : b; }
inline Q_LONGLONG qMax(long a, Q_LONGLONG b) { return (b < a) ? a : b; }
inline Q_LONGLONG qMax(Q_LONGLONG a, int b) { return (b < a) ? a : b; }
inline Q_LONGLONG qMax(int a, Q_LONGLONG b) { return (b < a) ? a : b; }
inline Q_LONGLONG qMax(Q_LONGLONG a, short b) { return (b < a) ? a : b; }
inline Q_LONGLONG qMax(short a, Q_LONGLONG b) { return (b < a) ? a : b; }
inline Q_LONGLONG qMax(Q_LONGLONG a, char b) { return (b < a) ? a : b; }
inline Q_LONGLONG qMax(char a, Q_LONGLONG b) { return (b < a) ? a : b; }
inline Q_ULONGLONG qMax(Q_ULONGLONG a, ulong b) { return (b < a) ? a : b; }
inline Q_ULONGLONG qMax(ulong a, Q_ULONGLONG b) { return (b < a) ? a : b; }
inline Q_ULONGLONG qMax(Q_ULONGLONG a, uint b) { return (b < a) ? a : b; }
inline Q_ULONGLONG qMax(uint a, Q_ULONGLONG b) { return (b < a) ? a : b; }
inline Q_ULONGLONG qMax(Q_ULONGLONG a, ushort b) { return (b < a) ? a : b; }
inline Q_ULONGLONG qMax(ushort a, Q_ULONGLONG b) { return (b < a) ? a : b; }
inline Q_ULONGLONG qMax(Q_ULONGLONG a, uchar b) { return (b < a) ? a : b; }
inline Q_ULONGLONG qMax(uchar a, Q_ULONGLONG b) { return (b < a) ? a : b; }
inline double qMax(double a, int b) { return (b < a) ? a : b; }
inline double qMax(int a, double b) { return (b < a) ? a : b; }
inline double qMax(double a, short b) { return (b < a) ? a : b; }
inline double qMax(short a, double b) { return (b < a) ? a : b; }
inline double qMax(double a, char b) { return (b < a) ? a : b; }
inline double qMax(char a, double b) { return (b < a) ? a : b; }
inline double qMax(double a, long b) { return (b < a) ? a : b; }
inline double qMax(long a, double b) { return (b < a) ? a : b; }
inline double qMax(double a, float b) { return (b < a) ? a : b; }
inline double qMax(float a, double b) { return (b < a) ? a : b; }
inline float qMax(float a, int b) { return (b < a) ? a : b; }
inline float qMax(int a, float b) { return (b < a) ? a : b; }
inline float qMax(float a, short b) { return (b < a) ? a : b; }
inline float qMax(short a, float b) { return (b < a) ? a : b; }
inline float qMax(float a, char b) { return (b < a) ? a : b; }
inline float qMax(char a, float b) { return (b < a) ? a : b; }
inline float qMax(float a, long b) { return (b < a) ? a : b; }
inline float qMax(long a, float b) { return (b < a) ? a : b; }
inline long qMax(long a, int b) { return (b < a) ? a : b; }
inline long qMax(int a, long b) { return (b < a) ? a : b; }
inline long qMax(long a, short b) { return (b < a) ? a : b; }
inline long qMax(short a, long b) { return (b < a) ? a : b; }
inline long qMax(long a, char b) { return (b < a) ? a : b; }
inline long qMax(char a, long b) { return (b < a) ? a : b; }
inline ulong qMax(ulong a, uint b) { return (b < a) ? a : b; }
inline ulong qMax(uint a, ulong b) { return (b < a) ? a : b; }
inline ulong qMax(ulong a, ushort b) { return (ulong(b) < a) ? a : ulong(b); }
inline ulong qMax(ushort a, ulong b) { return (b < ulong(a)) ? ulong(a) : b; }
inline ulong qMax(ulong a, uchar b) { return (ulong(b) < a) ? a : ulong(b); }
inline ulong qMax(uchar a, ulong b) { return (b < ulong(a)) ? ulong(a) : b; }
inline int qMax(int a, short b) { return (b < a) ? a : b; }
inline int qMax(short a, int b) { return (b < a) ? a : b; }
inline int qMax(int a, char b) { return (b < a) ? a : b; }
inline int qMax(char a, int b) { return (b < a) ? a : b; }
inline uint qMax(uint a, ushort b) { return (uint(b) < a) ? a : uint(b); }
inline uint qMax(ushort a, uint b) { return (b < uint(a)) ? uint(a) : b; }
inline uint qMax(uint a, uchar b) { return (uint(b) < a) ? a : uint(b); }
inline uint qMax(uchar a, uint b) { return (b < uint(a)) ? uint(a) : b; }
inline short qMax(short a, char b) { return (b < a) ? a : b; }
inline short qMax(char a, short b) { return (b < a) ? a : b; }
inline ushort qMax(ushort a, uchar b) { return (b < a) ? a : b; }
inline ushort qMax(uchar a, ushort b) { return (b < a) ? a : b; }


//
// Data stream functions are provided by many classes (defined in qdatastream.h)
//

class QDataStream;

#ifdef Q_OS_DARWIN
#  ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#    undef MAC_OS_X_VERSION_MIN_REQUIRED
#  endif
#  define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_2
#  include <AvailabilityMacros.h>
#  if !defined(MAC_OS_X_VERSION_10_3)
#     define MAC_OS_X_VERSION_10_3 MAC_OS_X_VERSION_10_2 + 1
#  endif
#  if !defined(MAC_OS_X_VERSION_10_4)
#       define MAC_OS_X_VERSION_10_4 MAC_OS_X_VERSION_10_3 + 1
#  endif
#  if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
#    warning "Support for this version of Mac OS X is still preliminary."
#  endif
#  if (MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_4)
#    error "This version is unsupported."
#  endif
#endif

#ifndef QT_BUILD_KEY
#define QT_BUILD_KEY "unspecified"
#endif

#if defined(Q_WS_MAC)
#  ifndef QMAC_QMENUBAR_NO_EVENT
#    define QMAC_QMENUBAR_NO_EVENT
#  endif
#endif

#if !defined(Q_WS_QWS) && !defined(QT_NO_COP)
#  define QT_NO_COP
#endif

#  include "qfeatures.h"

#ifndef Q_DECL_EXPORT
#  ifdef Q_OS_WIN
#    define Q_DECL_EXPORT __declspec(dllexport)
#  else
#    define Q_DECL_EXPORT
#  endif
#endif
#ifndef Q_DECL_IMPORT
#  ifdef Q_OS_WIN
#    define Q_DECL_IMPORT __declspec(dllimport)
#  else
#    define Q_DECL_IMPORT
#  endif
#endif

//
// Create Qt DLL if QT_DLL is defined (Windows only)
// or QT_SHARED is defined (Kylix only)
//
#if defined(Q_OS_WIN)
#  if defined(QT_NODLL)
#    undef QT_MAKEDLL
#    undef QT_DLL
#  elif defined(QT_MAKEDLL)        /* create a Qt DLL library */
#    if defined(QT_DLL)
#      undef QT_DLL
#    endif
#    if defined(QT_BUILD_CORE_LIB)
#      define Q_CORE_EXPORT Q_DECL_EXPORT
#    else
#      define Q_CORE_EXPORT Q_DECL_IMPORT
#    endif
#    if defined(QT_BUILD_GUI_LIB)
#      define Q_GUI_EXPORT Q_DECL_EXPORT
#    else
#      define Q_GUI_EXPORT Q_DECL_IMPORT
#    endif
#    if defined(QT_BUILD_SQL_LIB)
#      define Q_SQL_EXPORT Q_DECL_EXPORT
#    else
#      define Q_SQL_EXPORT Q_DECL_IMPORT
#    endif
#    if defined(QT_BUILD_NETWORK_LIB)
#      define Q_NETWORK_EXPORT Q_DECL_EXPORT
#    else
#      define Q_NETWORK_EXPORT Q_DECL_IMPORT
#    endif
#    if defined(QT_BUILD_OPENGL_LIB)
#      define Q_OPENGL_EXPORT Q_DECL_EXPORT
#    else
#      define Q_OPENGL_EXPORT Q_DECL_IMPORT
#    endif
#    if defined(QT_BUILD_XML_LIB)
#      define Q_XML_EXPORT Q_DECL_EXPORT
#    else
#      define Q_XML_EXPORT Q_DECL_IMPORT
#    endif
#    if defined(QT_BUILD_CANVAS_LIB)
#      define Q_CANVAS_EXPORT Q_DECL_EXPORT
#    else
#      define Q_CANVAS_EXPORT Q_DECL_IMPORT
#    endif
#    if defined(QT_BUILD_COMPAT_LIB)
#      define Q_COMPAT_EXPORT Q_DECL_EXPORT
#    else
#      define Q_COMPAT_EXPORT Q_DECL_IMPORT
#    endif
#    define Q_TEMPLATEDLL
#    define Q_NO_UNRESOLVED_EXTERNALS
#  elif defined(QT_DLL) /* use a Qt DLL library */
#    define Q_CORE_EXPORT Q_DECL_IMPORT
#    define Q_GUI_EXPORT Q_DECL_IMPORT
#    define Q_SQL_EXPORT Q_DECL_IMPORT
#    define Q_NETWORK_EXPORT Q_DECL_IMPORT
#    define Q_CANVAS_EXPORT Q_DECL_IMPORT
#    define Q_OPENGL_EXPORT Q_DECL_IMPORT
#    define Q_XML_EXPORT Q_DECL_IMPORT
#    define Q_COMPAT_EXPORT Q_DECL_IMPORT
#    define Q_TEMPLATEDLL
#    define Q_NO_UNRESOLVED_EXTERNALS
#  endif
#elif defined(Q_OS_LINUX) && defined(Q_CC_BOR)
#  define Q_TEMPLATEDLL
#  define Q_NO_UNRESOLVED_EXTERNALS
#else
#  undef QT_MAKEDLL /* ignore these for other platforms */
#  undef QT_DLL
#endif

//defaults
#ifndef Q_CORE_EXPORT
#  define Q_CORE_EXPORT
#endif
#ifndef Q_GUI_EXPORT
#  define Q_GUI_EXPORT
#endif
#ifndef Q_SQL_EXPORT
#  define Q_SQL_EXPORT
#endif
#ifndef Q_NETWORK_EXPORT
#  define Q_NETWORK_EXPORT
#endif
#ifndef Q_CANVAS_EXPORT
#  define Q_CANVAS_EXPORT
#endif
#ifndef Q_OPENGL_EXPORT
#  define Q_OPENGL_EXPORT
#endif
#ifndef Q_XML_EXPORT
#  define Q_XML_EXPORT
#endif
#ifndef Q_COMPAT_EXPORT
#  define Q_COMPAT_EXPORT
#endif

//
// System information
//

class Q_CORE_EXPORT QSysInfo {
public:
    enum {
        WordSize = (sizeof(Q_ULONG)<<3)
    };

    enum {
        BigEndian,
        LittleEndian

#ifdef Q_BYTE_ORDER
#  if Q_BYTE_ORDER == Q_BIG_ENDIAN
        , ByteOrder = BigEndian
#  elif Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        , ByteOrder = LittleEndian
#  else
#    error "undefined byte order"
#  endif
#endif
    };
#if !defined(Q_BYTE_ORDER)
#  if defined(QT_BUILD_QMAKE)
    // needed to bootstrap qmake
    static const int ByteOrder;
#  else
#    error "Qt not configured correctly, please run configure."
#  endif
#endif
#ifdef Q_WS_WIN
    enum WinVersion {
        WV_32s      = 0x0001,
        WV_95       = 0x0002,
        WV_98       = 0x0003,
        WV_Me       = 0x0004,
        WV_DOS_based= 0x000f,

        WV_NT       = 0x0010,
        WV_2000     = 0x0020,
        WV_XP       = 0x0030,
        WV_2003     = 0x0040,
        WV_NT_based = 0x00f0,

        WV_CE       = 0x0100,
        WV_CENET    = 0x0200,
        WV_CE_based = 0x0f00
    };
    static const WinVersion WindowsVersion;
#endif
#ifdef Q_WS_MAC
    enum MacVersion {
        //Unknown
        MV_Unknown  = 0x0000,

        //Version numbers
        MV_9        = 0x0001,
        MV_10_DOT_0 = 0x0002,
        MV_10_DOT_1 = 0x0003,
        MV_10_DOT_2 = 0x0004,
        MV_10_DOT_3 = 0x0005,
        MV_10_DOT_4 = 0x0006,

        //Code names
        MV_CHEETAH  = MV_10_DOT_0,
        MV_PUMA     = MV_10_DOT_1,
        MV_JAGUAR   = MV_10_DOT_2,
        MV_PANTHER  = MV_10_DOT_3,
        MV_TIGER    = MV_10_DOT_4
    };
    static const MacVersion MacintoshVersion;
#endif
};

Q_CORE_EXPORT const char *qVersion();
Q_CORE_EXPORT bool qSharedBuild();

#if defined(Q_OS_MAC)
inline int qMacVersion() { return QSysInfo::MacintoshVersion; }
#endif

#ifdef QT_COMPAT
inline QT_COMPAT bool qSysInfo(int *wordSize, bool *bigEndian)
{
    *wordSize = QSysInfo::WordSize;
    *bigEndian = (QSysInfo::ByteOrder == QSysInfo::BigEndian);
    return true;
}
#endif

#if defined(Q_WS_WIN)
#if defined(QT_COMPAT)
inline QT_COMPAT bool qt_winUnicode() { return !(QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based); }
inline QT_COMPAT int qWinVersion() { return QSysInfo::WindowsVersion; }
#endif

#ifdef Q_OS_TEMP
#define QT_WA(uni, ansi) uni
#define QT_WA_INLINE(uni, ansi) (uni)
#elif defined(UNICODE)
#define QT_WA(uni, ansi) if (!(QSysInfo::WindowsVersion& QSysInfo::WV_DOS_based)) { uni } else { ansi }
#define QT_WA_INLINE(uni, ansi) (!(QSysInfo::WindowsVersion& QSysInfo::WV_DOS_based) ? uni : ansi)
#else
#define QT_WA(uni, ansi) ansi
#define QT_WA_INLINE(uni, ansi) ansi
#endif
#endif // Q_WS_WIN

#ifndef Q_OUTOFLINE_TEMPLATE
#  define Q_OUTOFLINE_TEMPLATE
#endif
#ifndef Q_INLINE_TEMPLATE
#  define Q_INLINE_TEMPLATE inline
#endif

#ifndef Q_TYPENAME
#  define Q_TYPENAME typename
#endif

//
// Use to avoid "unused parameter" warnings
//
#define Q_UNUSED(x) (void)x;

//
// Debugging and error handling
//

#if !defined(QT_NO_DEBUG) && !defined(QT_DEBUG)
#  define QT_DEBUG
#endif

Q_CORE_EXPORT void qDebug(const char *, ...) // print debug message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

Q_CORE_EXPORT void qWarning(const char *, ...) // print warning message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

class QDebug;
class QNoDebug;

#ifdef QT_NO_DEBUG
#  define qDebug if(1); else qDebug
#  define qWarning if(1); else qWarning
#endif

Q_CORE_EXPORT void qSystemWarning(const char *, ...) // print system message
#if defined(Q_CC_GNU) && !defined(__INSURE__)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

Q_CORE_EXPORT void qFatal(const char *, ...) // print fatal message and exit
#if defined(Q_CC_GNU)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

inline void qt_noop() {}

Q_CORE_EXPORT void qt_assert(const char *assertion, const char *file, int line);

#if !defined(Q_ASSERT)
#  ifndef QT_NO_DEBUG
#    define Q_ASSERT(cond) do {if(!(cond))qt_assert(#cond,__FILE__,__LINE__);} while (0)
#  else
#    define Q_ASSERT(cond) do{}while(0)
#  endif
#endif

Q_CORE_EXPORT void qt_assert_x(const char *where, const char *what, const char *file, int line);

#if !defined(Q_ASSERT_X)
#  ifndef QT_NO_DEBUG
#    define Q_ASSERT_X(cond, where, what) do {if(!(cond))qt_assert_x(where, what,__FILE__,__LINE__);} while (0)
#  else
#    define Q_ASSERT_X(cond, where, what) do{}while(0)
#  endif
#endif

Q_CORE_EXPORT void qt_check_pointer(const char *, int);

#ifndef QT_NO_DEBUG
#  define Q_CHECK_PTR(p) do {if(!(p))qt_check_pointer(__FILE__,__LINE__);} while (0)
#else
#  define Q_CHECK_PTR(p)
#endif

enum QtMsgType { QtDebugMsg, QtSystemMsg, QtWarningMsg, QtFatalMsg };

typedef void (*QtMsgHandler)(QtMsgType, const char *);
Q_CORE_EXPORT QtMsgHandler qInstallMsgHandler(QtMsgHandler);

#ifdef QT_COMPAT
inline QT_COMPAT void qSuppressObsoleteWarnings(bool = true) {}
inline QT_COMPAT void qObsolete(const char *, const char * = 0, const char * = 0) {}
#endif

#if defined(QT_NO_THREAD)

template <typename T>
class QGlobalStatic
{
public:
    T *pointer;
    inline QGlobalStatic(T *p) : pointer(p) { }
    inline ~QGlobalStatic() { pointer = 0; }
};

#define Q_GLOBAL_STATIC(TYPE, NAME)                              \
    static TYPE *NAME()                                          \
    {                                                            \
        static TYPE this_##NAME;                                 \
        static QGlobalStatic<TYPE > global_##NAME(&this_##NAME); \
        return global_##NAME.pointer;                            \
    }

#define Q_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, ARGS)              \
    static TYPE *NAME()                                          \
    {                                                            \
        static TYPE this_##NAME ARGS;                            \
        static QGlobalStatic<TYPE > global_##NAME(&this_##NAME); \
        return global_##NAME.pointer;                            \
    }

#else

template <typename T>
class QGlobalStatic
{
public:
    T *pointer;
    bool destroyed;

    inline QGlobalStatic()
        : pointer(0), destroyed(false)
    { }

    inline ~QGlobalStatic()
    {
        delete pointer;
        pointer = 0;
        destroyed = true;
    }
};

#define Q_GLOBAL_STATIC(TYPE, NAME)                                     \
    static TYPE *NAME()                                                 \
    {                                                                   \
        static QGlobalStatic<TYPE > this_##NAME;                        \
        if (!this_##NAME.pointer && !this_##NAME.destroyed) {           \
            TYPE *x = new TYPE;                                         \
            if (!q_atomic_test_and_set_ptr(&this_##NAME.pointer, 0, x)) \
                delete x;                                               \
        }                                                               \
        return this_##NAME.pointer;                                   \
    }

#define Q_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, ARGS)                     \
    static TYPE *NAME()                                                 \
    {                                                                   \
        static QGlobalStatic<TYPE > this_##NAME;                        \
        if (!this_##NAME.pointer && !this_##NAME.destroyed) {           \
            TYPE *x = new TYPE ARGS;                                    \
            if (!q_atomic_test_and_set_ptr(&this_##NAME.pointer, 0, x)) \
                delete x;                                               \
        }                                                               \
        return this_##NAME.pointer;                                     \
    }

#endif


//
// Install paths from configure
//

Q_CORE_EXPORT const char *qInstallPath();
Q_CORE_EXPORT const char *qInstallPathDocs();
Q_CORE_EXPORT const char *qInstallPathHeaders();
Q_CORE_EXPORT const char *qInstallPathLibs();
Q_CORE_EXPORT const char *qInstallPathBins();
Q_CORE_EXPORT const char *qInstallPathPlugins();
Q_CORE_EXPORT const char *qInstallPathData();
Q_CORE_EXPORT const char *qInstallPathTranslations();
Q_CORE_EXPORT const char *qInstallPathSysconf();


class QBool
{
    bool b;

public:
    inline explicit QBool(bool B) : b(B) {}
    inline operator const void *() const
    { return b ? static_cast<const void *>(this) : static_cast<const void *>(0); }
};


/*
 compilers which follow outdated template instantiation rules
 require a class to have a comparison operator to exist when
 a QList of this type is instantiated. It's not actually
 used in the list, though. Hence the dummy implementation.
 Just in case other code relies on it we better trigger a warning
 mandating a real implementation.
*/
#ifdef Q_FULL_TEMPLATE_INSTANTIATION
#  define Q_DUMMY_COMPARISON_OPERATOR(C) \
    bool operator==(const C&) const { \
        qWarning(#C"::operator==(const "#C"&) was called"); \
        return false; \
    }
#else
#  define Q_DUMMY_COMPARISON_OPERATOR(C)
#endif


/*
  QTypeInfo     - type trait functionality
  qInit         - type initialization
  qIsDetached   - data sharing functionality
*/

#ifndef QT_NO_PARTIAL_TEMPLATE_SPECIALIZATION

/*
  The catch-all template.
*/
template <typename T> inline void qInit(T &) { }
template <typename T> inline bool qIsDetached(T &) { return true; }

template <typename T>
class QTypeInfo
{
public:
    enum {
        isPointer = false,
        isComplex = true,
        isStatic  = true,
        isLarge   = (sizeof(T)>sizeof(void*))
    };
};

/*
  The partial specialization to catch all pointers.
*/
template <typename T> inline void qInit(T *&t) { t = 0; }
template <typename T> inline void qInit(const T *&t) { t = 0; }

template <typename T>
class QTypeInfo<T*>
{
public:
    enum {
        isPointer = true,
        isComplex = false,
        isStatic  = false,
        isLarge   = false
    };
};

#else

/*
  Lack of partial template specialization mostly on MSVC compilers
  makes it hard to distinguish between pointers and non-pointer types.
 */
template <typename T> inline void qInitHelper(T*(*)(), void* ptr) { *(void**)ptr = 0; }
inline void qInitHelper(...) { }

template <typename T> char QTypeInfoHelper(T*(*)());
void* QTypeInfoHelper(...);

template <typename T> inline void qInit(T &t){ qInitHelper((T(*)())0, (void*)&t); }
template <typename T> inline bool qIsDetached(T &) { return true; }

template <typename T>
class QTypeInfo
{
public:
    enum {
        isPointer = (1 == sizeof(QTypeInfoHelper((T(*)())0))),
        isComplex = !isPointer,
        isStatic  = !isPointer,
        isLarge   = (sizeof(T)>sizeof(void*))
    };
};

#endif // QT_NO_PARTIAL_TEMPLATE_SPECIALIZATION


/*
  Specialize a specific type with:

    Q_DECLARE_TYPEINFO(type, flags);

  where 'type' is the name of the type to specialize and 'flags' is
  logically-OR'ed combination of the flags below.
*/
enum { // TYPEINFO flags
    Q_COMPLEX_TYPE   = 0,
    Q_PRIMITIVE_TYPE = 1,
    Q_STATIC_TYPE    = 0,
    Q_MOVABLE_TYPE   = 2
};

#define Q_DECLARE_TYPEINFO(TYPE, FLAGS) \
template <> \
class QTypeInfo<TYPE> \
{ \
public: \
    enum { \
        isComplex = ((FLAGS & Q_PRIMITIVE_TYPE) == 0), \
        isStatic  = ((FLAGS & (Q_MOVABLE_TYPE|Q_PRIMITIVE_TYPE)) == 0), \
        isLarge   = (sizeof(TYPE)>sizeof(void*)), \
        isPointer = false \
    }; \
    static inline const char *name() { return #TYPE; } \
}

/*
  Specialize a shared type with:

    Q_DECLARE_SHARED(type);

  where 'type' is the name of the type to specialize.  NOTE: shared
  types must declare a 'bool isDetached(void) const;' member for this
  to work.
*/
#define Q_DECLARE_SHARED(TYPE) \
template <> inline bool qIsDetached<TYPE>(TYPE &t) { return t.isDetached(); }

/*
  QTypeInfo primitive specializations
*/
Q_DECLARE_TYPEINFO(bool, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(char, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(signed char, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(uchar, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(short, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(ushort, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(int, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(uint, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(long, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(ulong, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Q_LONGLONG, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Q_ULONGLONG, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(float, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(double, Q_PRIMITIVE_TYPE);
#ifndef Q_OS_DARWIN
Q_DECLARE_TYPEINFO(long double, Q_PRIMITIVE_TYPE);
#endif

template <> inline void qInit<bool>(bool &t) { t = false; }
template <> inline void qInit<char>(char &t) { t = 0; }
template <> inline void qInit<signed char>(signed char &t) { t = 0; }
template <> inline void qInit<uchar>(uchar &t) { t = 0; }
template <> inline void qInit<short>(short &t) { t = 0; }
template <> inline void qInit<ushort>(ushort &t) { t = 0; }
template <> inline void qInit<int>(int &t) { t = 0; }
template <> inline void qInit<uint>(uint &t) { t = 0; }
template <> inline void qInit<long>(long &t) { t = 0; }
template <> inline void qInit<ulong>(ulong &t) { t = 0; }
template <> inline void qInit<Q_LONGLONG>(Q_LONGLONG &t) { t = 0; }
template <> inline void qInit<Q_ULONGLONG>(Q_ULONGLONG &t) { t = 0; }
template <> inline void qInit<float>(float &t) { t = 0.0f; }
template <> inline void qInit<double>(double &t) { t = 0.0; }
#ifndef Q_OS_DARWIN
template <> inline void qInit<long double>(long double &t) { t = 0.0; }
#endif

/*
   These functions make it possible to use standard C++ functions with
   a similar name from Qt header files (especially template classes).
*/
Q_CORE_EXPORT void *qMalloc(size_t size);
Q_CORE_EXPORT void qFree(void *ptr);
Q_CORE_EXPORT void *qRealloc(void *ptr, size_t size);
Q_CORE_EXPORT void *qMemCopy(void *dest, const void *src, size_t n);
Q_CORE_EXPORT void *qMemSet(void *dest, int c, size_t n);


/*
    Avoid some particularly useless warnings from some stupid compilers.
    To get ALL C++ compiler warnings, define QT_CC_WARNINGS or comment out
    the line "#define QT_NO_WARNINGS".
*/

#if !defined(QT_CC_WARNINGS)
#  define QT_NO_WARNINGS
#endif
#if defined(QT_NO_WARNINGS)
#  if defined(Q_CC_MSVC)
#    pragma warning(disable: 4251) // class 'A' needs to have dll interface for to be used by clients of class 'B'.
#    pragma warning(disable: 4244) // 'conversion' conversion from 'type1' to 'type2', possible loss of data
#    pragma warning(disable: 4275) // non - DLL-interface classkey 'identifier' used as base for DLL-interface classkey 'identifier'
#    pragma warning(disable: 4514) // unreferenced inline/local function has been removed
#    pragma warning(disable: 4800) // 'type' : forcing value to bool 'true' or 'false' (performance warning)
#    pragma warning(disable: 4097) // typedef-name 'identifier1' used as synonym for class-name 'identifier2'
#    pragma warning(disable: 4706) // assignment within conditional expression
#    pragma warning(disable: 4786) // truncating debug info after 255 characters
#    pragma warning(disable: 4660) // template-class specialization 'identifier' is already instantiated
#    pragma warning(disable: 4355) // 'this' : used in base member initializer list
#    pragma warning(disable: 4231) // nonstandard extension used : 'extern' before template explicit instantiation
#    pragma warning(disable: 4710) // function not inlined
#    pragma warning(disable: 4530) // C++ exception handler used, but unwind semantics are not enabled. Specify -GX
#  elif defined(Q_CC_BOR)
#    pragma option -w-inl
#    pragma option -w-aus
#    pragma warn -inl
#    pragma warn -pia
#    pragma warn -ccc
#    pragma warn -rch
#    pragma warn -sig
#  endif
#endif

class Q_CORE_EXPORT QFlag
{
    int i;
public:
    inline QFlag(int i) : i(i) {}
    inline operator int() const { return i; }
};


//#define Q_NO_TYPESAFE_FLAGS

#ifndef Q_NO_TYPESAFE_FLAGS

template<typename Enum>
class QFlags
{
    typedef void *Zero;
    int i;
public:
    typedef Enum enum_type;
    inline QFlags(const QFlags &f) : i(f.i) {}
    inline QFlags(Enum f) : i(f) {}
    inline QFlags(Zero * = 0) : i(0) {}
    inline QFlags(QFlag f) : i(f) {}

    inline QFlags &operator=(const QFlags &f) { i = f.i; return *this; }
    inline QFlags &operator&=(int mask) {  i &= mask; return *this; }
    inline QFlags &operator&=(uint mask) {  i &= mask; return *this; }
    inline QFlags &operator|=(QFlags f) {  i |= f.i; return *this; }
    inline QFlags &operator|=(Enum f) {  i |= f; return *this; }
    inline QFlags &operator^=(QFlags f) {  i ^= f.i; return *this; }
    inline QFlags &operator^=(Enum f) {  i ^= f; return *this; }


    inline operator int() const { return i;}

    inline QFlags operator|(QFlags f) const { QFlags g; g.i = i | f.i; return g; }
    inline QFlags operator|(Enum f) const { QFlags g; g.i = i | f; return g; }
    inline int operator|(int f) const { return i | f; }
    inline QFlags operator^(QFlags f) const { QFlags g; g.i = i ^ f.i; return g; }
    inline QFlags operator^(Enum f) const { QFlags g; g.i = i ^ f; return g; }
    inline int operator^(int f) const { return i ^ f; }
    inline QFlags operator&(int mask) const { QFlags g; g.i = i & mask; return g; }
    inline QFlags operator&(uint mask) const { QFlags g; g.i = i & mask; return g; }
    inline QFlags operator&(Enum f) const { QFlags g; g.i = i & f; return g; }
    inline QFlags operator~() const { QFlags g; g.i = ~i; return g; }

    inline bool operator!() const { return !i; }
};

#define Q_DECLARE_FLAGS(Flags, Enum)\
typedef QFlags<Enum> Flags;
#define Q_DECLARE_OPERATORS_FOR_FLAGS(Flags) \
inline QFlags<Flags::enum_type> operator|(Flags::enum_type f1, Flags::enum_type f2) \
{ return QFlags<Flags::enum_type>(f1) | f2; } \
inline QFlags<Flags::enum_type> operator|(Flags::enum_type f1, QFlags<Flags::enum_type> f2) \
{ return f2 | f1; } \
inline int operator|(int f1, QFlags<Flags::enum_type> f2) \
{ return f2 | f1; }

#else // Q_NO_TYPESAFE_FLAGS

#define Q_DECLARE_FLAGS(Flags, Enum)\
typedef uint Flags
#define Q_DECLARE_OPERATORS_FOR_FLAGS(Flags)

#endif // Q_NO_TYPESAFE_FLAGS


#if defined(Q_CC_GNU) && !defined(Q_CC_INTEL) && !(__GNUC__ == 2 && __GNUC_MINOR__ <= 95)
// make use of typeof-extension
template <typename T>
class QForeachContainer {
public:
    inline QForeachContainer(const T& t) : c(t), i(c.begin()), e(c.end()) { }
    const T c;
    typename T::const_iterator i, e;
};

#define Q_FOREACH(variable, container) \
for (QForeachContainer<__typeof__(container)> _container_(container); _container_.i != _container_.e; \
    ++_container_.i) \
    for (variable = *_container_.i;;({break;}))

#elif (defined Q_CC_MSVC && _MSC_VER < 1300) || (defined Q_CC_XLC)

template <typename T>
class QForeachContainer  {
public:
    inline QForeachContainer(const T& t): c(t), i(c.begin()), e(c.end()){};
    const T c;
    typename T::const_iterator i, e;
    inline bool condition() const { if (i != e) return true; this->~QForeachContainer(); return false; }
};

struct QForeachMemory {
    inline QForeachMemory(void *):done(0){}
    int done;
    char padding[256];
};

template<typename T>
QForeachContainer<T> qForeachSizeofContainerHelper(const T &);

template <typename T>
inline QForeachContainer<T> *qForeachContainer(const T &, QForeachMemory &memory)
{ return reinterpret_cast<QForeachContainer<T>*>(memory.padding); }

template <typename T>
inline void *qForeachContainerNew(const T& t, QForeachMemory &memory)
{
    Q_ASSERT_X(sizeof(QForeachContainer<T>) < 256, "foreach", "Unsupported container");
    return new (memory.padding) QForeachContainer<T>(t);
}

#define Q_FOREACH(variable, container) \
for (QForeachMemory _container_ = qForeachContainerNew(container, _container_); \
    qForeachContainer(container, _container_)->condition();\
    ++qForeachContainer(container, _container_)->i) \
    for (variable = *qForeachContainer(container, _container_)->i; --_container_.done; _container_.done = 1)

#else

template <typename T>
class QForeachContainer  {
public:
    inline QForeachContainer(const T& t): c(t), i(c.begin()), e(c.end()){};
    const T c;
    typename T::const_iterator i, e;
    inline bool condition() const { if (i != e) return true; this->~QForeachContainer(); return false; }
};

template <int size>
struct QForeachMemory {
    inline QForeachMemory(void *):done(0){}
    int done;
    char padding[size];
};

template<typename T>
QForeachContainer<T> qForeachSizeofContainerHelper(const T &);

template <typename T, typename U>
inline QForeachContainer<T> *qForeachContainer(const T &, U &memory)
{ return reinterpret_cast<QForeachContainer<T>*>(memory.padding); }

template <typename T, typename U>
inline void *qForeachContainerNew(const T& t, U &memory)
{ return new (memory.padding) QForeachContainer<T>(t); }

#define Q_FOREACH(variable, container) \
for (QForeachMemory<sizeof(qForeachSizeofContainerHelper(container))> _container_ = qForeachContainerNew(container, _container_); qForeachContainer(container, _container_)->condition();\
    ++qForeachContainer(container, _container_)->i) \
    for (variable = *qForeachContainer(container, _container_)->i; --_container_.done; _container_.done = 1)

#endif

#define Q_FOREVER for(;;)
#ifndef QT_NO_KEYWORDS
#  ifndef foreach
#    define foreach Q_FOREACH
#  endif
#  ifndef forever
#    define forever Q_FOREVER
#  endif
#endif

#if 0
// tell gcc to use its built-in methods for some common functions
#if defined(QT_NO_DEBUG) && defined(Q_CC_GNU)
#  define qMemCopy __builtin_memcpy
#  define qMemSet __builtin_memset
#endif
#endif

#define Q_DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(d_ptr); } \
    inline const Class##Private* d_func() const { return reinterpret_cast<const Class##Private *>(d_ptr); } \
    inline Class* q_func() { return this; } \
    inline const Class* q_func() const { return this; } \
    friend class Class##Private;

#define Q_DECLARE_PUBLIC(Class) \
    inline Class##Private* d_func() { return this; } \
    inline const Class##Private* d_func() const { return this; } \
    inline Class* q_func() { return static_cast<Class *>(q_ptr); } \
    inline const Class* q_func() const { return static_cast<const Class *>(q_ptr); } \
    friend class Class;

#define Q_D(Class) Class##Private *d = d_func()
#define Q_Q(Class) Class *q = q_func()

#define QT_TR_NOOP(x) (x)
#define QT_TRANSLATE_NOOP(scope, x) (x)

/*
   Some classes do not permit copies to be made of an object. These
   classes contains a private copy constructor and assignment
   operator to disable copying (the compiler gives an error message).
*/

#ifndef Q_NO_UNRESOLVED_EXTERNALS
#define Q_DISABLE_COPY(Class) \
    Class(const Class &); \
    Class &operator=(const Class &);
#else
#define Q_DISABLE_COPY(Class)
#endif

#endif /* __cplusplus */

#endif /* QGLOBAL_H */
