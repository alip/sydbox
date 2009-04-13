/* Sydbox unit tests
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@googlemail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdlib.h>

#include <check.h>

#include "../src/util.h"
#include "check_sydbox.h"

int main(void) {
    int number_failed;

    /* Add suites */
    SRunner *sr = srunner_create(sydbox_utils_suite_create());
    srunner_add_suite(sr, path_suite_create());
    srunner_add_suite(sr, children_suite_create());
    srunner_add_suite(sr, trace_suite_create());

    /* Run and grab the results */
    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);

    /* Cleanup and exit */
    srunner_free(sr);
    return (0 == number_failed) ? EXIT_SUCCESS : EXIT_FAILURE;
}
