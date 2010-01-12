/* vim: set et ts=4 sts=4 sw=4 fdm=syntax : */

/*
 * Copyright (c) 2010 Ali Polatel <alip@exherbo.org>
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

#include <errno.h>
#include <string.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <glib.h>

#include "syd-net.h"

#include "test-helpers.h"

static void test1(void)
{
    struct sydbox_addr *addr;

    addr = address_from_string("invalid://", false);
    XFAIL_UNLESS(addr == NULL, "returned non-null for invalid address\n");
    addr = address_from_string("inet://127.0.0.1", false);
    XFAIL_UNLESS(addr == NULL, "returned non-null for inet address without port range\n");
#if HAVE_IPV6
    addr = address_from_string("inet6://::1", false);
    XFAIL_UNLESS(addr == NULL, "returned non-null for inet6 address without port range\n");
#endif /* HAVE_IPV6 */
}

static void test2(void)
{
    struct sydbox_addr *addr;

    addr = address_from_string("unix:///dev/null", false);
    XFAIL_IF(addr == NULL, "returned NULL for valid unix address\n");
    XFAIL_UNLESS(addr->family == AF_UNIX, "wrong family, got:%d expected:%d\n", addr->family, AF_UNIX);
    XFAIL_UNLESS(addr->u.saun.abstract == false, "non-abstract address marked abstract");
    XFAIL_UNLESS(addr->u.saun.exact == false, "not a pattern");
    XFAIL_UNLESS(0 == strncmp(addr->u.saun.sun_path, "/dev/null", 10),
            "wrong path got:`%s' expected:/dev/null\n", addr->u.saun.sun_path);
    g_free(addr);
}

static void test3(void)
{
    struct sydbox_addr *addr;

    addr = address_from_string("unix-abstract:///tmp/.X11-unix/X0", false);
    XFAIL_IF(addr == NULL, "returned NULL for valid unix address\n");
    XFAIL_UNLESS(addr->family == AF_UNIX, "wrong family, got:%d expected:%d\n", addr->family, AF_UNIX);
    XFAIL_UNLESS(addr->u.saun.abstract == true, "abstract address marked non-abstract");
    XFAIL_UNLESS(0 == strncmp(addr->u.saun.sun_path, "/tmp/.X11-unix/X0", 18),
            "wrong path got:`%s' expected:/tmp/.X11-unix/X0\n",
            addr->u.saun.sun_path);
    g_free(addr);
}

static void test4(void)
{
    char ip[100] = { 0 };
    struct sydbox_addr *addr;

    addr = address_from_string("inet://127.0.0.1@3", false);
    XFAIL_IF(addr == NULL, "returned NULL for valid inet address\n");
    XFAIL_UNLESS(addr->family == AF_INET, "wrong family, got:%d expected:%d\n", addr->family, AF_INET);
    XFAIL_UNLESS(addr->u.sa.netmask == 32, "wrong netmask, got:%d expected:32\n", addr->u.sa.netmask);
    XFAIL_UNLESS(addr->u.sa.port[0] == 3, "wrong port[0], got:%d expected:3\n", addr->u.sa.port[0]);
    XFAIL_UNLESS(addr->u.sa.port[1] == 3, "wrong port[1], got:%d expected:3\n", addr->u.sa.port[1]);
    XFAIL_IF(NULL == inet_ntop(AF_INET, &addr->u.sa.sin_addr, ip, sizeof(ip)),
            "inet_ntop failed: %s\n", g_strerror(errno));
    XFAIL_UNLESS(0 == strncmp(ip, "127.0.0.1", 10), "wrong ip got:`%s' expected:127.0.0.1\n", ip);
    g_free(addr);
}

#if HAVE_IPV6
static void test5(void)
{
    char ip[100] = { 0 };
    struct sydbox_addr *addr;

    addr = address_from_string("inet6://::1@3", false);
    XFAIL_IF(addr == NULL, "returned NULL for valid inet6 address\n");
    XFAIL_UNLESS(addr->family == AF_INET6, "wrong family, got:%d expected:%d\n", addr->family, AF_INET6);
    XFAIL_UNLESS(addr->u.sa6.netmask == 128, "wrong netmask, got:%d expected:128\n", addr->u.sa6.netmask);
    XFAIL_UNLESS(addr->u.sa6.port[0] == 3, "wrong port[0], got:%d expected:3\n", addr->u.sa6.port[0]);
    XFAIL_UNLESS(addr->u.sa6.port[1] == 3, "wrong port[1], got:%d expected:3\n", addr->u.sa6.port[1]);
    XFAIL_IF(NULL == inet_ntop(AF_INET6, &addr->u.sa6.sin6_addr, ip, sizeof(ip)),
            "inet_ntop failed: %s\n", g_strerror(errno));
    XFAIL_UNLESS(0 == strncasecmp(ip, "::1", 4), "wrong ip got:`%s' expected:`::1'\n", ip);
    g_free(addr);
}
#endif /* HAVE_IPV6 */

static void test6(void)
{
    char ip[100] = { 0 };
    struct sydbox_addr *addr;

    addr = address_from_string("inet://127.0.0.1@3-5", false);
    XFAIL_IF(addr == NULL, "returned NULL for valid inet address\n");
    XFAIL_UNLESS(addr->family == AF_INET, "wrong family, got:%d expected:%d\n", addr->family, AF_INET);
    XFAIL_UNLESS(addr->u.sa.netmask == 32, "wrong netmask, got:%d expected:32\n", addr->u.sa.netmask);
    XFAIL_UNLESS(addr->u.sa.port[0] == 3, "wrong port[0], got:%d expected:3\n", addr->u.sa.port[0]);
    XFAIL_UNLESS(addr->u.sa.port[1] == 5, "wrong port[1], got:%d expected:5\n", addr->u.sa.port[1]);
    XFAIL_IF(NULL == inet_ntop(AF_INET, &addr->u.sa.sin_addr, ip, sizeof(ip)),
            "inet_ntop failed: %s\n", g_strerror(errno));
    XFAIL_UNLESS(0 == strncmp(ip, "127.0.0.1", 10), "wrong ip got:`%s' expected:127.0.0.1\n", ip);
    g_free(addr);
}

#if HAVE_IPV6
static void test7(void)
{
    char ip[100] = { 0 };
    struct sydbox_addr *addr;

    addr = address_from_string("inet6://2001:db8:ac10:fe01::@3-5", false);
    XFAIL_IF(addr == NULL, "returned NULL for valid inet6 address\n");
    XFAIL_UNLESS(addr->family == AF_INET6, "wrong family, got:%d expected:%d\n", addr->family, AF_INET6);
    XFAIL_UNLESS(addr->u.sa6.netmask == 64, "wrong netmask, got:%d expected:64\n", addr->u.sa6.netmask);
    XFAIL_UNLESS(addr->u.sa6.port[0] == 3, "wrong port[0], got:%d expected:3\n", addr->u.sa6.port[0]);
    XFAIL_UNLESS(addr->u.sa6.port[1] == 5, "wrong port[1], got:%d expected:5\n", addr->u.sa6.port[1]);
    XFAIL_IF(NULL == inet_ntop(AF_INET6, &addr->u.sa6.sin6_addr, ip, sizeof(ip)),
            "inet_ntop failed: %s\n", g_strerror(errno));
    XFAIL_UNLESS(0 == strncasecmp(ip, "2001:db8:ac10:fe01::", 21),
            "wrong ip got:`%s' expected:`2001:db8:ac10:fe01::'\n", ip);
    g_free(addr);
}
#endif /* HAVE_IPV6 */

static void test8(void)
{
    char ip[100] = { 0 };
    struct sydbox_addr *addr;

    addr = address_from_string("inet://192.168.0.0/16@3", false);
    XFAIL_IF(addr == NULL, "returned NULL for valid inet address\n");
    XFAIL_UNLESS(addr->family == AF_INET, "wrong family, got:%d expected:%d\n", addr->family, AF_INET);
    XFAIL_UNLESS(addr->u.sa.netmask == 16, "wrong netmask, got:%d expected:16\n", addr->u.sa.netmask);
    XFAIL_UNLESS(addr->u.sa.port[0] == 3, "wrong port[0], got:%d expected:3\n", addr->u.sa.port[0]);
    XFAIL_UNLESS(addr->u.sa.port[1] == 3, "wrong port[1], got:%d expected:3\n", addr->u.sa.port[1]);
    XFAIL_IF(NULL == inet_ntop(AF_INET, &addr->u.sa.sin_addr, ip, sizeof(ip)),
            "inet_ntop failed: %s\n", g_strerror(errno));
    XFAIL_UNLESS(0 == strncmp(ip, "192.168.0.0", 10), "wrong ip got:`%s' expected:192.168.0.0\n", ip);
    g_free(addr);
}

#if HAVE_IPV6
static void test9(void)
{
    char ip[100] = { 0 };
    struct sydbox_addr *addr;

    addr = address_from_string("inet6://::1/64@3", false);
    XFAIL_IF(addr == NULL, "returned NULL for valid inet6 address\n");
    XFAIL_UNLESS(addr->family == AF_INET6, "wrong family, got:%d expected:%d\n", addr->family, AF_INET6);
    XFAIL_UNLESS(addr->u.sa6.netmask == 64, "wrong netmask, got:%d expected:64\n", addr->u.sa6.netmask);
    XFAIL_UNLESS(addr->u.sa6.port[0] == 3, "wrong port[0], got:%d expected:3\n", addr->u.sa6.port[0]);
    XFAIL_UNLESS(addr->u.sa6.port[1] == 3, "wrong port[1], got:%d expected:3\n", addr->u.sa6.port[1]);
    XFAIL_IF(NULL == inet_ntop(AF_INET6, &addr->u.sa6.sin6_addr, ip, sizeof(ip)),
            "inet_ntop failed: %s\n", g_strerror(errno));
    XFAIL_UNLESS(0 == strncasecmp(ip, "::1", 4), "wrong ip got:`%s' expected:`::1'\n", ip);
    g_free(addr);
}
#endif /* HAVE_IPV6 */

static void test10(void)
{
    char ip[100] = { 0 };
    struct sydbox_addr *addr;

    addr = address_from_string("inet://127.0.0.0/8@8990-9000", false);
    XFAIL_IF(addr == NULL, "returned NULL for valid inet address\n");
    XFAIL_UNLESS(addr->family == AF_INET, "wrong family, got:%d expected:%d\n", addr->family, AF_INET);
    XFAIL_UNLESS(addr->u.sa.netmask == 8, "wrong netmask, got:%d expected:8\n", addr->u.sa.netmask);
    XFAIL_UNLESS(addr->u.sa.port[0] == 8990, "wrong port[0], got:%d expected:8990\n", addr->u.sa.port[0]);
    XFAIL_UNLESS(addr->u.sa.port[1] == 9000, "wrong port[1], got:%d expected:9000\n", addr->u.sa.port[1]);
    XFAIL_IF(NULL == inet_ntop(AF_INET, &addr->u.sa.sin_addr, ip, sizeof(ip)),
            "inet_ntop failed: %s\n", g_strerror(errno));
    XFAIL_UNLESS(0 == strncmp(ip, "127.0.0.0", 10), "wrong ip got:`%s' expected:127.0.0.0\n", ip);
    g_free(addr);
}

#if HAVE_IPV6
static void test11(void)
{
    char ip[100] = { 0 };
    struct sydbox_addr *addr;

    addr = address_from_string("inet6://::1/64@3-5", false);
    XFAIL_IF(addr == NULL, "returned NULL for valid inet6 address\n");
    XFAIL_UNLESS(addr->family == AF_INET6, "wrong family, got:%d expected:%d\n", addr->family, AF_INET6);
    XFAIL_UNLESS(addr->u.sa6.netmask == 64, "wrong netmask, got:%d expected:64\n", addr->u.sa6.netmask);
    XFAIL_UNLESS(addr->u.sa6.port[0] == 3, "wrong port[0], got:%d expected:3\n", addr->u.sa6.port[0]);
    XFAIL_UNLESS(addr->u.sa6.port[1] == 5, "wrong port[1], got:%d expected:5\n", addr->u.sa6.port[1]);
    XFAIL_IF(NULL == inet_ntop(AF_INET6, &addr->u.sa6.sin6_addr, ip, sizeof(ip)),
            "inet_ntop failed: %s\n", g_strerror(errno));
    XFAIL_UNLESS(0 == strncasecmp(ip, "::1", 4), "wrong ip got:`%s' expected:`::1'\n", ip);
    g_free(addr);
}
#endif /* HAVE_IPV6 */

void test12(void)
{
    struct sydbox_addr *haystack, *needle;

    haystack = address_from_string("inet://192.168.0.0/16@0", false);
    XFAIL_IF(haystack == NULL, "returned NULL for valid inet address\n");

    for (unsigned int i = 0; i < 255; i++) {
        for (unsigned int j = 0; j < 255; j++) {
            char *addr = g_strdup_printf("inet://192.168.%u.%u@0", i, j);
            needle = address_from_string(addr, false);
            XFAIL_IF(needle == NULL, "returned NULL for valid inet address `%s'\n", addr);
            XFAIL_UNLESS(address_has(haystack, needle), "needle `%s' not in haystack!\n", addr);
            g_free(addr);
            g_free(needle);
        }
    }

    for (unsigned int i = 0; i < 255; i++) {
        for (unsigned int j = 0; j < 255; j++) {
            if (i == 192 && j == 168)
                continue;
            char *addr = g_strdup_printf("inet://%u.%u.0.0@0", i, j);
            needle = address_from_string(addr, false);
            XFAIL_IF(needle == NULL, "returned NULL for valid inet address `%s'\n", addr);
            XFAIL_IF(address_has(haystack, needle), "needle `%s' in haystack!\n", addr);
            g_free(addr);
            g_free(needle);
        }
    }
    g_free(haystack);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/net/address_from_string/invalid", test1);
    g_test_add_func("/net/address_from_string/unix", test2);
    g_test_add_func("/net/address_from_string/unix-abstract", test3);
    g_test_add_func("/net/address_from_string/inet", test4);
#if HAVE_IPV6
    g_test_add_func("/net/address_from_string/inet6", test5);
#endif /* HAVE_IPV6 */
    g_test_add_func("/net/address_from_string/inet/range", test6);
#if HAVE_IPV6
    g_test_add_func("/net/address_from_string/inet6/range", test7);
#endif /* HAVE_IPV6 */
    g_test_add_func("/net/address_from_string/inet/netmask", test8);
#if HAVE_IPV6
    g_test_add_func("/net/address_from_string/inet6/netmask", test9);
#endif /* HAVE_IPV6 */
    g_test_add_func("/net/address_from_string/inet/netmask/range", test10);
#if HAVE_IPV6
    g_test_add_func("/net/address_from_string/inet6/netmask/range", test11);
#endif /* HAVE_IPV6 */

    g_test_add_func("/net/has_address/inet", test12);
    /* TODO: g_test_add_func("/net/has_address/inet6", test13); */

    return g_test_run();
}

