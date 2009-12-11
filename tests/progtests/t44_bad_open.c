/* Check program for t44-bad-open.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <alip@exherbo.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(void)
{
    int ret;
    ret = open(NULL, O_RDONLY);
    return (EFAULT == errno) ? EXIT_SUCCESS : EXIT_FAILURE;
}
