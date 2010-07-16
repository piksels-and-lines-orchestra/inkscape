/**
 * @file
 * @brief Cairo software blending templates
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2010 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_INKSCAPE_DISPLAY_CAIRO_TEMPLATES_H
#define SEEN_INKSCAPE_DISPLAY_CAIRO_TEMPLATES_H

#ifdef HAVE_OPENMP
#include <omp.h>
#include "preferences.h"
#endif

#include <cairo.h>
#include <glib.h>

/**
 * @brief Blend two surfaces using the supplied functor.
 * This template blends two Cairo image surfaces using a blending functor that takes
 * two 32-bit ARGB pixel values and returns a modified 32-bit pixel value.
 * Differences in input surface formats are handled transparently. In future, this template
 * will also handle software fallback for GL surfaces. */
template <typename Blend>
void ink_cairo_surface_blend(cairo_surface_t *in1, cairo_surface_t *in2, cairo_surface_t *out, Blend blend)
{
    cairo_surface_flush(in1);
    cairo_surface_flush(in2);

    // WARNING: code below assumes that:
    // 1. Cairo ARGB32 surface strides are always divisible by 4
    // 2. We can only receive CAIRO_FORMAT_ARGB32 or CAIRO_FORMAT_A8 surfaces
    // 3. Both surfaces are of the same size
    // 4. Output surface is ARGB32 if at least one input is ARGB32

    int w = cairo_image_surface_get_width(in2);
    int h = cairo_image_surface_get_height(in2);
    int stride1   = cairo_image_surface_get_stride(in1);
    int stride2   = cairo_image_surface_get_stride(in2);
    int strideout = cairo_image_surface_get_stride(out);
    int bpp1   = cairo_image_surface_get_format(in1) == CAIRO_FORMAT_A8 ? 1 : 4;
    int bpp2   = cairo_image_surface_get_format(in2) == CAIRO_FORMAT_A8 ? 1 : 4;

    guint32 *const in1_data = (guint32*) cairo_image_surface_get_data(in1);
    guint32 *const in2_data = (guint32*) cairo_image_surface_get_data(in2);
    guint32 *const out_data = (guint32*) cairo_image_surface_get_data(out);

    #if HAVE_OPENMP
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int num_threads = prefs->getIntLimited("/options/threading/numthreads", omp_get_num_procs(), 1, 256);
    #endif

    if (bpp1 == 4) {
        if (bpp2 == 4) {
            #if HAVE_OPENMP
            #pragma omp parallel for num_threads(num_threads)
            #endif
            for (int i = 0; i < h; ++i) {
                guint32 *in1_p = in1_data + i * stride1/4;
                guint32 *in2_p = in2_data + i * stride2/4;
                guint32 *out_p = out_data + i * strideout/4;
                for (int j = 0; j < w; ++j) {
                    *out_p = blend(*in1_p, *in2_p);
                    ++in1_p;
                    ++in2_p;
                    ++out_p;
                }
            }
        } else {
            // bpp2 == 1
            #if HAVE_OPENMP
            #pragma omp parallel for num_threads(num_threads)
            #endif
            for (int i = 0; i < h; ++i) {
                guint32 *in1_p = in1_data + i * stride1/4;
                guint8  *in2_p = reinterpret_cast<guint8*>(in2_data) + i * stride2;
                guint32 *out_p = out_data + i * strideout/4;
                for (int j = 0; j < w; ++j) {
                    guint32 in2_px = *in2_p;
                    in2_px <<= 24;
                    *out_p = blend(*in1_p, in2_px);
                    ++in1_p;
                    ++in2_p;
                    ++out_p;
                }
            }
        }
    } else {
        if (bpp2 == 4) {
            // bpp1 == 1
            #if HAVE_OPENMP
            #pragma omp parallel for num_threads(num_threads)
            #endif
            for (int i = 0; i < h; ++i) {
                guint8  *in1_p = reinterpret_cast<guint8*>(in1_data) + i * stride1;
                guint32 *in2_p = in2_data + i * stride2/4;
                guint32 *out_p = out_data + i * strideout/4;
                for (int j = 0; j < w; ++j) {
                    guint32 in1_px = *in1_p;
                    in1_px <<= 24;
                    *out_p = blend(in1_px, *in2_p);
                    ++in1_p;
                    ++in2_p;
                    ++out_p;
                }
            }
        } else {
            // bpp1 == 1 && bpp2 == 1
            // don't do anything - this should have been handled via Cairo blending
            g_assert_not_reached();
        }
    }

    cairo_surface_mark_dirty(out);
}

// helper macros for pixel extraction
#define EXTRACT_ARGB32(px,a,r,g,b) \
    guint32 a, r, g, b; \
    a = (px & 0xff000000) >> 24; \
    r = (px & 0x00ff0000) >> 16; \
    g = (px & 0x0000ff00) >> 8;  \
    b = (px & 0x000000ff);

#define ASSEMBLE_ARGB32(px,a,r,g,b) \
    guint32 px = (a << 24) | (r << 16) | (g << 8) | b;

#endif
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
