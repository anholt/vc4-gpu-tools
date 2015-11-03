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

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "xf86drm.h"
#include "vc4_drm.h"

struct hang {
        struct drm_vc4_get_hang_state *get_state;
        struct drm_vc4_get_hang_state_bo *bo_state;
        void **maps;
        uint32_t bo_count;
};

static void
get_hang_state(int fd, struct hang *hang)
{
        int ret;

        hang->get_state = calloc(1, sizeof(*hang->get_state));
        if (!hang->get_state)
                err(1, "malloc failure");

        ret = ioctl(fd, DRM_IOCTL_VC4_GET_HANG_STATE, hang->get_state);
        if (ret) {
                if (errno == ENOENT) {
                        fprintf(stdout, "No hang state recorded\n");
                        exit(0);
                }

                if (errno == EACCES && geteuid() != 0) {
                        fprintf(stderr, "Root permission is required to "
                                "get hang state\n");
                        exit(1);
                }

                err(1, "Initial get hang state failed");
        }

        hang->bo_count = hang->get_state->bo_count;
        hang->bo_state = calloc(hang->bo_count,
                                sizeof(struct drm_vc4_get_hang_state_bo));
        if (!hang->bo_state)
                err(1, "malloc failure");


        ret = ioctl(fd, DRM_IOCTL_VC4_GET_HANG_STATE, hang->get_state);
        if (ret)
                err(1, "Full get hang state failed");
}

static void
map_bos(int fd, struct hang *hang)
{
        int ret;

        hang->maps = calloc(hang->bo_count, sizeof(*hang->maps));
        if (!hang->maps)
                err(1, "malloc failure");

        for (int i = 0; i < hang->bo_count; i++) {
                struct drm_vc4_mmap_bo map;

                memset(&map, 0, sizeof(map));
                map.handle = hang->bo_state[i].handle;
                ret = ioctl(fd, DRM_IOCTL_VC4_MMAP_BO, &map);
                if (ret) {
                        err(1, "Couldn't get map offset for "
                            "bo %d (handle %d)", i, hang->bo_state[i].handle);
                }

                hang->maps[i] = mmap(NULL, hang->bo_state[i].size,
                                     PROT_READ, MAP_SHARED,
                                     fd, map.offset);

                if (hang->maps[i] == MAP_FAILED) {
                        err(1, "Failed to map BO %d (handle %d)",
                            i, hang->bo_state[i].handle);
                }
        }
}

static void
write_hang_state(const char *filename, struct hang *hang)
{
        uint32_t data;
        FILE *f;

        if (strcmp(filename, "-") == 0)
                f = stdout;
        else
                f = fopen(filename, "w+");
        if (!f)
                err(1, "Couldn't open %s for writing", filename);

        /* Version */
        data = 0;
        fwrite(&data, sizeof(data), 1, f);

        fwrite(hang->get_state, sizeof(*hang->get_state), 1, f);

        fwrite(hang->bo_state, sizeof(*hang->bo_state), hang->bo_count, f);

        for (int i = 0; i < hang->bo_count; i++)
                fwrite(hang->maps[i], hang->bo_state[i].size, 1, f);

        if (ferror(f))
                errx(1, "Error writing hang state file\n");

        fclose(f);
}

static void
usage(const char *name)
{
        fprintf(stderr, "Usage: %s hang_file\n", name);
        exit(1);
}

int
main(int argc, char **argv)
{
        int fd;
        struct hang hang;

        if (argc != 2)
                usage(argv[0]);

        memset(&hang, 0, sizeof(hang));

        fd = drmOpen("vc4", NULL);
        if (fd == -1)
                err(1, "couldn't open DRM node");

        get_hang_state(fd, &hang);
        map_bos(fd, &hang);
        write_hang_state(argv[1], &hang);

        return 0;
}
