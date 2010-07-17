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

#include <algorithm>
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

    // ASSUMPTIONS
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
    int bppout = std::max(bpp1, bpp2);

    // Check whether we can loop over pixels without taking stride into account.
    bool fast_path = true;
    fast_path &= (stride1 == w * bpp1);
    fast_path &= (stride2 == w * bpp2);
    fast_path &= (strideout == w * bppout);

    int limit = w * h;

    guint32 *const in1_data = (guint32*) cairo_image_surface_get_data(in1);
    guint32 *const in2_data = (guint32*) cairo_image_surface_get_data(in2);
    guint32 *const out_data = (guint32*) cairo_image_surface_get_data(out);

    // NOTE
    // OpenMP probably doesn't help much here.
    // It would be better to render more than 1 tile at a time.
    #if HAVE_OPENMP
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int num_threads = prefs->getIntLimited("/options/threading/numthreads", omp_get_num_procs(), 1, 256);
    #endif

    // The number of code paths here is evil.
    if (bpp1 == 4) {
        if (bpp2 == 4) {
            if (fast_path) {
                #if HAVE_OPENMP
                #pragma omp parallel for num_threads(num_threads)
                #endif
                for (int i = 0; i < limit; ++i) {
                    *(out_data + i) = blend(*(in1_data + i), *(in2_data + i));
                }
            } else {
                #if HAVE_OPENMP
                #pragma omp parallel for num_threads(num_threads)
                #endif
                for (int i = 0; i < h; ++i) {
                    guint32 *in1_p = in1_data + i * stride1/4;
                    guint32 *in2_p = in2_data + i * stride2/4;
                    guint32 *out_p = out_data + i * strideout/4;
                    for (int j = 0; j < w; ++j) {
                        *out_p = blend(*in1_p, *in2_p);
                        ++in1_p; ++in2_p; ++out_p;
                    }
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
                    ++in1_p; ++in2_p; ++out_p;
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
                    ++in1_p; ++in2_p; ++out_p;
                }
            }
        } else {
            // bpp1 == 1 && bpp2 == 1
            if (fast_path) {
                #if HAVE_OPENMP
                #pragma omp parallel for num_threads(num_threads)
                #endif
                for (int i = 0; i < limit; ++i) {
                    guint8 *in1_p = reinterpret_cast<guint8*>(in1_data) + i;
                    guint8 *in2_p = reinterpret_cast<guint8*>(in2_data) + i;
                    guint8 *out_p = reinterpret_cast<guint8*>(out_data) + i;
                    guint32 in1_px = *in1_p; in1_px <<= 24;
                    guint32 in2_px = *in2_p; in2_px <<= 24;
                    guint32 out_px = blend(in1_px, in2_px);
                    *out_p = out_px >> 24;
                }
            } else {
                #if HAVE_OPENMP
                #pragma omp parallel for num_threads(num_threads)
                #endif
                for (int i = 0; i < h; ++i) {
                    guint8 *in1_p = reinterpret_cast<guint8*>(in1_data) + i * stride1;
                    guint8 *in2_p = reinterpret_cast<guint8*>(in2_data) + i * stride2;
                    guint8 *out_p = reinterpret_cast<guint8*>(out_data) + i * strideout;
                    for (int j = 0; j < w; ++j) {
                        guint32 in1_px = *in1_p; in1_px <<= 24;
                        guint32 in2_px = *in2_p; in2_px <<= 24;
                        guint32 out_px = blend(in1_px, in2_px);
                        *out_p = out_px >> 24;
                        ++in1_p; ++in2_p; ++out_p;
                    }
                }
            }
        }
    }

    cairo_surface_mark_dirty(out);
}

template <typename Filter>
void ink_cairo_surface_filter(cairo_surface_t *in, cairo_surface_t *out, Filter filter)
{
    cairo_surface_flush(in);

    // ASSUMPTIONS
    // 1. Cairo ARGB32 surface strides are always divisible by 4
    // 2. We can only receive CAIRO_FORMAT_ARGB32 or CAIRO_FORMAT_A8 surfaces
    // 3. Surfaces have the same dimensions
    // 4. Output surface is A8 if input is A8

    int w = cairo_image_surface_get_width(in);
    int h = cairo_image_surface_get_height(in);
    int stridein   = cairo_image_surface_get_stride(in);
    int strideout = cairo_image_surface_get_stride(out);
    int bppin = cairo_image_surface_get_format(in) == CAIRO_FORMAT_A8 ? 1 : 4;
    int bppout = cairo_image_surface_get_format(out) == CAIRO_FORMAT_A8 ? 1 : 4;
    int limit = w * h;

    // Check whether we can loop over pixels without taking stride into account.
    bool fast_path = true;
    fast_path &= (stridein == w * bppin);
    fast_path &= (strideout == w * bppout);

    guint32 *const in_data = (guint32*) cairo_image_surface_get_data(in);
    guint32 *const out_data = (guint32*) cairo_image_surface_get_data(out);

    #if HAVE_OPENMP
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    int num_threads = prefs->getIntLimited("/options/threading/numthreads", omp_get_num_procs(), 1, 256);
    #endif

    if (bppin == 4) {
        if (bppout == 4) {
            // bppin == 4, bppout == 4
            if (fast_path) {
                #if HAVE_OPENMP
                #pragma omp parallel for num_threads(num_threads)
                #endif
                for (int i = 0; i < limit; ++i) {
                    *(out_data + i) = filter(*(in_data + i));
                }
            } else {
                #if HAVE_OPENMP
                #pragma omp parallel for num_threads(num_threads)
                #endif
                for (int i = 0; i < h; ++i) {
                    guint32 *in_p = in_data + i * stridein/4;
                    guint32 *out_p = out_data + i * strideout/4;
                    for (int j = 0; j < w; ++j) {
                        *out_p = filter(*in_p);
                        ++in_p; ++out_p;
                    }
                }
            }
        } else {
            // bppin == 4, bppout == 1
            // we use this path with COLORMATRIX_LUMINANCETOALPHA
            #if HAVE_OPENMP
            #pragma omp parallel for num_threads(num_threads)
            #endif
            for (int i = 0; i < h; ++i) {
                guint32 *in_p = in_data + i * stridein/4;
                guint8 *out_p = reinterpret_cast<guint8*>(out_data) + i * strideout;
                for (int j = 0; j < w; ++j) {
                    guint32 out_px = filter(*in_p);
                    *out_p = out_px >> 24;
                    ++in_p; ++out_p;
                }
            }
        }
    } else {
        // bppin == 1, bppout == 1
        // Note: there is no path for bppin == 1, bppout == 4 because it is useless
        if (fast_path) {
            #if HAVE_OPENMP
            #pragma omp parallel for num_threads(num_threads)
            #endif
            for (int i = 0; i < limit; ++i) {
                guint8 *in_p = reinterpret_cast<guint8*>(in_data) + i;
                guint8 *out_p = reinterpret_cast<guint8*>(out_data) + i;
                guint32 in_px = *in_p; in_px <<= 24;
                guint32 out_px = filter(in_px);
                *out_p = out_px >> 24;
            }
        } else {
            #if HAVE_OPENMP
            #pragma omp parallel for num_threads(num_threads)
            #endif
            for (int i = 0; i < h; ++i) {
                guint8 *in_p = reinterpret_cast<guint8*>(in_data) + i * stridein;
                guint8 *out_p = reinterpret_cast<guint8*>(out_data) + i * strideout;
                for (int j = 0; j < w; ++j) {
                    guint32 in_px = *in_p; in_px <<= 24;
                    guint32 out_px = filter(in_px);
                    *out_p = out_px >> 24;
                    ++in_p; ++out_p;
                }
            }
        }
    }
}

// Some helpers for pixel manipulation

G_GNUC_CONST inline gint32
pxclamp(gint32 v, gint32 low, gint32 high) {
    if (v < low) return low;
    if (v > high) return high;
    return v;
}

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
