/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef INTELCONTEXT_INC
#define INTELCONTEXT_INC


#include <stdbool.h>
#include <string.h>
#include "main/mtypes.h"
#include "main/mm.h"

#ifdef __cplusplus
extern "C" {
	/* Evil hack for using libdrm in a c++ compiler. */
	#define virtual virt
#endif

#include "drm.h"
#include "intel_bufmgr.h"

#include "intel_screen.h"
#include "intel_tex_obj.h"
#include "i915_drm.h"

#ifdef __cplusplus
	#undef virtual
#endif

#include "tnl/t_vertex.h"

#define TAG(x) intel##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

struct intel_region;
struct intel_context;

typedef void (*intel_tri_func) (struct intel_context *, intelVertex *,
                                intelVertex *, intelVertex *);
typedef void (*intel_line_func) (struct intel_context *, intelVertex *,
                                 intelVertex *);
typedef void (*intel_point_func) (struct intel_context *, intelVertex *);

#define INTEL_WRITE_PART  0x1
#define INTEL_WRITE_FULL  0x2
#define INTEL_READ        0x4

#ifndef likely
#ifdef __GNUC__
#define likely(expr) (__builtin_expect(expr, 1))
#define unlikely(expr) (__builtin_expect(expr, 0))
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif
#endif

struct intel_sync_object {
   struct gl_sync_object Base;

   /** Batch associated with this sync object */
   drm_intel_bo *bo;
};

struct brw_context;

struct intel_batchbuffer {
   /** Current batchbuffer being queued up. */
   drm_intel_bo *bo;
   /** Last BO submitted to the hardware.  Used for glFinish(). */
   drm_intel_bo *last_bo;
   /** BO for post-sync nonzero writes for gen6 workaround. */
   drm_intel_bo *workaround_bo;
   bool need_workaround_flush;

   struct cached_batch_item *cached_items;

   uint16_t emit, total;
   uint16_t used, reserved_space;
   uint32_t *map;
   uint32_t *cpu_map;
#define BATCH_SZ (8192*sizeof(uint32_t))

   uint32_t state_batch_offset;
   bool is_blit;
   bool needs_sol_reset;

   struct {
      uint16_t used;
      int reloc_count;
   } saved;
};

/**
 * intel_context is derived from Mesa's context class: struct gl_context.
 */
struct intel_context
{
   struct gl_context ctx;  /**< base class, must be first field */

   struct
   {
      void (*destroy) (struct intel_context * intel);
      void (*finish_batch) (struct intel_context * intel);
      void (*new_batch) (struct intel_context * intel);

      void (*invalidate_state) (struct intel_context *intel,
				GLuint new_state);

      void (*debug_batch)(struct intel_context *intel);
      void (*annotate_aub)(struct intel_context *intel);
      bool (*render_target_supported)(struct intel_context *intel,
				      struct gl_renderbuffer *rb);

      /** Can HiZ be enabled on a depthbuffer of the given format? */
      bool (*is_hiz_depth_format)(struct intel_context *intel,
	                          gl_format format);

      void (*update_texture_surface)(struct gl_context *ctx,
                                     unsigned unit,
                                     uint32_t *binding_table,
                                     unsigned surf_index);
      void (*update_renderbuffer_surface)(struct brw_context *brw,
					  struct gl_renderbuffer *rb,
					  bool layered,
					  unsigned unit);
      void (*update_null_renderbuffer_surface)(struct brw_context *brw,
					       unsigned unit);
      void (*create_constant_surface)(struct brw_context *brw,
				      drm_intel_bo *bo,
				      uint32_t offset,
				      uint32_t size,
				      uint32_t *out_offset,
                                      bool dword_pitch);

      /**
       * Send the appropriate state packets to configure depth, stencil, and
       * HiZ buffers (i965+ only)
       */
      void (*emit_depth_stencil_hiz)(struct brw_context *brw,
                                     struct intel_mipmap_tree *depth_mt,
                                     uint32_t depth_offset,
                                     uint32_t depthbuffer_format,
                                     uint32_t depth_surface_type,
                                     struct intel_mipmap_tree *stencil_mt,
                                     bool hiz, bool separate_stencil,
                                     uint32_t width, uint32_t height,
                                     uint32_t tile_x, uint32_t tile_y);

   } vtbl;

   GLuint NewGLState;
 
   dri_bufmgr *bufmgr;
   unsigned int maxBatchSize;

   /**
    * Generation number of the hardware: 2 is 8xx, 3 is 9xx pre-965, 4 is 965.
    */
   int gen;
   int gt;
   bool needs_ff_sync;
   bool is_haswell;
   bool is_baytrail;
   bool is_g4x;
   bool is_945;
   bool has_separate_stencil;
   bool must_use_separate_stencil;
   bool has_hiz;
   bool has_llc;
   bool has_swizzling;

   int urb_size;

   drm_intel_context *hw_ctx;

   struct intel_batchbuffer batch;

   drm_intel_bo *first_post_swapbuffers_batch;
   bool need_throttle;
   bool no_batch_wrap;

   /**
    * Set if we're either a debug context or the INTEL_DEBUG=perf environment
    * variable is set, this is the flag indicating to do expensive work that
    * might lead to a perf_debug() call.
    */
   bool perf_debug;

   struct {
      drm_intel_bo *bo;
      GLuint offset;
      uint32_t buffer_len;
      uint32_t buffer_offset;
      char buffer[4096];
   } upload;

   uint32_t max_gtt_map_object_size;

   GLuint stats_wm;

   bool hw_stencil;
   bool no_rast;
   bool always_flush_batch;
   bool always_flush_cache;
   bool disable_throttling;

   GLenum reduced_primitive;

   /**
    * Set if rendering has occured to the drawable's front buffer.
    *
    * This is used in the DRI2 case to detect that glFlush should also copy
    * the contents of the fake front buffer to the real front buffer.
    */
   bool front_buffer_dirty;

   /**
    * Track whether front-buffer rendering is currently enabled
    *
    * A separate flag is used to track this in order to support MRT more
    * easily.
    */
   bool is_front_buffer_rendering;
   /**
    * Track whether front-buffer is the current read target.
    *
    * This is closely associated with is_front_buffer_rendering, but may
    * be set separately.  The DRI2 fake front buffer must be referenced
    * either way.
    */
   bool is_front_buffer_reading;

   int driFd;

   __DRIcontext *driContext;
   struct intel_screen *intelScreen;
   void (*saved_viewport)(struct gl_context * ctx,
			  GLint x, GLint y, GLsizei width, GLsizei height);

   /**
    * Configuration cache
    */
   driOptionCache optionCache;
};

/**
 * Align a value down to an alignment value
 *
 * If \c value is not already aligned to the requested alignment value, it
 * will be rounded down.
 *
 * \param value  Value to be rounded
 * \param alignment  Alignment value to be used.  This must be a power of two.
 *
 * \sa ALIGN()
 */
#define ROUND_DOWN_TO(value, alignment) ((value) & ~(alignment - 1))

static INLINE uint32_t
U_FIXED(float value, uint32_t frac_bits)
{
   value *= (1 << frac_bits);
   return value < 0 ? 0 : value;
}

static INLINE uint32_t
S_FIXED(float value, uint32_t frac_bits)
{
   return value * (1 << frac_bits);
}

/* ================================================================
 * From linux kernel i386 header files, copes with odd sizes better
 * than COPY_DWORDS would:
 * XXX Put this in src/mesa/main/imports.h ???
 */
#if defined(i386) || defined(__i386__)
static INLINE void * __memcpy(void * to, const void * from, size_t n)
{
   int d0, d1, d2;
   __asm__ __volatile__(
      "rep ; movsl\n\t"
      "testb $2,%b4\n\t"
      "je 1f\n\t"
      "movsw\n"
      "1:\ttestb $1,%b4\n\t"
      "je 2f\n\t"
      "movsb\n"
      "2:"
      : "=&c" (d0), "=&D" (d1), "=&S" (d2)
      :"0" (n/4), "q" (n),"1" ((long) to),"2" ((long) from)
      : "memory");
   return (to);
}
#else
#define __memcpy(a,b,c) memcpy(a,b,c)
#endif


/* ================================================================
 * Debugging:
 */
extern int INTEL_DEBUG;

#define DEBUG_TEXTURE	0x1
#define DEBUG_STATE	0x2
#define DEBUG_IOCTL	0x4
#define DEBUG_BLIT	0x8
#define DEBUG_MIPTREE   0x10
#define DEBUG_PERF	0x20
#define DEBUG_BATCH     0x80
#define DEBUG_PIXEL     0x100
#define DEBUG_BUFMGR    0x200
#define DEBUG_REGION    0x400
#define DEBUG_FBO       0x800
#define DEBUG_GS        0x1000
#define DEBUG_SYNC	0x2000
#define DEBUG_PRIMS	0x4000
#define DEBUG_VERTS	0x8000
#define DEBUG_DRI       0x10000
#define DEBUG_SF        0x20000
#define DEBUG_STATS     0x100000
#define DEBUG_WM        0x400000
#define DEBUG_URB       0x800000
#define DEBUG_VS        0x1000000
#define DEBUG_CLIP      0x2000000
#define DEBUG_AUB       0x4000000
#define DEBUG_SHADER_TIME 0x8000000
#define DEBUG_BLORP     0x10000000
#define DEBUG_NO16      0x20000000

#ifdef HAVE_ANDROID_PLATFORM
#define LOG_TAG "INTEL-MESA"
#include <cutils/log.h>
#ifndef ALOGW
#define ALOGW LOGW
#endif
#define dbg_printf(...)	ALOGW(__VA_ARGS__)
#else
#define dbg_printf(...)	printf(__VA_ARGS__)
#endif /* HAVE_ANDROID_PLATFORM */

#define DBG(...) do {						\
	if (unlikely(INTEL_DEBUG & FILE_DEBUG_FLAG))		\
		dbg_printf(__VA_ARGS__);			\
} while(0)

#define perf_debug(...) do {					\
   static GLuint msg_id = 0;                                    \
   if (unlikely(INTEL_DEBUG & DEBUG_PERF))                      \
      dbg_printf(__VA_ARGS__);                                  \
   if (intel->perf_debug)                                       \
      _mesa_gl_debug(&intel->ctx, &msg_id,                      \
                     MESA_DEBUG_TYPE_PERFORMANCE,               \
                     MESA_DEBUG_SEVERITY_MEDIUM,                \
                     __VA_ARGS__);                              \
} while(0)

#define WARN_ONCE(cond, fmt...) do {                            \
   if (unlikely(cond)) {                                        \
      static bool _warned = false;                              \
      static GLuint msg_id = 0;                                 \
      if (!_warned) {                                           \
         fprintf(stderr, "WARNING: ");                          \
         fprintf(stderr, fmt);                                  \
         _warned = true;                                        \
                                                                \
         _mesa_gl_debug(ctx, &msg_id,                           \
                        MESA_DEBUG_TYPE_OTHER,                  \
                        MESA_DEBUG_SEVERITY_HIGH, fmt);         \
      }                                                         \
   }                                                            \
} while (0)

/* ================================================================
 * intel_context.c:
 */

extern bool intelInitContext(struct intel_context *intel,
                             int api,
                             unsigned major_version,
                             unsigned minor_version,
                             const struct gl_config * mesaVis,
                             __DRIcontext * driContextPriv,
                             void *sharedContextPrivate,
                             struct dd_function_table *functions,
                             unsigned *dri_ctx_error);

extern void intelFinish(struct gl_context * ctx);
extern void _intel_flush(struct gl_context * ctx, const char *file, int line);

#define intel_flush(ctx) _intel_flush(ctx, __FILE__, __LINE__)

extern void intelInitDriverFunctions(struct dd_function_table *functions);

void intel_init_syncobj_functions(struct dd_function_table *functions);


/* ================================================================
 * intel_state.c:
 */

#define COMPAREFUNC_ALWAYS		0
#define COMPAREFUNC_NEVER		0x1
#define COMPAREFUNC_LESS		0x2
#define COMPAREFUNC_EQUAL		0x3
#define COMPAREFUNC_LEQUAL		0x4
#define COMPAREFUNC_GREATER		0x5
#define COMPAREFUNC_NOTEQUAL		0x6
#define COMPAREFUNC_GEQUAL		0x7

#define STENCILOP_KEEP			0
#define STENCILOP_ZERO			0x1
#define STENCILOP_REPLACE		0x2
#define STENCILOP_INCRSAT		0x3
#define STENCILOP_DECRSAT		0x4
#define STENCILOP_INCR			0x5
#define STENCILOP_DECR			0x6
#define STENCILOP_INVERT		0x7

#define LOGICOP_CLEAR			0
#define LOGICOP_NOR			0x1
#define LOGICOP_AND_INV 		0x2
#define LOGICOP_COPY_INV		0x3
#define LOGICOP_AND_RVRSE		0x4
#define LOGICOP_INV			0x5
#define LOGICOP_XOR			0x6
#define LOGICOP_NAND			0x7
#define LOGICOP_AND			0x8
#define LOGICOP_EQUIV			0x9
#define LOGICOP_NOOP			0xa
#define LOGICOP_OR_INV			0xb
#define LOGICOP_COPY			0xc
#define LOGICOP_OR_RVRSE		0xd
#define LOGICOP_OR			0xe
#define LOGICOP_SET			0xf

#define BLENDFACT_ZERO			0x01
#define BLENDFACT_ONE			0x02
#define BLENDFACT_SRC_COLR		0x03
#define BLENDFACT_INV_SRC_COLR 		0x04
#define BLENDFACT_SRC_ALPHA		0x05
#define BLENDFACT_INV_SRC_ALPHA 	0x06
#define BLENDFACT_DST_ALPHA		0x07
#define BLENDFACT_INV_DST_ALPHA 	0x08
#define BLENDFACT_DST_COLR		0x09
#define BLENDFACT_INV_DST_COLR		0x0a
#define BLENDFACT_SRC_ALPHA_SATURATE	0x0b
#define BLENDFACT_CONST_COLOR		0x0c
#define BLENDFACT_INV_CONST_COLOR	0x0d
#define BLENDFACT_CONST_ALPHA		0x0e
#define BLENDFACT_INV_CONST_ALPHA	0x0f
#define BLENDFACT_MASK          	0x0f

enum {
   DRI_CONF_BO_REUSE_DISABLED,
   DRI_CONF_BO_REUSE_ALL
};

extern int intel_translate_shadow_compare_func(GLenum func);
extern int intel_translate_compare_func(GLenum func);
extern int intel_translate_stencil_op(GLenum op);
extern int intel_translate_logic_op(GLenum opcode);

void intel_update_renderbuffers(__DRIcontext *context,
				__DRIdrawable *drawable);
void intel_prepare_render(struct intel_context *intel);

void
intel_resolve_for_dri2_flush(struct intel_context *intel,
                             __DRIdrawable *drawable);

void i915_set_buf_info_for_region(uint32_t *state, struct intel_region *region,
				  uint32_t buffer_id);
void intel_init_texture_formats(struct gl_context *ctx);

/*======================================================================
 * Inline conversion functions.  
 * These are better-typed than the macros used previously:
 */
static INLINE struct intel_context *
intel_context(struct gl_context * ctx)
{
   return (struct intel_context *) ctx;
}

static INLINE bool
is_power_of_two(uint32_t value)
{
   return (value & (value - 1)) == 0;
}

#ifdef __cplusplus
}
#endif

#endif