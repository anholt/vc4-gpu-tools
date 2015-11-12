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

#include <stdarg.h>
#include "vc4_dump_parse.h"
#include "vc4_packet.h"
#include "vc4_tools.h"

struct cl_dump_state {
        void *cl;
        uint32_t offset;
};

#define dump_VC4_PACKET_LINE_WIDTH dump_float
#define dump_VC4_PACKET_POINT_SIZE dump_float

static const char * const prim_name[] = {
        "points",
        "lines",
        "line_loop",
        "line_strip",
        "triangles",
        "triangle_strip",
        "triangle_fan"
};

static void
dump_printf(struct cl_dump_state *state, uint32_t offset,
            const char *format, ...)
        __attribute__ ((format(__printf__, 3, 4)));

static void
dump_printf(struct cl_dump_state *state, uint32_t offset,
            const char *format, ...)
{
        va_list ap;

        printf("0x%08x:      ", state->offset + offset);
        va_start(ap, format);
        vprintf(format, ap);
        va_end(ap);
}

static void
dump_float(struct cl_dump_state *state)
{
        dump_printf(state, 0, "%f (0x%08x)\n",
                    *(float *)state->cl, *(uint32_t *)state->cl);
}

static void
dump_VC4_PACKET_BRANCH_TO_SUB_LIST(struct cl_dump_state *state)
{
        uint32_t *addr = state->cl;

        dump_printf(state, 0, "addr 0x%08x\n", *addr);

        vc4_parse_add_mem_area(VC4_MEM_AREA_SUB_LIST, *addr);
}

static void
dump_loadstore_full(struct cl_dump_state *state)
{
        uint32_t bits = *(uint32_t *)state->cl;

        dump_printf(state, 0, "addr 0x%08x%s%s%s%s\n",
                    bits & ~0xf,
                    (bits & VC4_LOADSTORE_FULL_RES_DISABLE_CLEAR_ALL) ? "" : " clear",
                    (bits & VC4_LOADSTORE_FULL_RES_DISABLE_ZS) ? "" : " zs",
                    (bits & VC4_LOADSTORE_FULL_RES_DISABLE_COLOR) ? "" : " color",
                    (bits & VC4_LOADSTORE_FULL_RES_EOF) ? " eof" : "");
}

static void
dump_VC4_PACKET_LOAD_FULL_RES_TILE_BUFFER(struct cl_dump_state *state)
{
        dump_loadstore_full(state);
}

static void
dump_VC4_PACKET_STORE_FULL_RES_TILE_BUFFER(struct cl_dump_state *state)
{
        dump_loadstore_full(state);
}

static void
dump_loadstore_general(struct cl_dump_state *state)
{
        uint8_t *bytes = state->cl;
        uint32_t *addr = state->cl + 2;

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

        dump_printf(state, 0, "0x%02x %s %s\n", bytes[0], buffer, tiling);
        dump_printf(state, 1, "0x%02x %s\n", bytes[1], format);
        dump_printf(state, 2, "addr 0x%08x %s%s%s%s\n",
                    *addr & ~15,
                    fullcolor, fullzs, fullvg,
                    (*addr & (1 << 3)) ? " EOF" : "");
}

static void
dump_VC4_PACKET_STORE_TILE_BUFFER_GENERAL(struct cl_dump_state *state)
{
        dump_loadstore_general(state);
}

static void
dump_VC4_PACKET_LOAD_TILE_BUFFER_GENERAL(struct cl_dump_state *state)
{
        dump_loadstore_general(state);
}

static void
dump_VC4_PACKET_GL_INDEXED_PRIMITIVE(struct cl_dump_state *state)
{
        uint8_t *b = state->cl;
        uint32_t *count = state->cl + 1;
        uint32_t *ib_offset = state->cl + 5;
        uint32_t *max_index = state->cl + 9;

        dump_printf(state, 0, "0x%02x %s %s\n",
                    b[0], (b[0] & VC4_INDEX_BUFFER_U16) ? "16-bit" : "8-bit",
                    prim_name[b[0] & 0x7]);
        dump_printf(state, 1, "     %d verts\n", *count);
        dump_printf(state, 5, "0x%08x IB offset\n", *ib_offset);
        dump_printf(state, 9, "0x%08x max index\n", *max_index);
}

static void
dump_VC4_PACKET_GL_ARRAY_PRIMITIVE(struct cl_dump_state *state)
{
        uint8_t *b = state->cl;
        uint32_t *count = state->cl + 1;
        uint32_t *start = state->cl + 5;

        dump_printf(state, 0, "0x%02x %s\n", b[0], prim_name[b[0] & 0x7]);
        dump_printf(state, 1, "%d verts\n", *count);
        dump_printf(state, 5, "0x%08x start\n", *start);
}

static void
dump_VC4_PACKET_GL_SHADER_STATE(struct cl_dump_state *state)
{
        uint8_t *addr = state->cl;
        uint32_t paddr = *addr & ~0xf;
        uint8_t attributes = *addr & 7;
        bool extended;

        if (attributes == 0)
                attributes = 8;
        extended = *addr & (1 << 3);

        dump_printf(state, 0, "0x%08x %d attr count, %s\n",
                    paddr, attributes,
                    extended ? "extended" : "unextended");
}

static void
dump_VC4_PACKET_FLAT_SHADE_FLAGS(struct cl_dump_state *state)
{
        uint32_t *bits = state->cl;

        dump_printf(state, 0, "bits 0x%08x\n", *bits);
}

static void
dump_VC4_PACKET_CLIP_WINDOW(struct cl_dump_state *state)
{
        uint16_t *o = state->cl;

        dump_printf(state, 0, "%d, %d (b,l)\n", o[0], o[1]);
        dump_printf(state, 2, "%d, %d (w,h)\n", o[2], o[3]);
}

static void
dump_VC4_PACKET_VIEWPORT_OFFSET(struct cl_dump_state *state)
{
        uint16_t *o = state->cl;

        dump_printf(state, 0, "%f, %f (0x%04x, 0x%04x)\n",
                    o[0] / 16.0, o[1] / 16.0,
                    o[0], o[1]);
}

static void
dump_VC4_PACKET_CLIPPER_XY_SCALING(struct cl_dump_state *state)
{
        uint32_t *scale = state->cl;

        dump_printf(state, 0, "%f, %f (%f, %f, 0x%08x, 0x%08x)\n",
                    uif(scale[0]) / 16.0, uif(scale[1]) / 16.0,
                    uif(scale[0]), uif(scale[1]),
                    scale[0], scale[1]);
}

static void
dump_VC4_PACKET_CLIPPER_Z_SCALING(struct cl_dump_state *state)
{
        uint32_t *translate = state->cl;
        uint32_t *scale = state->cl + 8;

        dump_printf(state, 0, "%f, %f (0x%08x, 0x%08x)\n",
                    uif(translate[0]), uif(translate[1]),
                    translate[0], translate[1]);

        dump_printf(state, 8, "%f, %f (0x%08x, 0x%08x)\n",
                    uif(scale[0]), uif(scale[1]),
                    scale[0], scale[1]);
}

static void
dump_VC4_PACKET_TILE_BINNING_MODE_CONFIG(struct cl_dump_state *state)
{
        uint32_t *tile_alloc_addr = state->cl;
        uint32_t *tile_alloc_size = state->cl + 4;
        uint32_t *tile_state_addr = state->cl + 8;
        uint8_t *bin_x = state->cl + 12;
        uint8_t *bin_y = state->cl + 13;
        uint8_t *flags = state->cl + 14;

        dump_printf(state, 0, " tile alloc addr 0x%08x\n", *tile_alloc_addr);
        dump_printf(state, 4, " tile alloc size %db\n", *tile_alloc_size);
        dump_printf(state, 8, " tile state addr 0x%08x\n", *tile_state_addr);
        dump_printf(state, 12, " tiles (%d, %d)\n", *bin_x, *bin_y);
        dump_printf(state, 14, " flags 0x%02x\n", *flags);
}

static void
dump_VC4_PACKET_TILE_RENDERING_MODE_CONFIG(struct cl_dump_state *state)
{
        uint32_t *render_offset = state->cl;
        uint16_t *shorts = state->cl + 4;
        uint8_t *bytes = state->cl + 8;

        dump_printf(state, 0, "color offset 0x%08x\n", *render_offset);
        dump_printf(state, 4, "width %d\n", shorts[0]);
        dump_printf(state, 6, "height %d\n", shorts[1]);

        const char *format = "???";
        switch (VC4_GET_FIELD(shorts[2], VC4_RENDER_CONFIG_FORMAT)) {
        case VC4_RENDER_CONFIG_FORMAT_BGR565_DITHERED:
                format = "BGR565_DITHERED";
                break;
        case VC4_RENDER_CONFIG_FORMAT_RGBA8888:
                format = "RGBA8888";
                break;
        case VC4_RENDER_CONFIG_FORMAT_BGR565:
                format = "BGR565";
                break;
        }
        if (shorts[2] & VC4_RENDER_CONFIG_TILE_BUFFER_64BIT)
                format = "64bit";

        const char *tiling = "???";
        switch (VC4_GET_FIELD(shorts[2], VC4_RENDER_CONFIG_MEMORY_FORMAT)) {
        case VC4_TILING_FORMAT_LINEAR:
                tiling = "linear";
                break;
        case VC4_TILING_FORMAT_T:
                tiling = "T";
                break;
        case VC4_TILING_FORMAT_LT:
                tiling = "LT";
                break;
        }

        dump_printf(state, 8, "0x%02x %s %s %s %s\n",
                    bytes[0],
                    format, tiling,
                    (shorts[2] & VC4_RENDER_CONFIG_MS_MODE_4X) ? "ms" : "ss",
                    (shorts[2] & VC4_RENDER_CONFIG_DECIMATE_MODE_4X) ?
                    "ms_decimate" : "ss_decimate");

        const char *earlyz = "";
        if (shorts[2] & VC4_RENDER_CONFIG_EARLY_Z_COVERAGE_DISABLE) {
                earlyz = "early_z disabled";
        } else {
                if (shorts[2] & VC4_RENDER_CONFIG_EARLY_Z_DIRECTION_G)
                        earlyz = "early_z >";
                else
                        earlyz = "early_z <";
        }

        dump_printf(state, 0, "0x%02x %s\n", bytes[1], earlyz);
}

static void
dump_VC4_PACKET_CLEAR_COLORS(struct cl_dump_state *state)
{
        uint32_t *colors = state->cl;
        uint8_t *s = state->cl + 12;

        dump_printf(state, 0, "0x%08x rgba8888[0]\n", colors[0]);
        dump_printf(state, 4, "0x%08x rgba8888[1]\n", colors[1]);
        dump_printf(state, 8, "0x%08x zs\n", colors[2]);
        dump_printf(state, 12, "0x%02x stencil\n", *s);
}

static void
dump_VC4_PACKET_TILE_COORDINATES(struct cl_dump_state *state)
{
        uint8_t *tilecoords = state->cl;

        dump_printf(state, 0, "%d, %d\n",
                    tilecoords[0], tilecoords[1]);
}

static void
dump_VC4_PACKET_GEM_HANDLES(struct cl_dump_state *state)
{
        uint32_t *handles = state->cl;

        dump_printf(state, 0, "handle 0: %d, handle 1: %d\n",
                    handles[0], handles[1]);
}

#define PACKET_DUMP(name) [name] = { #name, name ## _SIZE, dump_##name }
#define PACKET(name) [name] = { #name, name ## _SIZE, NULL }

static const struct packet_info {
        const char *name;
        uint8_t size;
        void (*dump_func)(struct cl_dump_state *state);
} packet_info[] = {
        PACKET(VC4_PACKET_HALT),
        PACKET(VC4_PACKET_NOP),

        PACKET(VC4_PACKET_FLUSH),
        PACKET(VC4_PACKET_FLUSH_ALL),
        PACKET(VC4_PACKET_START_TILE_BINNING),
        PACKET(VC4_PACKET_INCREMENT_SEMAPHORE),
        PACKET(VC4_PACKET_WAIT_ON_SEMAPHORE),

        PACKET(VC4_PACKET_BRANCH),
        PACKET_DUMP(VC4_PACKET_BRANCH_TO_SUB_LIST),

        PACKET(VC4_PACKET_STORE_MS_TILE_BUFFER),
        PACKET(VC4_PACKET_STORE_MS_TILE_BUFFER_AND_EOF),
        PACKET_DUMP(VC4_PACKET_STORE_FULL_RES_TILE_BUFFER),
        PACKET_DUMP(VC4_PACKET_LOAD_FULL_RES_TILE_BUFFER),
        PACKET_DUMP(VC4_PACKET_STORE_TILE_BUFFER_GENERAL),
        PACKET_DUMP(VC4_PACKET_LOAD_TILE_BUFFER_GENERAL),

        PACKET_DUMP(VC4_PACKET_GL_INDEXED_PRIMITIVE),
        PACKET_DUMP(VC4_PACKET_GL_ARRAY_PRIMITIVE),

        PACKET(VC4_PACKET_COMPRESSED_PRIMITIVE),
        PACKET(VC4_PACKET_CLIPPED_COMPRESSED_PRIMITIVE),

        PACKET(VC4_PACKET_PRIMITIVE_LIST_FORMAT),

        PACKET_DUMP(VC4_PACKET_GL_SHADER_STATE),
        PACKET(VC4_PACKET_NV_SHADER_STATE),
        PACKET(VC4_PACKET_VG_SHADER_STATE),

        PACKET(VC4_PACKET_CONFIGURATION_BITS),
        PACKET_DUMP(VC4_PACKET_FLAT_SHADE_FLAGS),
        PACKET_DUMP(VC4_PACKET_POINT_SIZE),
        PACKET_DUMP(VC4_PACKET_LINE_WIDTH),
        PACKET(VC4_PACKET_RHT_X_BOUNDARY),
        PACKET(VC4_PACKET_DEPTH_OFFSET),
        PACKET_DUMP(VC4_PACKET_CLIP_WINDOW),
        PACKET_DUMP(VC4_PACKET_VIEWPORT_OFFSET),
        PACKET(VC4_PACKET_Z_CLIPPING),
        PACKET_DUMP(VC4_PACKET_CLIPPER_XY_SCALING),
        PACKET_DUMP(VC4_PACKET_CLIPPER_Z_SCALING),

        PACKET_DUMP(VC4_PACKET_TILE_BINNING_MODE_CONFIG),
        PACKET_DUMP(VC4_PACKET_TILE_RENDERING_MODE_CONFIG),
        PACKET_DUMP(VC4_PACKET_CLEAR_COLORS),
        PACKET_DUMP(VC4_PACKET_TILE_COORDINATES),

        PACKET_DUMP(VC4_PACKET_GEM_HANDLES),
};

void
vc4_dump_cl(uint32_t start, uint32_t end, bool is_render)
{
        uint32_t offset = start;
        uint8_t *cmds = vc4_paddr_to_pointer(start);
        struct cl_dump_state state;

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
                        state.cl = cmds + 1;
                        state.offset = offset + 1;
                        p->dump_func(&state);
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

