/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
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

#ifndef SYDBOX_GUARD_NET_H
#define SYDBOX_GUARD_NET_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <limits.h>
#include <stdbool.h>
#include <netinet/in.h>

#include <glib.h>

struct sydbox_addr {
    int family;

    union {
        struct {
            bool abstract;
            bool exact;
            char sun_path[PATH_MAX];
            char *rsun_path;
        } saun;

        struct {
            int netmask;
            int port[2];
            struct in_addr sin_addr;
        } sa;

#if HAVE_IPV6
        struct {
            int netmask;
            int port[2];
            struct in6_addr sin6_addr;
        } sa6;
#endif /* HAVE_IPV6 */
    } u;
};

bool address_cmp(const struct sydbox_addr *addr1, const struct sydbox_addr *addr2);

struct sydbox_addr *address_dup(const struct sydbox_addr *src);

bool address_has(struct sydbox_addr *haystack, struct sydbox_addr *needle);

char *address_to_string(const struct sydbox_addr *addr);

struct sydbox_addr *address_from_string(const gchar *addr, bool canlog);

#endif // SYDBOX_GUARD_NET_H

