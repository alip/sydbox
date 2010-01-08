/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009, 2010 Ali Polatel <alip@exherbo.org>
 * Based in part upon courier which is:
 *     Copyright 1998-2009 Double Precision, Inc
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <glib.h>

#include "syd-log.h"
#include "syd-net.h"

bool address_cmp(const struct sydbox_addr *addr1, const struct sydbox_addr *addr2)
{
    if (addr1->family != addr2->family)
        return false;
    else if (addr1->netmask != addr2->netmask)
        return false;
    else if (addr1->port[0] != addr2->port[0])
        return false;
    else if (addr1->port[1] != addr2->port[1])
        return false;

    switch (addr1->family) {
        case AF_UNIX:
            return (0 == strncmp(addr1->u.sun_path, addr2->u.sun_path, PATH_MAX));
        case AF_INET:
            return (0 == memcmp(&addr1->u.sin_addr, &addr2->u.sin_addr, sizeof(struct in_addr)));
#if HAVE_IPV6
        case AF_INET6:
            return (0 == memcmp(&addr1->u.sin6_addr, &addr2->u.sin6_addr, sizeof(struct in6_addr)));
#endif /* HAVE_IPV6 */
        default:
            g_assert_not_reached();
    }
}

struct sydbox_addr *address_dup(const struct sydbox_addr *src)
{
    struct sydbox_addr *dest;

    dest = g_new(struct sydbox_addr, 1);

    dest->family = src->family;
    dest->netmask = src->netmask;
    dest->port[0] = src->port[0];
    dest->port[1] = src->port[1];
    switch (src->family) {
        case AF_UNIX:
            strncpy(dest->u.sun_path, src->u.sun_path, PATH_MAX);
            break;
        case AF_INET:
            memcpy(&dest->u.sin_addr, &src->u.sin_addr, sizeof(struct in_addr));
            break;
#if HAVE_IPV6
        case AF_INET6:
            memcpy(&dest->u.sin6_addr, &src->u.sin6_addr, sizeof(struct in6_addr));
            break;
#endif /* HAVE_IPV6 */
        default:
            g_assert_not_reached();
    }
    return dest;
}

bool address_has(struct sydbox_addr *haystack, struct sydbox_addr *needle)
{
    int n, mask;
    unsigned char *b, *ptr;

    if (needle->family != haystack->family)
        return false;

    switch (needle->family) {
        case AF_UNIX:
            return (0 == strncmp(needle->u.sun_path, haystack->u.sun_path, strlen(haystack->u.sun_path) + 1));
        case AF_INET:
            ptr = (unsigned char *)&needle->u.sin_addr;
            b = (unsigned char *)&haystack->u.sin_addr;
            break;
#if HAVE_IPV6
        case AF_INET6:
            ptr = (unsigned char *)&needle->u.sin6_addr;
            b = (unsigned char *)&haystack->u.sin6_addr;
            break;
#endif /* HAVE_IPV6 */
        default:
            return false;
    }

    n = haystack->netmask;
    while (n >= 8) {
        if (*ptr != *b)
            return false;
        ++ptr;
        ++b;
        n -= 8;
    }

    if (n != 0) {
        mask = ((~0) << (8 - n)) & 255;

        if ((*ptr ^ *b) & mask)
            return false;
    }
    return true;
}

struct sydbox_addr *address_from_string(const gchar *src, bool canlog)
{
    char *addr, *netmask, *p, *port_range, *delim;
    struct sydbox_addr *saddr;

    saddr = g_new0(struct sydbox_addr, 1);

    if (0 == strncmp(src, "unix://", 7)) {
        saddr->family = AF_UNIX;
        saddr->port[0] = -1;
        saddr->port[1] = -1;
        strncpy(saddr->u.sun_path, src + 7, PATH_MAX);
        saddr->u.sun_path[PATH_MAX - 1] = '\0';
        if (canlog)
            g_info("New whitelist address {family=AF_UNIX path=%s}", saddr->u.sun_path);
    }
    else if (0 == strncmp(src, "inet://", 7)) {
        saddr->family = AF_INET;

        addr = g_strdup(src + 7);

        /* Find out port */
        port_range = strrchr(addr, '@');
        if (NULL == port_range) {
            g_free(addr);
            g_free(saddr);
            return NULL;
        }
        addr[port_range - addr] = '\0';

        delim = strchr(++port_range, '-');
        if (NULL == delim) {
            saddr->port[0] = atoi(port_range);
            saddr->port[1] = saddr->port[0];
        }
        else {
            port_range[delim - port_range] = '\0';
            saddr->port[0] = atoi(port_range);
            saddr->port[1] = atoi(++delim);
        }

        /* Find out netmask */
        netmask = strrchr(addr, '/');
        if (netmask == NULL) {
            /* Netmask not specified, figure it out. */
            saddr->netmask = 8;
            p = addr;
            while (*p != '\0') {
                if (*p++ == '.') {
                    if (*p == '\0')
                        break;
                    saddr->netmask += 8;
                }
            }
        }
        else {
            saddr->netmask = atoi(netmask + 1);
            addr[netmask - addr] = '\0';
        }

        if (0 >= inet_pton(AF_INET, addr, &saddr->u.sin_addr)) {
            g_free(addr);
            g_free(saddr);
            return NULL;
        }
        if (canlog)
            g_info("New whitelist address {family=AF_INET addr=%s netmask=%d port_range=%d-%d}",
                    addr, saddr->netmask, saddr->port[0], saddr->port[1]);
        g_free(addr);
    }
    else if (0 == strncmp(src, "inet6://", 8)) {
#if HAVE_IPV6
        saddr->family = AF_INET6;

        addr = g_strdup(src + 8);

        /* Find out port */
        port_range = strrchr(addr, '@');
        if (NULL == port_range || (port_range + 1) == '\0') {
            g_free(addr);
            g_free(saddr);
            return NULL;
        }
        addr[port_range - addr] = '\0';

        delim = strchr(++port_range, '-');
        if (NULL == delim) {
            saddr->port[0] = atoi(port_range);
            saddr->port[1] = saddr->port[0];
        }
        else {
            port_range[delim - port_range] = '\0';
            saddr->port[0] = atoi(port_range);
            saddr->port[1] = atoi(++delim);
        }

        /* Find out netmask */
        netmask = strrchr(addr, '/');
        if (netmask == NULL) {
            /* Netmask not give, figure it out */
            saddr->netmask = 16;
            p = addr;
            while (*p != '\0') {
                if (*p++ == ':') {
                    /* ip:: ends the prefix right here,
                     * but ip::ip is a full IPv6 address.
                     */
                    if (*p == ':') {
                        if (p[1] != '\0')
                            saddr->netmask = sizeof(struct in6_addr) * 8;
                        break;
                    }
                    if (*p == '\0')
                        break;
                    saddr->netmask += 16;
                }
            }
        }
        else {
            saddr->netmask = atoi(netmask + 1);
            addr[netmask - addr] = '\0';
        }

        if (0 >= inet_pton(AF_INET6, addr, &saddr->u.sin6_addr)) {
            g_free(addr);
            g_free(saddr);
            return NULL;
        }
        if (canlog)
            g_info("New whitelist address {family=AF_INET6 addr=%s netmask=%d port_range=%d-%d}",
                    addr, saddr->netmask, saddr->port[0], saddr->port[1]);
        g_free(addr);
#else
        g_warning("inet6:// not supported (no IPV6 support)");
        g_free(saddr);
        return NULL;
#endif /* HAVE_IPV6 */
    }
    else {
        g_free(saddr);
        return NULL;
    }
    return saddr;
}

