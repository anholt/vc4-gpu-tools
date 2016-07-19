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
        uint32_t end;

        uint8_t prim_mode;
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
                    uif(*(uint32_t *)state->cl), *(uint32_t *)state->cl);
}

static void
dump_VC4_PACKET_BRANCH(struct cl_dump_state *state)
{
        uint32_t *addr = state->cl;

        dump_printf(state, 0, "addr 0x%08x\n", *addr);

        vc4_parse_add_sublist(*addr, state->prim_mode);
}

static void
dump_VC4_PACKET_BRANCH_TO_SUB_LIST(struct cl_dump_state *state)
{
        uint32_t *addr = state->cl;

        dump_printf(state, 0, "addr 0x%08x\n", *addr);

        vc4_parse_add_sublist(*addr, state->prim_mode);
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
dump_VC4_PACKET_PRIMITIVE_LIST_FORMAT(struct cl_dump_state *state)
{
        uint8_t *b = state->cl;
        const char *prim_mode = "unknown";
        const char *data_type = "unknown";

        switch (*b & 0xf) {
        case 0:
                prim_mode = "points";
                break;
        case 1:
                prim_mode = "lines";
                break;
        case 2:
                prim_mode = "triangles";
                break;
        case 3:
                prim_mode = "RHT";
                break;
        }

        switch (*b >> 4) {
        case 1:
                data_type = "16-bit index";
                break;
        case 3:
                prim_mode = "32-bit x/y";
                break;
        }

        dump_printf(state, 0, "0x%02x: prim_mode %s, data_type %s\n",
                    *b, prim_mode, data_type);

        state->prim_mode = *b & 0x0f;
}

static void
dump_VC4_PACKET_GL_SHADER_STATE(struct cl_dump_state *state)
{
        uint32_t *addr = state->cl;
        uint32_t paddr = *addr & ~0xf;
        uint8_t attributes = *addr & 7;
        bool extended;

        if (attributes == 0)
                attributes = 8;
        extended = *addr & (1 << 3);

        dump_printf(state, 0, "0x%08x %d attr count, %s\n",
                    paddr, attributes,
                    extended ? "extended" : "unextended");

        vc4_parse_add_gl_shader_rec(paddr, attributes, extended);
}

static void
dump_VC4_PACKET_CONFIGURATION_BITS(struct cl_dump_state *state)
{
        uint8_t *b = state->cl;
        const char *msaa;

        switch (b[0] & VC4_CONFIG_BITS_RASTERIZER_OVERSAMPLE_MASK) {
        case VC4_CONFIG_BITS_RASTERIZER_OVERSAMPLE_NONE:
                msaa = "1x";
                break;
        case VC4_CONFIG_BITS_RASTERIZER_OVERSAMPLE_4X:
                msaa = "4x";
                break;
        case VC4_CONFIG_BITS_RASTERIZER_OVERSAMPLE_16X:
                msaa = "16x";
                break;
        default:
                msaa = "unknownx";
                break;
        }

        dump_printf(state, 0,
                    "0x%02x f %d, b %d, %s, depthoff %d, aapointslines %d, %s\n",
                    b[0],
                    (b[0] & VC4_CONFIG_BITS_ENABLE_PRIM_FRONT) != 0,
                    (b[0] & VC4_CONFIG_BITS_ENABLE_PRIM_BACK) != 0,
                    (b[0] & VC4_CONFIG_BITS_CW_PRIMITIVES) ? "cw" : "ccw",
                    (b[0] & VC4_CONFIG_BITS_ENABLE_DEPTH_OFFSET) != 0,
                    (b[0] & VC4_CONFIG_BITS_AA_POINTS_AND_LINES) != 0,
                    msaa);

        dump_printf(state, 1, "0x%02x z_upd %d, z_func %d\n", b[1],
                    (b[1] & VC4_CONFIG_BITS_Z_UPDATE) != 0,
                    ((b[1] >> VC4_CONFIG_BITS_DEPTH_FUNC_SHIFT) & 0x7));


        dump_printf(state, 2, "0x%02x ez %d, ezup %d\n", b[2],
                    (b[2] & VC4_CONFIG_BITS_EARLY_Z) != 0,
                    (b[2] & VC4_CONFIG_BITS_EARLY_Z_UPDATE) != 0);

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

        const char *earlyz = "";
        if (shorts[2] & VC4_RENDER_CONFIG_EARLY_Z_COVERAGE_DISABLE) {
                earlyz = "early_z disabled";
        } else {
                if (shorts[2] & VC4_RENDER_CONFIG_EARLY_Z_DIRECTION_G)
                        earlyz = "early_z >";
                else
                        earlyz = "early_z <";
        }

        const char *decimate;
        switch (shorts[2] & VC4_RENDER_CONFIG_DECIMATE_MODE_MASK) {
        case VC4_RENDER_CONFIG_DECIMATE_MODE_1X:
                decimate = "1x";
                break;
        case VC4_RENDER_CONFIG_DECIMATE_MODE_4X:
                decimate = "4x";
                break;
        case VC4_RENDER_CONFIG_DECIMATE_MODE_16X:
                decimate = "16x";
                break;
        default:
                decimate = "unknown";
                break;
        }

        dump_printf(state, 8, "0x%04x %s, %s, %s, %s, decimate %s\n", shorts[2],
                    format, tiling,
                    earlyz,
                    (shorts[2] & VC4_RENDER_CONFIG_MS_MODE_4X) ? "ms_4x" : "ss",
                    decimate);
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

        PACKET_DUMP(VC4_PACKET_BRANCH),
        PACKET_DUMP(VC4_PACKET_BRANCH_TO_SUB_LIST),
        PACKET(VC4_PACKET_RETURN_FROM_SUB_LIST),

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

        PACKET_DUMP(VC4_PACKET_PRIMITIVE_LIST_FORMAT),

        PACKET_DUMP(VC4_PACKET_GL_SHADER_STATE),
        PACKET(VC4_PACKET_NV_SHADER_STATE),
        PACKET(VC4_PACKET_VG_SHADER_STATE),

        PACKET_DUMP(VC4_PACKET_CONFIGURATION_BITS),
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

/* Prints a single entry from Table 39: Compressed Triangles List Indices, and
 * returns the length of the encoding.
 */
static uint32_t
dump_compressed_triangle(struct cl_dump_state *state, uint32_t offset)
{
        uint8_t *cl = state->cl;
        uint32_t index_size = 2;

        if (cl[offset] == 129) {
                uint16_t *index = (void *)(&cl[offset + 1]);
                dump_printf(state, offset, "0x%02x: 3 abs, 0 rel indices\n",
                            cl[offset]);
                dump_printf(state, offset + 2, "index 0: 0x%04x\n", index[0]);
                dump_printf(state, offset + 4, "index 1: 0x%04x\n", index[1]);
                dump_printf(state, offset + 6, "index 2: 0x%04x\n", index[2]);
                return 1 + 3 * index_size;
        } else if ((cl[offset] & 0xf) == 15) {
                uint16_t *index = (void *)(&cl[offset + 2]);
                dump_printf(state, offset, "0x%02x: 1 abs, 2 rel indices\n",
                            cl[offset]);
                dump_printf(state, offset + 2, "index 0: 0x%04x\n", *index);
                return 2 + index_size;
        } else if ((cl[offset] & 0x3) == 3) {
                dump_printf(state, offset,
                            "0x%02x: 3 rel indices (%d, %d, %d)\n",
                            cl[offset],
                            (int8_t)cl[offset] >> 4,
                            ((int8_t)cl[offset + 1] << 4) >> 4,
                            (int8_t)cl[offset + 1] >> 4);
                return 2;
        } else {
                dump_printf(state, offset, "0x%02x: 1 rel index (%d)\n",
                            cl[offset], (int8_t)cl[offset] >> 2);
                return 1;
        }
}

static uint32_t
dump_compressed_primitive(struct cl_dump_state *state)
{
        uint8_t *cl = state->cl;
        uint32_t offset = 0;

        while (state->offset + offset < state->end) {
                if (cl[offset] == 128) {
                        dump_printf(state, offset, "0x%02x: escape\n",
                                    cl[offset]);
                        return offset + 1;
                } else if (cl[offset] == 130) {
                        /* The packet's offset is a 2's complement relative
                         * branch.
                         */
                        int16_t branch = *(int16_t *)&cl[offset + 1];
                        uint32_t addr = (((state->offset + offset) & ~31) +
                                         (branch << 5));
                        dump_printf(state, offset,
                                    "0x%02x: relative branch 0x%08x (0x%04x)\n",
                                    cl[offset], addr, (uint16_t)branch);
                        vc4_parse_add_compressed_list(addr, state->prim_mode);
                        return ~0;
                } else {
                        switch (state->prim_mode) {
                        case VC4_PRIMITIVE_LIST_FORMAT_TYPE_TRIANGLES:
                                offset += dump_compressed_triangle(state,
                                                                   offset) - 1;
                                break;
                        default:
                                dump_printf(state, offset,
                                            "0x%02x: unknown (UNPARSED!)\n",
                                            cl[offset]);
                        }
                }

                offset++;
        }

        printf("0x%08x: CL overflow!\n", offset);
        return offset;
}

static uint32_t
dump_clipped_compressed_primitive(struct cl_dump_state *state)
{
        uint32_t *addr = state->cl;

        dump_printf(state, 0, "clipped verts at 0x%08x, clip 0x%1x\n",
                    *addr & ~0x7, *addr & 0x7);

        state->offset += 4;
        state->cl += 4;
        uint32_t compressed_len = dump_compressed_primitive(state);
        if (compressed_len == ~0)
                return compressed_len;
        else
                return compressed_len + 4;
}

void
vc4_dump_cl(uint32_t start, uint32_t end, bool is_render,
            bool in_compressed_list, uint8_t start_prim_mode)
{
        uint32_t offset = start;
        uint8_t *cmds = vc4_paddr_to_pointer(start);
        struct cl_dump_state state;

        state.end = end;
        state.prim_mode = start_prim_mode;

        /* A relative branch in a compressed list will continue at the branch
         * target still in a compressed list.
         */
        if (in_compressed_list) {
                state.cl = cmds;
                state.offset = offset;
                uint32_t len = dump_compressed_primitive(&state);
                if (len == ~0)
                        return;

                cmds = state.cl + len;
                offset = state.offset + len;
        }

        while (offset < end) {
                uint8_t header = *cmds;
                uint32_t size;

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

                /* Use the per-packet size, unless it's variable length. */
                size = p->size;

                state.cl = cmds + 1;
                state.offset = offset + 1;
                if (header == VC4_PACKET_COMPRESSED_PRIMITIVE) {
                        uint32_t len = dump_compressed_primitive(&state);
                        if (len == ~0)
                                return;
                        size = len + 1;
                } else if (header == VC4_PACKET_CLIPPED_COMPRESSED_PRIMITIVE) {
                        uint32_t len = dump_clipped_compressed_primitive(&state);
                        if (len == ~0)
                                return;
                        size = len + 1;
                } else if (offset + size <= end && p->dump_func) {
                        p->dump_func(&state);
                } else {
                        for (uint32_t i = 1; i < size; i++) {
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
                case VC4_PACKET_RETURN_FROM_SUB_LIST:
                case VC4_PACKET_BRANCH:
                        return;
                default:
                        break;
                }

                offset += size;
                cmds += size;
        }
}

