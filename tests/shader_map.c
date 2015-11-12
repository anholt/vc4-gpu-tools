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

#include <sys/mman.h>
#include "vc4_test.h"

static int
get_shader_bo(int fd)
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
        }

        return create.handle;
}

static int
get_mmap_offset(int fd, int handle)
{
        struct drm_vc4_mmap_bo map = {
                .handle = handle,
        };

        int ret = ioctl(fd, DRM_IOCTL_VC4_MMAP_BO, &map);
        if (ret != 0) {
                fprintf(stderr, "Map unexpectedly returned %d\n", errno);
                vc4_report_result(VC4_RESULT_FAIL);
        }

        return map.offset;
}

static void *
do_map(int fd, uint64_t offset, int prot)
{
        return mmap(NULL, 3 * sizeof(uint64_t), prot, MAP_SHARED, fd, offset);
}

SINGLE_TEST_WITH_DRM()
{
        int handle = get_shader_bo(fd);
        uint64_t offset = get_mmap_offset(fd, handle);
        void *map;
        bool pass = true;

        printf("Testing a mapping with PROT_READ | PROT_WRITE\n");
        map = do_map(fd, offset, PROT_READ | PROT_WRITE);
        if (map != MAP_FAILED) {
                fprintf(stderr, "mmap returned %p, expected MAP_FAILELD\n",
                        map);
                pass = false;
        }

        printf("Testing a mapping with PROT_READ\n");
        map = do_map(fd, offset, PROT_READ);
        if (map == MAP_FAILED) {
                fprintf(stderr, "mmap returned MAP_FAILELD\n");
                pass = false;
        }

        vc4_report_result(pass ? VC4_RESULT_PASS : VC4_RESULT_FAIL);
}
