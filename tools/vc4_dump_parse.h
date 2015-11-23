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

#include <stdbool.h>
#include <stdint.h>

struct vc4_mem_area_rec;

enum vc4_mem_area_type {
        VC4_MEM_AREA_GL_SHADER_REC,
        VC4_MEM_AREA_SUB_LIST,
        VC4_MEM_AREA_COMPRESSED_PRIM_LIST,
        VC4_MEM_AREA_CS,
        VC4_MEM_AREA_VS,
        VC4_MEM_AREA_FS,
};

void vc4_dump_cl(uint32_t start, uint32_t end, bool is_render,
                 bool in_compressed_list, uint8_t prim_mode);

uint32_t vc4_pointer_to_paddr(void *p);
void *vc4_paddr_to_pointer(uint32_t addr);

struct vc4_mem_area_rec *
vc4_parse_add_mem_area(enum vc4_mem_area_type type, uint32_t paddr);

struct vc4_mem_area_rec *
vc4_parse_add_mem_area_sized(enum vc4_mem_area_type type, uint32_t paddr,
                             uint32_t size);

void vc4_parse_add_sublist(uint32_t paddr, uint8_t prim_mode);
void vc4_parse_add_compressed_list(uint32_t paddr, uint8_t prim_mode);
void vc4_parse_add_gl_shader_rec(uint32_t paddr, uint8_t attributes,
                                 bool extended);
