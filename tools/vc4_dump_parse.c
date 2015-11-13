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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "vc4_drm.h"

#include "list.h"
#include "vc4_dump_parse.h"

static void *
map_input(const char *filename)
{
        int fd;
        void *map;
        struct stat stat;
        int ret;

        fd = open(filename, 0);
        if (fd == -1)
                err(1, "Couldn't open input file %s", filename);

        ret = fstat(fd, &stat);
        if (ret)
                err(1, "Couldn't get size of input file %s", filename);

        map = mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED)
                err(1, "Couldn't map input file %s", filename);

        return map;
}

struct vc4_mem_area_rec {
        struct list_head link;

        enum vc4_mem_area_type type;
        void *addr;
        uint32_t paddr;
        uint32_t size;
};

static struct {
        struct drm_vc4_get_hang_state *state;
        struct drm_vc4_get_hang_state_bo *bo_state;
        void **map;

        struct list_head mem_areas;
} dump;

static void
dump_bo_list(void)
{
        fprintf(stderr, "BOs:\n");

        for (int i = 0; i < dump.state->bo_count; i++) {
                uint32_t paddr = dump.bo_state[i].paddr;
                fprintf(stderr, "0x%08x..0x%08x (%p)\n",
                        paddr,
                        paddr + dump.bo_state[i].size - 1,
                        dump.map[i]);
        }
}

void *
vc4_paddr_to_pointer(uint32_t addr)
{
        for (int i = 0; i < dump.state->bo_count; i++) {
                uint32_t paddr = dump.bo_state[i].paddr;
                if (addr >= paddr && addr < paddr + dump.bo_state[i].size)
                        return dump.map[i] + (addr - paddr);
        }

        fprintf(stderr, "Couldn't translate address 0x%08x\n", addr);
        dump_bo_list();

        return NULL;
}

uint32_t
vc4_pointer_to_paddr(void *p)
{
        for (int i = 0; i < dump.state->bo_count; i++) {
                void *map = dump.map[i];
                if (p >= map && p < map + dump.bo_state[i].size)
                        return dump.bo_state[i].paddr + (p - map);
        }

        fprintf(stderr, "Couldn't translate pointer %p\n", p);
        dump_bo_list();

        return NULL;
}

static uint32_t
vc4_get_end_paddr(uint32_t paddr)
{
        for (int i = 0; i < dump.state->bo_count; i++) {
                uint32_t start = dump.bo_state[i].paddr;
                uint32_t end = start + dump.bo_state[i].size;
                if (paddr >= start && paddr < end)
                        return end;
        }

        fprintf(stderr, "Couldn't translate paddr 0x%08x\n", paddr);
        dump_bo_list();

        return NULL;
}

void
vc4_parse_add_mem_area_sized(enum vc4_mem_area_type type, uint32_t paddr,
                             uint32_t size)
{
        struct vc4_mem_area_rec *rec;

        rec = calloc(1, sizeof(*rec));
        rec->type = type;
        rec->paddr = paddr;
        rec->addr = vc4_paddr_to_pointer(paddr);
        rec->size = size;

        list_addtail(&rec->link, &dump.mem_areas);
}

void
vc4_parse_add_mem_area(enum vc4_mem_area_type type, uint32_t paddr)
{
        uint32_t end_paddr = vc4_get_end_paddr(paddr);
        vc4_parse_add_mem_area_sized(type, paddr, end_paddr - paddr);
}

static void
set_bo_maps(void *input)
{
        uint32_t *version = input;
        if (*version != 0) {
                fprintf(stderr, "Input had wrong version %d\n", *version);
                exit(1);
        }

        dump.state = (void *)&version[1];
        dump.bo_state = (void *)&dump.state[1];

        dump.map = calloc(dump.state->bo_count, sizeof(*dump.map));
        if (!dump.map)
                err(1, "malloc failure");

        void *next_map = &dump.bo_state[dump.state->bo_count];
        for (int i = 0; i < dump.state->bo_count; i++) {
                dump.map[i] = next_map;
                next_map += dump.bo_state[i].size;
        }
}

static void
parse_cls(void)
{
        if (dump.state->start_bin != dump.state->ct0ea) {
                printf("Bin CL at 0x%08x\n", dump.state->start_bin);
                vc4_dump_cl(dump.state->start_bin, dump.state->ct0ea, false);
        }

        printf("Render CL at 0x%08x\n", dump.state->start_render);
        vc4_dump_cl(dump.state->start_render, dump.state->ct1ea, true);
}

static void
parse_sublists(void)
{
        list_for_each_entry(struct vc4_mem_area_rec, rec, &dump.mem_areas,
                            link) {
                printf("Sublist at 0x%08x:\n", rec->paddr);
                vc4_dump_cl(rec->paddr, rec->paddr + rec->size, true);
                printf("\n");
        }
}

static void
usage(const char *name)
{
        fprintf(stderr, "Usage: %s input.dump\n", name);
        exit(1);
}

static void
dump_registers(void)
{
        printf("Bin CL:         0x%08x to 0x%08x\n",
               dump.state->start_bin, dump.state->ct0ea);
        printf("Bin current:    0x%08x\n", dump.state->ct0ca);
        printf("Render CL:      0x%08x to 0x%08x\n",
               dump.state->start_render, dump.state->ct1ea);
        printf("Render current: 0x%08x\n", dump.state->ct1ca);
        printf("\n");

        printf("V3D_VPMBASE:    0x%08x\n", dump.state->vpmbase);
        printf("V3D_DBGE:       0x%08x\n", dump.state->dbge);
        printf("V3D_FDBGO:      0x%08x\n", dump.state->fdbgo);
        printf("V3D_FDBGB:      0x%08x\n", dump.state->fdbgb);
        printf("V3D_FDBGR:      0x%08x\n", dump.state->fdbgr);
        printf("V3D_FDBGS:      0x%08x\n", dump.state->fdbgs);
        printf("V3D_ERRSTAT:    0x%08x\n", dump.state->errstat);
        printf("\n");
}

int
main(int argc, char **argv)
{
        void *input;

        list_inithead(&dump.mem_areas);

        if (argc != 2)
                usage(argv[0]);

        input = map_input(argv[1]);
        set_bo_maps(input);

        dump_registers();
        parse_cls();
        parse_sublists();

        return 0;
}
