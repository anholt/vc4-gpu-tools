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

static bool test_create(int fd, uint64_t *prog, uint32_t size)
{
        struct drm_vc4_create_shader_bo create = {
                .size = size,
                .data = (uintptr_t)(void *)prog,
        };

        int ret = ioctl(fd, DRM_IOCTL_VC4_CREATE_SHADER_BO, &create);
        if (ret != -1 || errno != EINVAL) {
                fprintf(stderr, "Unexpected non-EINVAL (%d/%d)\n", ret, errno);
                return false;
        } else {
                printf("Got EINVAL\n");
                return true;
        }
}

SINGLE_TEST_WITH_DRM()
{
        /* Use a page-sized program to try to trigger overflow. */
        uint64_t prog[4096 / sizeof(uint64_t)];
        uint32_t last = ARRAY_SIZE(prog) - 1;
        bool pass = true;

        for (int i = 0; i < ARRAY_SIZE(prog); i++)
                prog[i] = qpu_NOP();

        printf("Testing with no PROG_END at all\n");
        pass = test_create(fd, prog, sizeof(prog)) && pass;

        printf("Testing with PROG_END at last instruction\n");
        prog[last] = qpu_set_sig(qpu_NOP(), QPU_SIG_PROG_END);
        pass = test_create(fd, prog, sizeof(prog)) && pass;
        prog[last] = qpu_NOP();

        printf("Testing with PROG_END at second-to-last instruction\n");
        prog[last - 1] = qpu_set_sig(qpu_NOP(), QPU_SIG_PROG_END);
        pass = test_create(fd, prog, sizeof(prog)) && pass;
        prog[last - 1] = qpu_NOP();

        vc4_report_result(pass ? VC4_RESULT_PASS : VC4_RESULT_FAIL);
}
