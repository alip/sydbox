/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Saleem Abdulrasool <compnerd@compnerd.org>
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
#endif // HAVE_CONFIG_H

#include <stdbool.h>
#include <string.h>
#include <fnmatch.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "syd-children.h"
#include "syd-config.h"
#include "syd-log.h"
#include "syd-utils.h"

void sydbox_access_violation(struct tchild *child, const gchar *path, const gchar *fmt, ...)
{
    va_list args;
    time_t now = time(NULL);

    if (NULL != path) {
        GSList *walk = sydbox_config_get_filters();
        while (NULL != walk) {
            gchar *pattern = (gchar *)walk->data;
            if (0 == fnmatch(pattern, path, FNM_PATHNAME)) {
                g_debug("pattern `%s' matches path `%s', ignoring the access violation", pattern, path);
                return;
            }
            else
                g_debug("pattern `%s' doesn't match path `%s'", pattern, path);
            walk = g_slist_next(walk);
        }
    }

    g_fprintf(stderr, PACKAGE "@%lu: %sAccess Violation!%s\n", now,
            sydbox_config_get_colourise_output() ? ANSI_MAGENTA : "",
            sydbox_config_get_colourise_output() ? ANSI_NORMAL : "");
    g_fprintf(stderr, PACKAGE "@%lu: %sChild Process ID: %s%i%s\n", now,
            sydbox_config_get_colourise_output() ? ANSI_MAGENTA : "",
            sydbox_config_get_colourise_output() ? ANSI_DARK_MAGENTA : "",
            child->pid,
            sydbox_config_get_colourise_output() ? ANSI_NORMAL : "");
    g_fprintf(stderr, PACKAGE "@%lu: %sChild CWD: %s%s%s\n", now,
            sydbox_config_get_colourise_output() ? ANSI_MAGENTA : "",
            sydbox_config_get_colourise_output() ? ANSI_DARK_MAGENTA : "",
            child->cwd,
            sydbox_config_get_colourise_output() ? ANSI_NORMAL : "");
    g_fprintf(stderr, PACKAGE "@%lu: %sLast Exec: %s%s%s\n", now,
            sydbox_config_get_colourise_output() ? ANSI_MAGENTA : "",
            sydbox_config_get_colourise_output() ? ANSI_DARK_MAGENTA : "",
            child->lastexec->str,
            sydbox_config_get_colourise_output() ? ANSI_NORMAL : "");
    g_fprintf(stderr, PACKAGE "@%lu: %sReason: %s", now,
            sydbox_config_get_colourise_output() ? ANSI_MAGENTA : "",
            sydbox_config_get_colourise_output() ? ANSI_DARK_MAGENTA : "");

    va_start(args, fmt);
    g_vfprintf(stderr, fmt, args);
    va_end(args);

    g_fprintf(stderr, "%s\n", sydbox_config_get_colourise_output() ? ANSI_NORMAL : "");
}

gchar *sydbox_compress_path(const gchar * const path)
{
    bool skip_slashes = false;
    GString *compressed;
    guint i;

    compressed = g_string_sized_new(strlen(path));

    for (i = 0; i < strlen(path); i++) {
        if (path[i] == '/' && skip_slashes)
            continue;
        skip_slashes = (path[i] == '/');

        g_string_append_c(compressed, path[i]);
    }

    /* truncate trailing slashes on paths other than '/' */
    if (compressed->len > 1 && compressed->str[compressed->len - 1] == '/')
        g_string_truncate(compressed, compressed->len - 1);

    return g_string_free(compressed, FALSE);
}

