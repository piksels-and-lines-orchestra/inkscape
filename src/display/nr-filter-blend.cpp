/*
 * SVG feBlend renderer
 *
 * "This filter composites two objects together using commonly used
 * imaging software blending modes. It performs a pixel-wise combination
 * of two input images." 
 * http://www.w3.org/TR/SVG11/filters.html#feBlend
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *   Jasper van de Gronde <th.v.d.gronde@hccnet.nl>
 *
 * Copyright (C) 2007-2008 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "display/cairo-utils.h"
#include "display/nr-filter-blend.h"
#include "display/nr-filter-primitive.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-types.h"
#include "preferences.h"

namespace Inkscape {
namespace Filters {

/*
 * From http://www.w3.org/TR/SVG11/filters.html#feBlend
 *
 * For all feBlend modes, the result opacity is computed as follows:
 * qr = 1 - (1-qa)*(1-qb)
 *
 * For the compositing formulas below, the following definitions apply:
 * cr = Result color (RGB) - premultiplied
 * qa = Opacity value at a given pixel for image A
 * qb = Opacity value at a given pixel for image B
 * ca = Color (RGB) at a given pixel for image A - premultiplied
 * cb = Color (RGB) at a given pixel for image B - premultiplied
 */

FilterBlend::FilterBlend() 
    : _blend_mode(BLEND_NORMAL),
      _input2(NR_FILTER_SLOT_NOT_SET)
{}

FilterPrimitive * FilterBlend::create() {
    return new FilterBlend();
}

FilterBlend::~FilterBlend()
{}

#define EXTRACT_ARGB32(px,a,r,g,b) \
    guint32 a, r, g, b; \
    a = (px & 0xff000000) >> 24; \
    r = (px & 0x00ff0000) >> 16; \
    g = (px & 0x0000ff00) >> 8;  \
    b = (px & 0x000000ff);

#define ASSEMBLE_ARGB32(px,a,r,g,b) \
    guint32 px = (a << 24) | (r << 16) | (g << 8) | b;

// cr = (1-qa)*cb + (1-qb)*ca + ca*cb
struct BlendMultiply {
    void operator()(guint32 in1, guint32 in2, guint32 *out)
    {
        EXTRACT_ARGB32(in1, aa, ra, ga, ba)
        EXTRACT_ARGB32(in2, ab, rb, gb, bb)

        guint32 ao = 255*255 - (255-aa)*(255-ab);        ao = (ao + 127) / 255;
        guint32 ro = (255-aa)*rb + (255-ab)*ra + ra*rb;  ro = (ro + 127) / 255;
        guint32 go = (255-aa)*gb + (255-ab)*ga + ga*gb;  go = (go + 127) / 255;
        guint32 bo = (255-aa)*bb + (255-ab)*ba + ba*bb;  bo = (bo + 127) / 255;

        ASSEMBLE_ARGB32(pxout, ao, ro, go, bo)
        *out = pxout;
    }
};

// cr = cb + ca - ca * cb
struct BlendScreen {
    void operator()(guint32 in1, guint32 in2, guint32 *out)
    {
        EXTRACT_ARGB32(in1, aa, ra, ga, ba)
        EXTRACT_ARGB32(in2, ab, rb, gb, bb)

        guint32 ao = 255*255 - (255-aa)*(255-ab);    ao = (ao + 127) / 255;
        guint32 ro = 255*(rb + ra) - ra * rb;        ro = (ro + 127) / 255;
        guint32 go = 255*(gb + ga) - ga * gb;        go = (go + 127) / 255;
        guint32 bo = 255*(bb + ba) - ba * bb;        bo = (bo + 127) / 255;

        ASSEMBLE_ARGB32(pxout, ao, ro, go, bo)
        *out = pxout;
    }
};

// cr = Min ((1 - qa) * cb + ca, (1 - qb) * ca + cb)
struct BlendDarken {
    void operator()(guint32 in1, guint32 in2, guint32 *out)
    {
        EXTRACT_ARGB32(in1, aa, ra, ga, ba)
        EXTRACT_ARGB32(in2, ab, rb, gb, bb)

        guint32 ao = 255*255 - (255-aa)*(255-ab);                           ao = (ao + 127) / 255;
        guint32 ro = std::min((255-aa)*rb + 255*ra, (255-ab)*ra + 255*rb);  ro = (ro + 127) / 255;
        guint32 go = std::min((255-aa)*gb + 255*ga, (255-ab)*ga + 255*gb);  go = (go + 127) / 255;
        guint32 bo = std::min((255-aa)*bb + 255*ba, (255-ab)*ba + 255*bb);  bo = (bo + 127) / 255;

        ASSEMBLE_ARGB32(pxout, ao, ro, go, bo)
        *out = pxout;
    }
};

// cr = Max ((1 - qa) * cb + ca, (1 - qb) * ca + cb)
struct BlendLighten {
    void operator()(guint32 in1, guint32 in2, guint32 *out)
    {
        EXTRACT_ARGB32(in1, aa, ra, ga, ba)
        EXTRACT_ARGB32(in2, ab, rb, gb, bb)

        guint32 ao = 255*255 - (255-aa)*(255-ab);                           ao = (ao + 127) / 255;
        guint32 ro = std::max((255-aa)*rb + 255*ra, (255-ab)*ra + 255*rb);  ro = (ro + 127) / 255;
        guint32 go = std::max((255-aa)*gb + 255*ga, (255-ab)*ga + 255*gb);  go = (go + 127) / 255;
        guint32 bo = std::max((255-aa)*bb + 255*ba, (255-ab)*ba + 255*bb);  bo = (bo + 127) / 255;

        ASSEMBLE_ARGB32(pxout, ao, ro, go, bo)
        *out = pxout;
    }
};

/*
struct BlendAlpha
static inline void blend_alpha(guint32 in1, guint32 in2, guint32 *out)
{
    EXTRACT_ARGB32(in1, a1, a2, a3, a4);
    EXTRACT_ARGB32(in2, b1, b2, b3, b4);

    guint32 o1 = 255*255 - (255-a1)*(255-b1);  o1 = (o1+127) / 255;
    guint32 o2 = 255*255 - (255-a2)*(255-b2);  o2 = (o2+127) / 255;
    guint32 o3 = 255*255 - (255-a3)*(255-b3);  o3 = (o3+127) / 255;
    guint32 o4 = 255*255 - (255-a4)*(255-b4);  o4 = (o4+127) / 255;

    ASSEMBLE_ARGB32(pxout, o1, o2, o3, o4);
    *out = pxout;
}
*/

template <typename Blend>
void surface_blend(cairo_surface_t *in1, cairo_surface_t *in2, cairo_surface_t *out)
{
    cairo_surface_flush(in1);
    cairo_surface_flush(in2);

    // WARNING: code below assumes that:
    // 1. Cairo ARGB32 surface strides are always divisible by 4
    // 2. We can only receive CAIRO_FORMAT_ARGB32 or CAIRO_FORMAT_A8 surfaces

    int w = cairo_image_surface_get_width(in2);
    int h = cairo_image_surface_get_height(in2);
    int stride1   = cairo_image_surface_get_stride(in1);
    int stride2   = cairo_image_surface_get_stride(in2);
    int strideout = cairo_image_surface_get_stride(out);
    int bpp1   = cairo_image_surface_get_format(in1) == CAIRO_FORMAT_A8 ? 1 : 4;
    int bpp2   = cairo_image_surface_get_format(in2) == CAIRO_FORMAT_A8 ? 1 : 4;
    // assumption: out surface is CAIRO_FORMAT_ARGB32 if at least one input is ARGB32

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
                    Blend()(*in1_p, *in2_p, out_p);
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
                    Blend()(*in1_p, in2_px, out_p);
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
                    Blend()(in1_px, *in2_p, out_p);
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

void FilterBlend::render_cairo(FilterSlot &slot)
{
    cairo_surface_t *input1 = slot.getcairo(_input);
    cairo_surface_t *input2 = slot.getcairo(_input2);

    cairo_content_t ct1 = cairo_surface_get_content(input1);
    cairo_content_t ct2 = cairo_surface_get_content(input2);

    // input2 is the "background" image
    // out should be ARGB32 if any of the inputs is ARGB32
    cairo_surface_t *out = NULL;
    if ((ct1 == CAIRO_CONTENT_ALPHA && ct2 == CAIRO_CONTENT_ALPHA)
        || _blend_mode == BLEND_NORMAL)
    {
        out = ink_cairo_surface_copy(input2);
        cairo_t *out_ct = cairo_create(out);
        cairo_set_source_surface(out_ct, input1, 0, 0);
        cairo_paint(out_ct);
        cairo_destroy(out_ct);
    } else {
        // blend mode != normal and at least 1 surface is not pure alpha
        // create surface identical to the ARGB32 surface
        if (ct1 == CAIRO_CONTENT_ALPHA) {
            out = ink_cairo_surface_create_identical(input2);
        } else {
            out = ink_cairo_surface_create_identical(input1);
        }

        // TODO: convert to Cairo blending operators once we start using the 1.10 series
        switch (_blend_mode) {
            case BLEND_MULTIPLY:
                surface_blend<BlendMultiply>(input1, input2, out);
                break;
            case BLEND_SCREEN:
                surface_blend<BlendScreen>(input1, input2, out);
                break;
            case BLEND_DARKEN:
                surface_blend<BlendDarken>(input1, input2, out);
                break;
            case BLEND_LIGHTEN:
                surface_blend<BlendLighten>(input1, input2, out);
                break;
            case BLEND_NORMAL:
            default:
                // this was handled before
                g_assert_not_reached();
                break;
        }
    }

    slot.set(_output, out);
    cairo_surface_destroy(out);
}

bool FilterBlend::can_handle_affine(Geom::Matrix const &)
{
    // blend is a per-pixel primitive and is immutable under transformations
    return true;
}

void FilterBlend::set_input(int slot) {
    _input = slot;
}

void FilterBlend::set_input(int input, int slot) {
    if (input == 0) _input = slot;
    if (input == 1) _input2 = slot;
}

void FilterBlend::set_mode(FilterBlendMode mode) {
    if (mode == BLEND_NORMAL || mode == BLEND_MULTIPLY ||
        mode == BLEND_SCREEN || mode == BLEND_DARKEN ||
        mode == BLEND_LIGHTEN)
    {
        _blend_mode = mode;
    }
}

} /* namespace Filters */
} /* namespace Inkscape */

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
