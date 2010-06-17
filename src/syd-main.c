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
#endif // HAVE_CONFIG_H

#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "syd-children.h"
#include "syd-config.h"
#include "syd-dispatch.h"
#include "syd-log.h"
#include "syd-loop.h"
#include "syd-path.h"
#include "syd-syscall.h"
#include "syd-trace.h"
#include "syd-utils.h"
#include "syd-wrappers.h"

/* pink floyd */
#define PINK_FLOYD  "       ..uu.                               \n" \
                    "       ?$\"\"`?i           z'              \n" \
                    "       `M  .@\"          x\"               \n" \
                    "       'Z :#\"  .   .    f 8M              \n" \
                    "       '&H?`  :$f U8   <  MP   x#'         \n" \
                    "       d#`    XM  $5.  $  M' xM\"          \n" \
                    "     .!\">     @  'f`$L:M  R.@!`           \n" \
                    "    +`  >     R  X  \"NXF  R\"*L           \n" \
                    "        k    'f  M   \"$$ :E  5.           \n" \
                    "        %%    `~  \"    `  'K  'M          \n" \
                    "            .uH          'E   `h           \n" \
                    "         .x*`             X     `          \n" \
                    "      .uf`                *                \n" \
                    "    .@8     .                              \n" \
                    "   'E9F  uf\"          ,     ,             \n" \
                    "     9h+\"   $M    eH. 8b. .8    .....     \n" \
                    "    .8`     $'   M 'E  `R;'   d?\"\"\"`\"# \n" \
                    "   ` E      @    b  d   9R    ?*     @     \n" \
                    "     >      K.zM `%%M'   9'    Xf   .f     \n" \
                    "    ;       R'          9     M  .=`       \n" \
                    "    t                   M     Mx~          \n" \
                    "    @                  lR    z\"           \n" \
                    "    @                  `   ;\"             \n" \
                    "                          `                \n"


static context_t *ctx = NULL;

static gint verbosity = -1;

static gchar *logfile;
static gchar *config_file;
static gchar *config_profile;

static gboolean dump;
static gboolean disable_sandbox_path;
static gboolean sandbox_exec;
static gboolean sandbox_net;
static gboolean network_auto_whitelist_bind;
static gboolean lock;
static gboolean colour;
static gboolean version;
static gboolean nowait;
static gboolean nowrap_lstat;

static GOptionEntry entries[] =
{
    { "version",                'V', 0, G_OPTION_ARG_NONE,                         &version,
        "Show version information",       NULL },
    { "config",                 'c', 0, G_OPTION_ARG_FILENAME,                     &config_file,
        "Path to the configuration file", NULL },
    { "profile",                'p', 0, G_OPTION_ARG_STRING,                       &config_profile,
        "Profile name of the configuration file", NULL },
    { "dump",                   'D', 0, G_OPTION_ARG_NONE,                         &dump,
        "Dump configuration and exit",    NULL },
    { "log-level",              '0', 0, G_OPTION_ARG_INT,                          &verbosity,
        "Logging verbosity",              NULL },
    { "log-file",               'l', 0, G_OPTION_ARG_FILENAME,                     &logfile,
        "Path to the log file",           NULL },
    { "no-colour",              'C', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,     &colour,
        "Disable colouring of messages",  NULL },
    { "lock",                   'L', 0, G_OPTION_ARG_NONE,                         &lock,
        "Disallow magic commands",        NULL },
    { "disable-sandbox-path",   'P', 0, G_OPTION_ARG_NONE,                         &disable_sandbox_path,
        "Disable path sandboxing",        NULL },
    { "sandbox-exec",           'E', 0, G_OPTION_ARG_NONE,                         &sandbox_exec,
        "Enable execve() sandboxing",     NULL },
    { "sandbox-network",        'N', 0, G_OPTION_ARG_NONE,                         &sandbox_net,
        "Enable network sandboxing",      NULL },
    { "network-whitelist-bind", 'B', 0, G_OPTION_ARG_NONE,                         &network_auto_whitelist_bind,
        "Automatically whitelist bind() addresses", NULL},
    { "exit-with-eldest",       'X', 0, G_OPTION_ARG_NONE,                         &nowait,
        "Finish tracing when eldest child exits", NULL},
    { "nowrap-lstat",           'W', 0, G_OPTION_ARG_NONE,                         &nowrap_lstat,
        "Disable wrapping of lstat() calls for too long paths", NULL},
    { NULL, -1, 0, 0, NULL, NULL, NULL },
};

// Cleanup functions
static void cleanup(void)
{
    dispatch_free();
    sydbox_config_rmfilter_all();
    sydbox_config_rmwhitelist_all();
    if (NULL != ctx) {
        if (NULL != ctx->children)
            g_hash_table_foreach(ctx->children, tchild_kill_one, NULL);
        context_free(ctx);
        ctx = NULL;
    }
    sydbox_log_fini();
}

static void sig_cleanup(int signum)
{
    struct sigaction action;
    g_fprintf(stderr, "Caught signal %d, exiting\n", signum);
    cleanup();
    sigaction(signum, NULL, &action);
    action.sa_handler = SIG_DFL;
    sigaction(signum, &action, NULL);
    raise(signum);
}

static gchar *get_username(void)
{
    struct passwd *pwd;
    uid_t uid;

    errno = 0;
    uid = geteuid();
    pwd = getpwuid(uid);

    return (errno) ? NULL : g_strdup(pwd->pw_name);
}

static gchar *get_groupname (void)
{
    struct group *grp;
    gid_t gid;

    errno = 0;
    gid = getegid();
    grp = getgrgid(gid);

    return (errno) ? NULL : g_strdup(grp->gr_name);
}

G_GNUC_NORETURN
static void sydbox_execute_child(G_GNUC_UNUSED int argc, char **argv)
{
    if (!trace_me()) {
        g_printerr("failed to set tracing: %s\n", g_strerror(errno));
        _exit(-1);
    }

    if (strncmp(argv[0], "/bin/sh", 8) == 0)
        g_fprintf(stderr, ANSI_DARK_MAGENTA PINK_FLOYD ANSI_NORMAL);

    execvp(argv[0], argv);

    g_printerr("execvp() failed: %s\n", g_strerror(errno));
    _exit(-1);
}

static int sydbox_execute_parent(int argc, char **argv, pid_t pid)
{
    int status, retval;
    struct sigaction new_action, old_action;
    struct tchild *eldest;

    new_action.sa_handler = sig_cleanup;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;

#define HANDLE_SIGNAL(sig)                          \
    do {                                            \
        sigaction ((sig), NULL, &old_action);       \
        if (old_action.sa_handler != SIG_IGN)       \
            sigaction ((sig), &new_action, NULL);   \
    } while (0)

    HANDLE_SIGNAL(SIGABRT);
    HANDLE_SIGNAL(SIGSEGV);
    HANDLE_SIGNAL(SIGINT);
    HANDLE_SIGNAL(SIGTERM);

#undef HANDLE_SIGNAL

    /* wait for SIGTRAP */
    wait(&status);
    if (WIFEXITED(status)) {
        g_critical("wtf? child died before sending SIGTRAP");
        g_printerr("wtf? child died before sending SIGTRAP\n");
        exit(WEXITSTATUS(status));
    }
    g_assert(WIFSTOPPED(status));
    g_assert(WSTOPSIG(status) == SIGTRAP);

    if (!trace_setup(pid)) {
        g_critical("failed to setup tracing options: %s", g_strerror(errno));
        g_printerr("failed to setup tracing options: %s\n", g_strerror(errno));
        exit(-1);
    }

    ctx->eldest = pid;
    eldest = tchild_new(ctx->children, pid, true);
    eldest->personality = trace_personality(pid);
    if (0 > eldest->personality) {
        g_critical("failed to determine personality of eldest child %i: %s", eldest->pid, g_strerror(errno));
        g_printerr("failed to determine personality of eldest child %i: %s\n", eldest->pid, g_strerror(errno));
        exit(-1);
    }
    g_debug("eldest child %i runs in %s mode", eldest->pid, dispatch_mode(eldest->personality));
    eldest->sandbox->path = sydbox_config_get_sandbox_path();
    eldest->sandbox->exec = sydbox_config_get_sandbox_exec();
    eldest->sandbox->network = sydbox_config_get_sandbox_network();
    eldest->sandbox->lock = sydbox_config_get_disallow_magic_commands() ? LOCK_SET : LOCK_UNSET;

    eldest->sandbox->write_prefixes = sydbox_config_get_write_prefixes();
    if (sydbox_config_get_allow_proc_pid()) {
        gchar *proc_pid = g_strdup_printf("/proc/%i", pid);
        pathnode_new(&(eldest->sandbox->write_prefixes), proc_pid, false);
        g_free(proc_pid);
    }
    eldest->sandbox->exec_prefixes = sydbox_config_get_exec_prefixes();

    eldest->cwd = egetcwd();
    if (NULL == eldest->cwd) {
        g_critical("failed to get current working directory: %s", g_strerror(errno));
        g_printerr("failed to get current working directory: %s\n", g_strerror(errno));
        exit(-1);
    }

    /* Construct the lastexec string for the initial exec */
    int i;
    const char *sep;
    g_string_printf(eldest->lastexec, "execvp(\"%s\", [", argv[0]);
    for (sep = "", i = 0; i < argc; sep = ", ", i++)
        g_string_append_printf(eldest->lastexec, "%s\"%s\"", sep, argv[i]);
    g_string_append(eldest->lastexec, "])");

    g_info ("child %i is ready to go, resuming", pid);
    if (!trace_syscall(pid, 0)) {
        trace_kill(pid);
        g_critical("failed to resume eldest child %i: %s", pid, g_strerror(errno));
        g_printerr("failed to resume eldest child %i: %s\n", pid, g_strerror(errno));
        exit(-1);
    }

    g_info("entering loop");
    retval = trace_loop(ctx);
    g_info("exited loop with return value: %d", retval);

    return retval;
}

static int sydbox_internal_main(int argc, char **argv)
{
    pid_t pid;

    dispatch_init();
    ctx = context_new();

    g_atexit(cleanup);

    /*
     * options are loaded from config file, user config file, updated from the
     * environment, and then overridden by the user passed parameters.
     */
    if (!sydbox_config_load(config_file, config_profile))
        return EXIT_FAILURE;

    if (verbosity >= 0)
        sydbox_config_set_verbosity(verbosity);

    if (logfile)
        sydbox_config_set_log_file(logfile);
    else if (g_getenv(ENV_LOG))
        sydbox_config_set_log_file(g_getenv(ENV_LOG));

    /* initialize logging as early as possible */
    sydbox_log_init();

    sydbox_config_update_from_environment();

    if (colour)
        sydbox_config_set_colourise_output(true);
    else if (g_getenv(ENV_NO_COLOUR))
        sydbox_config_set_colourise_output(false);

    if (disable_sandbox_path)
        sydbox_config_set_sandbox_path(false);
    else if (g_getenv(ENV_DISABLE_PATH))
        sydbox_config_set_sandbox_path(false);

    if (sandbox_exec)
        sydbox_config_set_sandbox_exec(true);
    else if (g_getenv(ENV_EXEC))
        sydbox_config_set_sandbox_exec(true);

    if (sandbox_net)
        sydbox_config_set_sandbox_network(true);
    else if (g_getenv(ENV_NET))
        sydbox_config_set_sandbox_network(true);

    if (network_auto_whitelist_bind)
        sydbox_config_set_network_auto_whitelist_bind(true);
    else if (g_getenv(ENV_NET_AUTO_WHITELIST_BIND))
        sydbox_config_set_network_auto_whitelist_bind(true);

    if (lock)
        sydbox_config_set_disallow_magic_commands(true);
    else if (g_getenv(ENV_LOCK))
        sydbox_config_set_disallow_magic_commands(true);

    if (nowait)
        sydbox_config_set_wait_all(false);
    else if (g_getenv(ENV_NO_WAIT))
        sydbox_config_set_wait_all(false);

    if (nowrap_lstat)
        sydbox_config_set_wrap_lstat(false);
    else if (g_getenv(ENV_NOWRAP_LSTAT))
        sydbox_config_set_wrap_lstat(false);

    if (dump) {
        sydbox_config_write_to_stderr();
        return EXIT_SUCCESS;
    }

    if (sydbox_config_get_verbosity() > 1) {
        gchar *username = NULL, *groupname = NULL;
        GString *command = NULL;

        if (!(username = get_username())) {
            g_printerr("failed to get password file entry: %s\n", g_strerror(errno));
            return EXIT_FAILURE;
        }

        if (!(groupname = get_groupname())) {
            g_printerr("failed to get group file entry: %s\n", g_strerror(errno));
            g_free(username);
            return EXIT_FAILURE;
        }

        command = g_string_new("");
        for (gint i = 0; i < argc; i++)
            g_string_append_printf(command, "%s ", argv[i]);
        g_string_truncate(command, command->len - 1);

        g_info("forking to execute '%s' as %s:%s", command->str, username, groupname);

        g_free(username);
        g_free(groupname);
        g_string_free(command, TRUE);
    }

    /* Set some environment variables so the children can check.
     */
    g_setenv("SYDBOX_ACTIVE", "1", 1);
    g_setenv("SYDBOX_VERSION", VERSION, 1);
    g_setenv("SYDBOX_GITHEAD", GIT_HEAD, 1);

    if ((pid = vfork()) < 0) {
        g_printerr("failed to fork: %s\n", g_strerror(errno));
        return EXIT_FAILURE;
    }

    if (pid == 0)
        sydbox_execute_child(argc, argv);
    else
        return sydbox_execute_parent(argc, argv, pid);
}

int main(int argc, char **argv)
{
    GError *parse_error = NULL;
    GOptionContext *context;

    context = g_option_context_new("-- command [args]");
    g_option_context_add_main_entries(context, entries, PACKAGE);
    g_option_context_set_summary(context, PACKAGE "-" VERSION GIT_HEAD " - ptrace based sandbox");

    if (! g_option_context_parse(context, &argc, &argv, &parse_error)) {
        g_printerr("fatal: option parsing failed: %s\n", parse_error->message);
        g_option_context_free(context);
        g_error_free(parse_error);
        return EXIT_FAILURE;
    }
    g_option_context_free(context);

    if (version) {
        g_printerr(PACKAGE "-" VERSION GIT_HEAD "\n");
        return EXIT_SUCCESS;
    }

    if (!dump) {
        argc--;
        argv++;

        if (argv[0] && strcmp(argv[0], "--") == 0) {
            argc--;
            argv++;
        }

        if (!argv[0]) {
            g_printerr("fatal: no command given\n");
            return EXIT_FAILURE;
        }
    }

    return sydbox_internal_main(argc, argv);
}

