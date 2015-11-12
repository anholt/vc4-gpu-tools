/*
 * Copyright © 2015 Broadcom
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "vc4_test.h"

int
main_func_for_single_test(int argc, char **argv, void (*func)(int fd))
{
        if (argc != 1) {
                fprintf(stderr, "usage: %s\n", argv[0]);
                vc4_report_result(VC4_RESULT_FAIL);
        }

        int fd = drmOpen("vc4", NULL);
        if (fd == -1) {
                fprintf(stderr, "Failed to open drm.\n");
                vc4_report_result(VC4_RESULT_SKIP);
        }

        func(fd);

        /* If func() didn't report anything, then this is an error. */
        fprintf(stderr, "%s: Exited without reporting another result\n",
                argv[0]);
        vc4_report_result(VC4_RESULT_FAIL);

        return 1;
}

void
vc4_report_result(enum vc4_result result)
{
        switch (result) {
        case VC4_RESULT_PASS:
                printf("Passed.\n");
                exit(0);
        case VC4_RESULT_FAIL:
                printf("Failed.\n");
                exit(1);
        case VC4_RESULT_SKIP:
                printf("Skipped.\n");
                exit(77);
        }
}
