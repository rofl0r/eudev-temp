/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#pragma once

/***
  This file is part of systemd.

  Copyright 2010 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <assert.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <inttypes.h>

#define _printf_(a,b) __attribute__ ((format (printf, a, b)))
#define _alloc_(...) __attribute__ ((alloc_size(__VA_ARGS__)))
#define _sentinel_ __attribute__ ((sentinel))
#define _noreturn_ __attribute__((noreturn))
#define _pure_ __attribute__ ((pure))
#define _const_ __attribute__ ((const))
#define _packed_ __attribute__ ((packed))
#define _malloc_ __attribute__ ((malloc))
#define _likely_(x) (__builtin_expect(!!(x),1))
#define _unlikely_(x) (__builtin_expect(!!(x),0))
#define _public_ __attribute__ ((visibility("default")))
#define _cleanup_(x) __attribute__((cleanup(x)))

/* Rounds up */

#define ALIGN4(l) (((l) + 3) & ~3)
#define ALIGN8(l) (((l) + 7) & ~7)

#if __SIZEOF_POINTER__ == 8
#define ALIGN(l) ALIGN8(l)
#elif __SIZEOF_POINTER__ == 4
#define ALIGN(l) ALIGN4(l)
#else
#error "Wut? Pointers are neither 4 nor 8 bytes long?"
#endif

static inline size_t ALIGN_TO(size_t l, size_t ali) {
        return ((l + ali - 1) & ~(ali - 1));
}

#define ALIGN_TO_PTR(p, ali) ((void*) ALIGN_TO((unsigned long) p))

#define ELEMENTSOF(x) (sizeof(x)/sizeof((x)[0]))

/*
 * container_of - cast a member of a structure out to the containing structure
 * @ptr: the pointer to the member.
 * @type: the type of the container struct this is embedded in.
 * @member: the name of the member within the struct.
 *
 */

#define container_of(ptr, type, member)                                 \
        __extension__ ({                                                \
                        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                        (type *)( (char *)__mptr - offsetof(type,member) ); \
                })

#define assert_se(expr)                                                 \
        do {                                                            \
                if (_unlikely_(!(expr)))                                \
                        log_assert_failed(#expr, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
        } while (false)                                                 \

/* We override the glibc assert() here. */
#undef assert
#ifdef NDEBUG
#define assert(expr) do {} while(false)
#else
#define assert(expr) assert_se(expr)
#endif

#define assert_not_reached(t)                                           \
        do {                                                            \
                log_assert_failed_unreachable(t, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
        } while (false)

#if defined(static_assert)
#define assert_cc(expr)                         \
        do {                                    \
                static_assert(expr, #expr);     \
        } while (false)
#else
#define assert_cc(expr)                         \
        do {                                    \
                switch (0) {                    \
                case 0:                         \
                case !!(expr):                  \
                        ;                       \
                }                               \
        } while (false)
#endif

#define PTR_TO_INT(p) ((int) ((intptr_t) (p)))
#define INT_TO_PTR(u) ((void *) ((intptr_t) (u)))
#define PTR_TO_UINT(p) ((unsigned int) ((uintptr_t) (p)))
#define UINT_TO_PTR(u) ((void *) ((uintptr_t) (u)))

#define PTR_TO_LONG(p) ((long) ((intptr_t) (p)))
#define LONG_TO_PTR(u) ((void *) ((intptr_t) (u)))
#define PTR_TO_ULONG(p) ((unsigned long) ((uintptr_t) (p)))
#define ULONG_TO_PTR(u) ((void *) ((uintptr_t) (u)))

#define PTR_TO_INT32(p) ((int32_t) ((intptr_t) (p)))
#define INT32_TO_PTR(u) ((void *) ((intptr_t) (u)))
#define PTR_TO_UINT32(p) ((uint32_t) ((uintptr_t) (p)))
#define UINT32_TO_PTR(u) ((void *) ((uintptr_t) (u)))

#define PTR_TO_INT64(p) ((int64_t) ((intptr_t) (p)))
#define INT64_TO_PTR(u) ((void *) ((intptr_t) (u)))
#define PTR_TO_UINT64(p) ((uint64_t) ((uintptr_t) (p)))
#define UINT64_TO_PTR(u) ((void *) ((uintptr_t) (u)))

#define memzero(x,l) (memset((x), 0, (l)))

#define char_array_0(x) x[sizeof(x)-1] = 0;

#define IOVEC_SET_STRING(i, s)                  \
        do {                                    \
                struct iovec *_i = &(i);        \
                char *_s = (char *)(s);         \
                _i->iov_base = _s;              \
                _i->iov_len = strlen(_s);       \
        } while(false)

static inline size_t IOVEC_TOTAL_SIZE(const struct iovec *i, unsigned n) {
        unsigned j;
        size_t r = 0;

        for (j = 0; j < n; j++)
                r += i[j].iov_len;

        return r;
}

static inline size_t IOVEC_INCREMENT(struct iovec *i, unsigned n, size_t k) {
        unsigned j;

        for (j = 0; j < n; j++) {
                size_t sub;

                if (_unlikely_(k <= 0))
                        break;

                sub = MIN(i[j].iov_len, k);
                i[j].iov_len -= sub;
                i[j].iov_base = (uint8_t*) i[j].iov_base + sub;
                k -= sub;
        }

        return k;
}

 /* Because statfs.t_type can be int on some architecures, we have to cast
  * the const magic to the type, otherwise the compiler warns about
  * signed/unsigned comparison, because the magic can be 32 bit unsigned.
 */
#define F_TYPE_CMP(a, b) (a == (typeof(a)) b)


/* Returns the number of chars needed to format variables of the
 * specified type as a decimal string. Adds in extra space for a
 * negative '-' prefix. */

#define DECIMAL_STR_MAX(type)                                           \
        (1+(sizeof(type) <= 1 ? 3 :                                     \
            sizeof(type) <= 2 ? 5 :                                     \
            sizeof(type) <= 4 ? 10 :                                    \
            sizeof(type) <= 8 ? 20 : sizeof(int[-2*(sizeof(type) > 8)])))

#include "log.h"
