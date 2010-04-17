/* vim: set et ts=4 sts=4 sw=4 fdm=syntax : */

/*
 * Copyright (c) 2009 Saleem Abdulrasool <compnerd@compnerd.org>
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

#include <stdlib.h>
#include <glib.h>

#include "syd-children.h"
#include "syd-config.h"
#include "syd-path.h"

static void test1(void)
{
    GHashTable *children = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, tchild_free_one);
    struct tchild *child;

    tchild_new(children, 666, true);

    child = tchild_find(children, 666);
    g_assert(child);
    g_assert_cmpint(child->pid, ==, 666);
    g_assert(child->flags & TCHILD_NEEDSETUP);
    g_assert(!(child->flags & TCHILD_INSYSCALL));
    g_assert_cmpint(child->sno, ==, 0xbadca11);
    g_assert_cmpint(child->retval, ==, -1);

    g_hash_table_destroy(children);
}

static void test2(void)
{
    GHashTable *children = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, tchild_free_one);

    tchild_new(children, 666, true);
    tchild_new(children, 667, false);
    tchild_new(children, 668, false);

    tchild_delete(children, 666);

    g_assert(NULL == tchild_find(children, 666));

    g_hash_table_destroy(children);
}

static void test3(void)
{
    GHashTable *children = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, tchild_free_one);
    struct tchild *child, *parent;

    tchild_new(children, 666, true);
    tchild_new(children, 667, false);

    parent = tchild_find(children, 666);
    child = tchild_find(children, 667);

    g_assert(parent);
    g_assert(child);

    /* Fill the information of the parent */
    parent->cwd = g_strdup("/var/empty");
    parent->lastexec = g_string_assign(parent->lastexec, "./jonathan_livingston");
    parent->personality = 3;
    parent->sandbox->path = false;
    parent->sandbox->exec = true;
    parent->sandbox->network = true;
    parent->sandbox->lock = LOCK_SET;
    pathnode_new(&(parent->sandbox->write_prefixes), "/dev", false);
    pathnode_new(&(parent->sandbox->exec_prefixes), "/tmp", false);

    /* Inherit information to the child */
    tchild_inherit(child, parent);

    g_assert(!(child->flags & TCHILD_NEEDINHERIT));
    g_assert_cmpstr(child->cwd, ==, "/var/empty");
    g_assert_cmpstr(child->lastexec->str, ==, "./jonathan_livingston");
    g_assert_cmpint(parent->personality, ==, 3);
    g_assert(!child->sandbox->path);
    g_assert(child->sandbox->exec);
    g_assert(child->sandbox->network);
    g_assert_cmpint(child->sandbox->lock, ==, LOCK_SET);
    g_assert_cmpstr(child->sandbox->write_prefixes->data, ==, "/dev");
    g_assert_cmpstr(child->sandbox->exec_prefixes->data, ==, "/tmp");

    g_hash_table_destroy(children);
}

static void no_log(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
}

int main(int argc, char **argv)
{
    g_setenv(ENV_NO_CONFIG, "1", 1);
    sydbox_config_load(NULL, NULL);

    g_test_init(&argc, &argv, NULL);

    g_log_set_default_handler(no_log, NULL);

    g_test_add_func("/children/new", test1);
    g_test_add_func("/children/delete", test2);
    g_test_add_func("/children/inherit", test3);

    return g_test_run();
}

