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

SINGLE_TEST_WITH_DRM()
{
        uint64_t prog[3];

        prog[0] = qpu_set_sig(qpu_NOP(), QPU_SIG_PROG_END);
        prog[1] = qpu_NOP();
        prog[2] = qpu_NOP();

        struct drm_vc4_create_shader_bo create = {
                .size = sizeof(prog),
                .data = (uintptr_t)(void *)prog,
        };

        int ret = ioctl(fd, DRM_IOCTL_VC4_CREATE_SHADER_BO, &create);
        if (ret != 0) {
                fprintf(stderr, "Create unexpectedly returned %d\n", errno);
                vc4_report_result(VC4_RESULT_FAIL);
        } else {
                printf("Successfully created shader %d\n", create.handle);
                vc4_report_result(VC4_RESULT_PASS);
        }
}
