#pragma once

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
#error This header file is intended for use in the MATRIXNET_WITHOUT_ARCADIA mode only.
#endif

#if defined(__GNUC__)
#define Y_PREFETCH_READ(Pointer, Priority) __builtin_prefetch((const void*)(Pointer), 0, Priority)
#else
#define Y_PREFETCH_READ(Pointer, Priority) (void)(const void*)(Pointer), (void)Priority
#endif

#if defined(__GNUC__)
#define Y_LIKELY(Cond) __builtin_expect(!!(Cond), 1)
#define Y_UNLIKELY(Cond) __builtin_expect(!!(Cond), 0)
#else
#define Y_LIKELY(Cond) (Cond)
#define Y_UNLIKELY(Cond) (Cond)
#endif

#if !defined(Y_FORCE_INLINE)
#if defined(_MSC_VER)
#define Y_FORCE_INLINE __forceinline
#elif defined(__GNUC__)
#/* Clang also defines __GNUC__ (as 4) */
#define Y_FORCE_INLINE inline __attribute__((__always_inline__))
#else
#define Y_FORCE_INLINE inline
#endif
#endif
