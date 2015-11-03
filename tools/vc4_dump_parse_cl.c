/*
 * Copyright © 2014 Broadcom
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

#include "vc4_dump_parse.h"
#include "vc4_packet.h"
#include "vc4_tools.h"

#define dump_VC4_PACKET_LINE_WIDTH dump_float
#define dump_VC4_PACKET_POINT_SIZE dump_float


static void
dump_float(void *cl, uint32_t offset)
{
        printf("0x%08x:      %f (0x%08x)\n",
               offset, *(float *)cl, *(uint32_t *)cl);
}

static void
dump_VC4_PACKET_BRANCH_TO_SUB_LIST(void *cl, uint32_t offset)
{
        uint32_t *addr = cl;

        printf("0x%08x:      addr 0x%08x\n",
               offset, *addr);
}

static void
dump_VC4_PACKET_STORE_TILE_BUFFER_GENERAL(void *cl, uint32_t offset)
{
        uint8_t *bytes = cl;
        uint32_t *addr = cl + 2;

        const char *fullvg = "";
        const char *fullzs = "";
        const char *fullcolor = "";
        const char *buffer = "???";

        switch ((bytes[0] & 0x7)){
        case 0:
                buffer = "none";
                break;
        case 1:
                buffer = "color";
                break;
        case 2:
                buffer = "zs";
                break;
        case 3:
                buffer = "z";
                break;
        case 4:
                buffer = "vgmask";
                break;
        case 5:
                buffer = "full";
                if (*addr & (1 << 0))
                        fullcolor = " !color";
                if (*addr & (1 << 1))
                        fullzs = " !zs";
                if (*addr & (1 << 2))
                        fullvg = " !vgmask";
                break;
        }

        const char *tiling = "???";
        switch ((bytes[0] >> 4) & 7) {
        case 0:
                tiling = "linear";
                break;
        case 1:
                tiling = "T";
                break;
        case 2:
                tiling = "LT";
                break;
        }

        const char *format = "???";
        switch (bytes[1] & 3) {
        case 0:
                format = "RGBA8888";
                break;
        case 1:
                format = "BGR565_DITHER";
                break;
        case 2:
                format = "BGR565";
                break;
        }

        printf("0x%08x: 0x%02x %s %s\n",
               offset + 0, bytes[0],
               buffer, tiling);

        printf("0x%08x: 0x%02x %s\n",
               offset + 1, bytes[1],
               format);

        printf("0x%08x:      addr 0x%08x %s%s%s%s\n",
               offset + 2, *addr & ~15,
               fullcolor, fullzs, fullvg,
               (*addr & (1 << 3)) ? " EOF" : "");
}

static void
dump_VC4_PACKET_FLAT_SHADE_FLAGS(void *cl, uint32_t offset)
{
        uint32_t *bits = cl;

        printf("0x%08x:      bits 0x%08x\n",
               offset, *bits);
}

static void
dump_VC4_PACKET_VIEWPORT_OFFSET(void *cl, uint32_t offset)
{
        uint16_t *o = cl;

        printf("0x%08x:      %f, %f (0x%04x, 0x%04x)\n",
               offset,
               o[0] / 16.0, o[1] / 16.0,
               o[0], o[1]);
}

static void
dump_VC4_PACKET_CLIPPER_XY_SCALING(void *cl, uint32_t offset)
{
        uint32_t *scale = cl;

        printf("0x%08x:      %f, %f (%f, %f, 0x%08x, 0x%08x)\n",
               offset,
               uif(scale[0]) / 16.0, uif(scale[1]) / 16.0,
               uif(scale[0]), uif(scale[1]),
               scale[0], scale[1]);
}

static void
dump_VC4_PACKET_CLIPPER_Z_SCALING(void *cl, uint32_t offset)
{
        uint32_t *translate = cl;
        uint32_t *scale = cl + 8;

        printf("0x%08x:      %f, %f (0x%08x, 0x%08x)\n",
               offset,
               uif(translate[0]), uif(translate[1]),
               translate[0], translate[1]);

        printf("0x%08x:      %f, %f (0x%08x, 0x%08x)\n",
               offset + 8,
               uif(scale[0]), uif(scale[1]),
               scale[0], scale[1]);
}

static void
dump_VC4_PACKET_TILE_RENDERING_MODE_CONFIG(void *cl, uint32_t offset)
{
        uint32_t *render_offset = cl;
        uint16_t *shorts = cl + 4;
        uint8_t *bytes = cl + 8;

        printf("0x%08x:       color offset 0x%08x\n",
               offset,
               *render_offset);

        printf("0x%08x:       width %d\n",
               offset + 4,
               shorts[0]);

        printf("0x%08x:       height %d\n",
               offset + 6,
               shorts[1]);

        const char *format = "???";
        switch ((bytes[0] >> 2) & 3) {
        case 0:
                format = "BGR565_DITHERED";
                break;
        case 1:
                format = "RGBA8888";
                break;
        case 2:
                format = "BGR565";
                break;
        }
        if (shorts[2] & VC4_RENDER_CONFIG_TILE_BUFFER_64BIT)
                format = "64bit";

        const char *tiling = "???";
        switch ((bytes[0] >> 6) & 3) {
        case 0:
                tiling = "linear";
                break;
        case 1:
                tiling = "T";
                break;
        case 2:
                tiling = "LT";
                break;
        }

        printf("0x%08x: 0x%02x %s %s %s\n",
               offset + 8,
               bytes[0],
               format, tiling,
               (bytes[0] & VC4_RENDER_CONFIG_MS_MODE_4X) ? "ms" : "ss");

        const char *earlyz = "";
        if (bytes[1] & (1 << 3)) {
                earlyz = "early_z disabled";
        } else {
                if (bytes[1] & (1 << 2))
                        earlyz = "early_z >";
                else
                        earlyz = "early_z <";
        }

        printf("0x%08x: 0x%02x %s\n",
               offset + 9,
               bytes[1],
               earlyz);
}

static void
dump_VC4_PACKET_TILE_COORDINATES(void *cl, uint32_t offset)
{
        uint8_t *tilecoords = cl;

        printf("0x%08x:      %d, %d\n",
               offset, tilecoords[0], tilecoords[1]);
}

#define PACKET_DUMP(name, size) [name] = { #name, size, dump_##name }
#define PACKET(name, size) [name] = { #name, size, NULL }

static const struct packet_info {
        const char *name;
        uint8_t size;
        void (*dump_func)(void *cl, uint32_t offset);
} packet_info[] = {
        PACKET(VC4_PACKET_HALT, 1),
        PACKET(VC4_PACKET_NOP, 1),

        PACKET(VC4_PACKET_FLUSH, 1),
        PACKET(VC4_PACKET_FLUSH_ALL, 1),
        PACKET(VC4_PACKET_START_TILE_BINNING, 1),
        PACKET(VC4_PACKET_INCREMENT_SEMAPHORE, 1),
        PACKET(VC4_PACKET_WAIT_ON_SEMAPHORE, 1),

        PACKET(VC4_PACKET_BRANCH, 5),
        PACKET_DUMP(VC4_PACKET_BRANCH_TO_SUB_LIST, 5),

        PACKET(VC4_PACKET_STORE_MS_TILE_BUFFER, 1),
        PACKET(VC4_PACKET_STORE_MS_TILE_BUFFER_AND_EOF, 1),
        PACKET(VC4_PACKET_STORE_FULL_RES_TILE_BUFFER, 5),
        PACKET(VC4_PACKET_LOAD_FULL_RES_TILE_BUFFER, 5),
        PACKET_DUMP(VC4_PACKET_STORE_TILE_BUFFER_GENERAL, 7),
        PACKET(VC4_PACKET_LOAD_TILE_BUFFER_GENERAL, 7),

        PACKET(VC4_PACKET_GL_INDEXED_PRIMITIVE, 14),
        PACKET(VC4_PACKET_GL_ARRAY_PRIMITIVE, 10),

        PACKET(VC4_PACKET_COMPRESSED_PRIMITIVE, 48),
        PACKET(VC4_PACKET_CLIPPED_COMPRESSED_PRIMITIVE, 49),

        PACKET(VC4_PACKET_PRIMITIVE_LIST_FORMAT, 2),

        PACKET(VC4_PACKET_GL_SHADER_STATE, 5),
        PACKET(VC4_PACKET_NV_SHADER_STATE, 5),
        PACKET(VC4_PACKET_VG_SHADER_STATE, 5),

        PACKET(VC4_PACKET_CONFIGURATION_BITS, 4),
        PACKET_DUMP(VC4_PACKET_FLAT_SHADE_FLAGS, 5),
        PACKET_DUMP(VC4_PACKET_POINT_SIZE, 5),
        PACKET_DUMP(VC4_PACKET_LINE_WIDTH, 5),
        PACKET(VC4_PACKET_RHT_X_BOUNDARY, 3),
        PACKET(VC4_PACKET_DEPTH_OFFSET, 5),
        PACKET(VC4_PACKET_CLIP_WINDOW, 9),
        PACKET_DUMP(VC4_PACKET_VIEWPORT_OFFSET, 5),
        PACKET(VC4_PACKET_Z_CLIPPING, 9),
        PACKET_DUMP(VC4_PACKET_CLIPPER_XY_SCALING, 9),
        PACKET_DUMP(VC4_PACKET_CLIPPER_Z_SCALING, 9),

        PACKET(VC4_PACKET_TILE_BINNING_MODE_CONFIG, 16),
        PACKET_DUMP(VC4_PACKET_TILE_RENDERING_MODE_CONFIG, 11),
        PACKET(VC4_PACKET_CLEAR_COLORS, 14),
        PACKET_DUMP(VC4_PACKET_TILE_COORDINATES, 3),
};

void
vc4_dump_cl(uint32_t start, uint32_t end, bool is_render)
{
        uint32_t offset = start;
        uint8_t *cmds = vc4_paddr_to_pointer(start);

        while (offset < end) {
                uint8_t header = *cmds;

                if (header > ARRAY_SIZE(packet_info) ||
                    !packet_info[header].name) {
                        printf("0x%08x: Unknown packet 0x%02x (%d)!\n",
                               offset, header, header);
                        return;
                }

                const struct packet_info *p = packet_info + header;
                printf("0x%08x: 0x%02x %s\n",
                       offset,
                       header, p->name);

                if (offset + p->size <= end &&
                    p->dump_func) {
                        p->dump_func(cmds + 1, offset + 1);
                } else {
                        for (uint32_t i = 1; i < p->size; i++) {
                                if (offset + i >= end) {
                                        printf("0x%08x: CL overflow!\n",
                                               offset + i);
                                        return;
                                }
                                printf("0x%08x: 0x%02x\n",
                                       offset + i,
                                       cmds[i]);
                        }
                }

                switch (header) {
                case VC4_PACKET_HALT:
                case VC4_PACKET_STORE_MS_TILE_BUFFER_AND_EOF:
                        return;
                default:
                        break;
                }

                offset += p->size;
                cmds += p->size;
        }
}

