/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009, 2010 Ali Polatel <alip@exherbo.org>
 *
 * This file is part of the sydbox sandbox tool. sydbox is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * sydbox is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SYDBOX_GUARD_DISPATCH_H
#define SYDBOX_GUARD_DISPATCH_H 1

#include <stdbool.h>

#include <pinktrace/pink.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#if PINKTRACE_BITNESS_COUNT_SUPPORTED == 1
void dispatch_init(void);
void dispatch_free(void);
int dispatch_lookup(int sno, pink_bitness_t bitness);
bool dispatch_chdir(int sno, pink_bitness_t bitness);
bool dispatch_dup(int sno, pink_bitness_t bitness);
bool dispatch_fcntl(int sno, pink_bitness_t bitness);
bool dispatch_maygetsockname(int sno, pink_bitness_t bitness, bool *decode);
#elif PINKTRACE_BITNESS_COUNT_SUPPORTED == 2
void dispatch_init32(void);
void dispatch_init64(void);
void dispatch_free32(void);
void dispatch_free64(void);
int dispatch_lookup32(int sno);
int dispatch_lookup64(int sno);
const char *dispatch_name32(int sno);
const char *dispatch_name64(int sno);
bool dispatch_chdir32(int sno);
bool dispatch_chdir64(int sno);
bool dispatch_dup32(int sno);
bool dispatch_dup64(int sno);
bool dispatch_fcntl32(int sno);
bool dispatch_fcntl64(int sno);
bool dispatch_maygetsockname32(int sno, bool *decode);
bool dispatch_maygetsockname64(int sno, bool *decode);

#define dispatch_init()     \
    do {                    \
        dispatch_init32();  \
        dispatch_init64();  \
    } while (0)
#define dispatch_free()     \
    do {                    \
        dispatch_free32();  \
        dispatch_free64();  \
    } while (0)
#define dispatch_lookup(sno, bitness) \
    ((bitness) == PINK_BITNESS_32) ? dispatch_lookup32((sno)) : dispatch_lookup64((sno))
#define dispatch_chdir(sno, bitness) \
    ((bitness) == PINK_BITNESS_32) ? dispatch_chdir32((sno)) : dispatch_chdir64((sno))
#define dispatch_dup(sno, bitness) \
    ((bitness) == PINK_BITNESS_32) ? dispatch_dup32((sno)) : dispatch_dup64((sno))
#define dispatch_fcntl(sno, bitness) \
    ((bitness) == PINK_BITNESS_32) ? dispatch_fcntl32((sno)) : dispatch_fcntl64((sno))
#define dispatch_maygetsockname(sno, bitness, decode) \
    ((bitness) == PINK_BITNESS_32)                    \
        ? dispatch_maygetsockname32((sno), (decode))  \
        : dispatch_maygetsockname64((sno), (decode))

#else
#error unsupported bitness count
#endif

#endif // SYDBOX_GUARD_DISPATCH_H

