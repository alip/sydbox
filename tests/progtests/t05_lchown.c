/* Check program for t05-lchown.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(void) {
    uid_t uid = geteuid();
    gid_t gid = getegid();
    if (0 > lchown("its.not.the.same", uid, gid))
        return (EPERM == errno) ? EXIT_FAILURE : EXIT_SUCCESS;
    else
        return EXIT_SUCCESS;
}
