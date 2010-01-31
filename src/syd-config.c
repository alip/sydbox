/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009, 2010 Ali Polatel <alip@exherbo.org>
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
#endif /* HAVE_CONFIG_H */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "syd-config.h"
#include "syd-log.h"
#include "syd-net.h"
#include "syd-path.h"

struct sydbox_config
{
    gchar *logfile;

    gint verbosity;

    bool sandbox_path;
    bool sandbox_exec;
    bool sandbox_network;

    bool network_auto_whitelist_bind;

    bool colourise_output;
    bool disallow_magic_commands;
    bool wait_all;
    bool allow_proc_pid;
    bool wrap_lstat;

    GSList *filters;
    GSList *exec_filters;
    GSList *network_filters;
    GSList *write_prefixes;
    GSList *exec_prefixes;
    GSList *network_whitelist_bind;
    GSList *network_whitelist_connect;
} *config;


static void sydbox_config_set_defaults(void)
{
    g_assert(config != NULL);

    config->colourise_output = true;
    config->verbosity = 1;
    config->sandbox_path = true;
    config->sandbox_exec = false;
    config->sandbox_network = false;
    config->network_auto_whitelist_bind = false;
    config->disallow_magic_commands = false;
    config->wait_all = true;
    config->allow_proc_pid = true;
    config->wrap_lstat = true;
    config->filters = NULL;
    config->exec_filters = NULL;
    config->network_filters = NULL;
    config->write_prefixes = NULL;
    config->exec_prefixes = NULL;
    config->network_whitelist_bind = NULL;
    config->network_whitelist_connect = NULL;
}

bool sydbox_config_load(const gchar * const file, const gchar * const profile)
{
    gchar *config_file;
    GKeyFile *config_fd;
    GError *config_error = NULL;

    g_return_val_if_fail(!config, true);

    // Initialize config structure
    config = g_new0(struct sydbox_config, 1);

    if (g_getenv(ENV_NO_CONFIG)) {
        /* ENV_NO_CONFIG set, set the defaults,
         * and return without parsing the configuration file.
         */
        sydbox_config_set_defaults();
        return true;
    }

    // Figure out the path to the configuration file
    if (file)
        config_file = g_strdup(file);
    else if (profile)
        config_file = g_strdup_printf(DATADIR G_DIR_SEPARATOR_S "sydbox" G_DIR_SEPARATOR_S "%s.conf", profile);
    else if (g_getenv(ENV_CONFIG))
        config_file = g_strdup(g_getenv(ENV_CONFIG));
    else
        config_file = g_strdup(SYSCONFDIR G_DIR_SEPARATOR_S "sydbox.conf");

    // Initialize key file
    config_fd = g_key_file_new();
    if (!g_key_file_load_from_file(config_fd, config_file, G_KEY_FILE_NONE, &config_error)) {
        switch (config_error->code) {
            case G_FILE_ERROR_NOENT:
                /* Configuration file not found!
                 * Set the defaults and return true.
                 */
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config_file);
                sydbox_config_set_defaults();
                return true;
            default:
                g_printerr("failed to parse config file: %s\n", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
        }
    }

    // Get main.colour
    config->colourise_output = g_key_file_get_boolean(config_fd, "main", "colour", &config_error);
    if (!config->colourise_output) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("main.colour not a boolean: %s\n", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->colourise_output = true;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get main.lock
    config->disallow_magic_commands = g_key_file_get_boolean(config_fd, "main", "lock", &config_error);
    if (!config->disallow_magic_commands && config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("main.lock not a boolean: %s\n", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->disallow_magic_commands = false;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get main.wait_all
    config->wait_all = g_key_file_get_boolean(config_fd, "main", "wait_all", &config_error);
    if (!config->wait_all && config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("main.wait_all not a boolean: %s\n", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->wait_all = true;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get main.allow_proc_pid
    config->allow_proc_pid = g_key_file_get_boolean(config_fd, "main", "allow_proc_pid", &config_error);
    if (!config->allow_proc_pid && config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("main.allow_proc_pid not a boolean: %s\n", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->allow_proc_pid = true;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    config->wrap_lstat = g_key_file_get_boolean(config_fd, "main", "wrap_lstat", &config_error);
    if (!config->wrap_lstat && config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("main.wrap_lstat not a boolean: %s\n", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->wrap_lstat = true;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get filter.path
    char **filterlist;
    filterlist = g_key_file_get_string_list(config_fd, "filter", "path", NULL, NULL);
    if (NULL != filterlist) {
        for (unsigned int i = 0; NULL != filterlist[i]; i++)
            sydbox_config_addfilter(filterlist[i]);
        g_strfreev(filterlist);
    }

    // Get filter.exec
    filterlist = g_key_file_get_string_list(config_fd, "filter", "exec", NULL, NULL);
    if (NULL != filterlist) {
        for (unsigned int i = 0; NULL != filterlist[i]; i++)
            sydbox_config_addfilter_exec(filterlist[i]);
        g_strfreev(filterlist);
    }

    // Get filter.network
    filterlist = g_key_file_get_string_list(config_fd, "filter", "network", NULL, NULL);
    if (NULL != filterlist) {
        for (unsigned int i = 0; NULL != filterlist[i]; i++) {
            struct sydbox_addr *addr;
            addr = address_from_string(filterlist[i], false);
            if (NULL == addr) {
                g_printerr("error: malformed address `%s' at position %d of filter.network\n",
                        filterlist[i], i);
                g_strfreev(filterlist);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
            }
            sydbox_config_addfilter_net(addr);
            g_free(addr);
        }
        g_strfreev(filterlist);
    }

    // Get log.file
    config->logfile = g_key_file_get_string(config_fd, "log", "file", NULL);

    // Get log.level
    config->verbosity = g_key_file_get_integer(config_fd, "log", "level", &config_error);
    if (config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("log.level not an integer: %s\n", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->verbosity = 1;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get sandbox.path
    config->sandbox_path = g_key_file_get_boolean(config_fd, "sandbox", "path", &config_error);
    if (config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("sandbox.path not a boolean: %s\n", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->sandbox_path = true;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get sandbox.exec
    config->sandbox_exec = g_key_file_get_boolean(config_fd, "sandbox", "exec", &config_error);
    if (config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("sandbox.exec not a boolean: %s\n", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->sandbox_exec = false;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get sandbox.network
    config->sandbox_network = g_key_file_get_boolean(config_fd, "sandbox", "network", &config_error);
    if (config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("sandbox.network not a boolean: %s\n", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->sandbox_network = false;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get prefix.write
    char **write_prefixes = g_key_file_get_string_list(config_fd, "prefix", "write", NULL, NULL);
    if (NULL != write_prefixes) {
        for (unsigned int i = 0; NULL != write_prefixes[i]; i++)
            pathnode_new_early(&config->write_prefixes, write_prefixes[i], 1);
        g_strfreev(write_prefixes);
    }

    // Get prefix.exec
    char **exec_prefixes = g_key_file_get_string_list(config_fd, "prefix", "exec", NULL, NULL);
    if (NULL != exec_prefixes) {
        for (unsigned int i = 0; NULL != exec_prefixes[i]; i++)
            pathnode_new_early(&config->exec_prefixes, exec_prefixes[i], 1);
        g_strfreev(exec_prefixes);
    }

    // Get net.auto_whitelist_bind
    config->network_auto_whitelist_bind = g_key_file_get_boolean(config_fd,
            "net", "auto_whitelist_bind", &config_error);
    if (config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("net.auto_whitelist_bind not a boolean: %s\n", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->network_auto_whitelist_bind = false;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get net.whitelist_bind
    char **netwhitelist_bind = g_key_file_get_string_list(config_fd, "net", "whitelist_bind", NULL, NULL);
    if (NULL != netwhitelist_bind) {
        for (unsigned int i = 0; NULL != netwhitelist_bind[i]; i++) {
            struct sydbox_addr *addr;
            addr = address_from_string(netwhitelist_bind[i], false);
            if (NULL == addr) {
                g_printerr("error: malformed address `%s' at position %d of net.whitelist_bind\n",
                        netwhitelist_bind[i], i);
                g_strfreev(netwhitelist_bind);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
            }
            config->network_whitelist_bind = g_slist_prepend(config->network_whitelist_bind, addr);
        }
        g_strfreev(netwhitelist_bind);
    }

    // Get net.whitelist_bind
    char **netwhitelist_connect = g_key_file_get_string_list(config_fd, "net", "whitelist_connect", NULL, NULL);
    if (NULL != netwhitelist_connect) {
        for (unsigned int i = 0; NULL != netwhitelist_connect[i]; i++) {
            struct sydbox_addr *addr;
            addr = address_from_string(netwhitelist_connect[i], false);
            if (NULL == addr) {
                g_printerr("error: malformed address `%s' at position %d of net.whitelist_connect\n",
                        netwhitelist_connect[i], i);
                g_strfreev(netwhitelist_connect);
                g_key_file_free(config_fd);
                g_free(config_file);
                g_free(config);
                return false;
            }
            config->network_whitelist_connect = g_slist_prepend(config->network_whitelist_connect, addr);
        }
        g_strfreev(netwhitelist_connect);
    }

    // Cleanup and return
    g_key_file_free(config_fd);
    g_free(config_file);
    return true;
}

void sydbox_config_update_from_environment(void)
{
    g_info("extending path list using environment variable " ENV_WRITE);
    pathlist_init(&config->write_prefixes, g_getenv(ENV_WRITE));

    g_info("extending path list using environment variable " ENV_EXEC_ALLOW);
    pathlist_init(&config->exec_prefixes, g_getenv(ENV_EXEC_ALLOW));

    g_info("extending network whitelist using environment variable " ENV_NET_WHITELIST_BIND);
    if (g_getenv(ENV_NET_WHITELIST_BIND)) {
        char **netwhitelist_bind = g_strsplit(g_getenv(ENV_NET_WHITELIST_BIND), ";", 0);
        for (unsigned int i = 0; NULL != netwhitelist_bind[i]; i++) {
            struct sydbox_addr *addr;
            addr = address_from_string(netwhitelist_bind[i], true);
            if (NULL == addr) {
                g_critical("error: malformed address `%s' at position %d of "ENV_NET_WHITELIST_BIND"\n",
                        netwhitelist_bind[i], i);
                g_printerr("error: malformed address `%s' at position %d of "ENV_NET_WHITELIST_BIND"\n",
                        netwhitelist_bind[i], i);
                g_strfreev(netwhitelist_bind);
                exit(-1);
            }
            config->network_whitelist_bind = g_slist_prepend(config->network_whitelist_bind, addr);
        }
        g_strfreev(netwhitelist_bind);
    }

    g_info("extending network whitelist using environment variable " ENV_NET_WHITELIST_CONNECT);
    if (g_getenv(ENV_NET_WHITELIST_CONNECT)) {
        char **netwhitelist_connect = g_strsplit(g_getenv(ENV_NET_WHITELIST_CONNECT), ";", 0);
        for (unsigned int i = 0; NULL != netwhitelist_connect[i]; i++) {
            struct sydbox_addr *addr;
            addr = address_from_string(netwhitelist_connect[i], true);
            if (NULL == addr) {
                g_critical("error: malformed address `%s' at position %d of "ENV_NET_WHITELIST_CONNECT"\n",
                        netwhitelist_connect[i], i);
                g_printerr("error: malformed address `%s' at position %d of "ENV_NET_WHITELIST_CONNECT"\n",
                        netwhitelist_connect[i], i);
                g_strfreev(netwhitelist_connect);
                exit(-1);
            }
            config->network_whitelist_connect = g_slist_prepend(config->network_whitelist_connect, addr);
        }
        g_strfreev(netwhitelist_connect);
    }
}

static inline void print_slist_entry(gpointer data, G_GNUC_UNUSED gpointer userdata)
{
    gchar *cdata = (gchar *) data;
    if (NULL != cdata && '\0' != cdata[0])
        g_fprintf(stderr, "\t%s\n", cdata);
}

static inline void print_netlist_entry(gpointer data, G_GNUC_UNUSED gpointer userdata)
{
    char ip[100] = { 0 };
    struct sydbox_addr *addr = (struct sydbox_addr *) data;

    if (NULL == addr)
        return;

    switch (addr->family) {
        case AF_UNIX:
            g_fprintf(stderr, "\t{family=AF_UNIX path=%s abstract=%s}\n",
                    addr->u.saun.sun_path,
                    addr->u.saun.abstract ? "true" : "false");
            break;
        case AF_INET:
            inet_ntop(AF_INET, &addr->u.sa.sin_addr, ip, sizeof(ip));
            g_fprintf(stderr, "\t{family=AF_INET addr=%s netmask=%d port_range=%d-%d}\n",
                    ip, addr->u.sa.netmask, addr->u.sa.port[0], addr->u.sa.port[1]);
            break;
#if HAVE_IPV6
        case AF_INET6:
            inet_ntop(AF_INET6, &addr->u.sa6.sin6_addr, ip, sizeof(ip));
            g_fprintf(stderr, "\t{family=AF_INET6 addr=%s netmask=%d port_range=%d-%d}\n",
                    ip, addr->u.sa6.netmask, addr->u.sa6.port[0], addr->u.sa6.port[1]);
            break;
#endif /* HAVE_IPV6 */
        default:
            g_assert_not_reached();
    }
}

void sydbox_config_write_to_stderr (void)
{
    g_fprintf(stderr, "main.colour = %s\n", config->colourise_output ? "on" : "off");
    g_fprintf(stderr, "main.lock = %s\n", config->disallow_magic_commands ? "set" : "unset");
    g_fprintf(stderr, "main.wait_all = %s\n", config->wait_all ? "yes" : "no");
    g_fprintf(stderr, "main.allow_proc_pid = %s\n", config->allow_proc_pid ? "yes" : "no");
    g_fprintf(stderr, "main.wrap_lstat = %s\n", config->wrap_lstat ? "yes" : "no");
    g_fprintf(stderr, "filter.path:\n");
    g_slist_foreach(config->filters, print_slist_entry, NULL);
    g_fprintf(stderr, "filter.exec:\n");
    g_slist_foreach(config->exec_filters, print_slist_entry, NULL);
    g_fprintf(stderr, "filter.network:\n");
    g_slist_foreach(config->network_filters, print_netlist_entry, NULL);
    g_fprintf(stderr, "log.file = %s\n", config->logfile ? config->logfile : "stderr");
    g_fprintf(stderr, "log.level = %d\n", config->verbosity);
    g_fprintf(stderr, "sandbox.path = %s\n", config->sandbox_path ? "yes" : "no");
    g_fprintf(stderr, "sandbox.exec = %s\n", config->sandbox_exec ? "yes" : "no");
    g_fprintf(stderr, "sandbox.network = %s\n", config->sandbox_network ? "yes" : "no");
    g_fprintf(stderr, "net.auto_whitelist_bind = %s\n", config->network_auto_whitelist_bind ? "yes" : "no");
    g_fprintf(stderr, "prefix.write:\n");
    g_slist_foreach(config->write_prefixes, print_slist_entry, NULL);
    g_fprintf(stderr, "prefix.exec:\n");
    g_slist_foreach(config->exec_prefixes, print_slist_entry, NULL);
    g_fprintf(stderr, "net.whitelist_bind:\n");
    g_slist_foreach(config->network_whitelist_bind, print_netlist_entry, NULL);
    g_fprintf(stderr, "net.whitelist_connect:\n");
    g_slist_foreach(config->network_whitelist_connect, print_netlist_entry, NULL);
}

const gchar *sydbox_config_get_log_file(void)
{
    return config->logfile;
}

void sydbox_config_set_log_file(const gchar * const logfile)
{
    if (config->logfile)
        g_free(config->logfile);

    config->logfile = g_strdup(logfile);
}

gint sydbox_config_get_verbosity(void)
{
    return config->verbosity;
}

void sydbox_config_set_verbosity(gint verbosity)
{
    config->verbosity = verbosity;
}

bool sydbox_config_get_sandbox_path(void)
{
    return config->sandbox_path;
}

void sydbox_config_set_sandbox_path(bool on)
{
    config->sandbox_path = on;
}

bool sydbox_config_get_sandbox_exec(void)
{
    return config->sandbox_exec;
}

void sydbox_config_set_sandbox_exec(bool on)
{
    config->sandbox_exec = on;
}

bool sydbox_config_get_sandbox_network(void)
{
    return config->sandbox_network;
}

void sydbox_config_set_sandbox_network(bool on)
{
    config->sandbox_network = on;
}

bool sydbox_config_get_network_auto_whitelist_bind(void)
{
    return config->network_auto_whitelist_bind;
}

void sydbox_config_set_network_auto_whitelist_bind(bool on)
{
    config->network_auto_whitelist_bind = on;
}

bool sydbox_config_get_colourise_output(void)
{
    return config->colourise_output;
}

void sydbox_config_set_colourise_output(bool colourise)
{
    config->colourise_output = colourise;
}

bool sydbox_config_get_disallow_magic_commands(void)
{
    return config->disallow_magic_commands;
}

void sydbox_config_set_disallow_magic_commands(bool disallow)
{
    config->disallow_magic_commands = disallow;
}

bool sydbox_config_get_wait_all(void)
{
    return config->wait_all;
}

void sydbox_config_set_wait_all(bool waitall)
{
    config->wait_all = waitall;
}

bool sydbox_config_get_allow_proc_pid(void)
{
    return config->allow_proc_pid;
}

void sydbox_config_set_allow_proc_pid(bool allow)
{
    config->allow_proc_pid = allow;
}

bool sydbox_config_get_wrap_lstat(void)
{
    return config->wrap_lstat;
}

void sydbox_config_set_wrap_lstat(bool wrap)
{
    config->wrap_lstat = wrap;
}

GSList *sydbox_config_get_write_prefixes(void)
{
    return config->write_prefixes;
}

GSList *sydbox_config_get_exec_prefixes(void)
{
    return config->exec_prefixes;
}

GSList *sydbox_config_get_filters(void)
{
    return config->filters;
}

GSList *sydbox_config_get_exec_filters(void)
{
    return config->exec_filters;
}

GSList *sydbox_config_get_network_filters(void)
{
    return config->network_filters;
}

GSList *sydbox_config_get_network_whitelist_bind(void)
{
    return config->network_whitelist_bind;
}

void sydbox_config_set_network_whitelist_bind(GSList *whitelist)
{
    config->network_whitelist_bind = whitelist;
}

GSList *sydbox_config_get_network_whitelist_connect(void)
{
    return config->network_whitelist_connect;
}

void sydbox_config_set_network_whitelist_connect(GSList *whitelist)
{
    config->network_whitelist_connect = whitelist;
}

void sydbox_config_addfilter(const gchar *filter)
{
    config->filters = g_slist_append(config->filters, g_strdup(filter));
}

int sydbox_config_rmfilter(const gchar *filter)
{
    GSList *walk;

    for (walk = config->filters; walk != NULL; walk = g_slist_next(walk)) {
        if (0 == strncmp(walk->data, filter, strlen(filter) + 1)) {
            config->filters = g_slist_remove_link(config->filters, walk);
            g_free(walk->data);
            g_slist_free(walk);
            return 1;
        }
    }
    return 0;
}

void sydbox_config_addfilter_exec(const gchar *filter)
{
    config->exec_filters = g_slist_append(config->exec_filters, g_strdup(filter));
}

int sydbox_config_rmfilter_exec(const gchar *filter)
{
    GSList *walk;

    for (walk = config->exec_filters; walk != NULL; walk = g_slist_next(walk)) {
        if (0 == strncmp(walk->data, filter, strlen(filter) + 1)) {
            config->exec_filters = g_slist_remove_link(config->exec_filters, walk);
            g_free(walk->data);
            g_slist_free(walk);
            return 1;
        }
    }
    return 0;
}

void sydbox_config_addfilter_net(const struct sydbox_addr *filter)
{
    config->network_filters = g_slist_append(config->network_filters, address_dup(filter));
}

int sydbox_config_rmfilter_net(const struct sydbox_addr *filter)
{
    GSList *walk;

    for (walk = config->network_filters; walk != NULL; walk = g_slist_next(walk)) {
        if (address_cmp(walk->data, filter)) {
            config->network_filters = g_slist_remove_link(config->network_filters, walk);
            g_free(walk->data);
            g_slist_free(walk);
            return 1;
        }
    }
    return 0;
}

void sydbox_config_rmfilter_all(void)
{
    g_slist_foreach(config->filters, (GFunc)g_free, NULL);
    g_slist_free(config->filters);
    config->filters = NULL;
    g_slist_foreach(config->exec_filters, (GFunc)g_free, NULL);
    g_slist_free(config->exec_filters);
    config->exec_filters = NULL;
    g_slist_foreach(config->network_filters, (GFunc)g_free, NULL);
    g_slist_free(config->network_filters);
    config->network_filters = NULL;
}

void sydbox_config_rmwhitelist_all(void)
{
    g_slist_foreach(config->network_whitelist_bind, (GFunc)g_free, NULL);
    g_slist_foreach(config->network_whitelist_connect, (GFunc)g_free, NULL);
    g_slist_free(config->network_whitelist_bind);
    g_slist_free(config->network_whitelist_connect);
    config->network_whitelist_bind = NULL;
    config->network_whitelist_connect = NULL;
}

