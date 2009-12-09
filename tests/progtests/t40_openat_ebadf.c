/* Check program for t40-openat-ebadf.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Elias Pipping <elias@pipping.org>
 * Distributed under the terms of the GNU General Public License v2
 */

#define _ATFILE_SOURCE
#include <errno.h>
#include <fcntl.h>

int main(void)
{
    openat(-1, ".", O_RDONLY);
    return !(errno == EBADF);
}
