/*
 * feColorMatrix filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Jasper van de Gronde <th.v.d.gronde@hccnet.nl>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include <algorithm>
#include "display/cairo-templates.h"
#include "display/cairo-utils.h"
#include "display/nr-filter-colormatrix.h"
#include "display/nr-filter-units.h"
#include "display/nr-filter-utils.h"

namespace Inkscape {
namespace Filters {

FilterColorMatrix::FilterColorMatrix()
{
}

FilterPrimitive * FilterColorMatrix::create() {
    return new FilterColorMatrix();
}

FilterColorMatrix::~FilterColorMatrix()
{}

#if 0
int FilterColorMatrix::render(FilterSlot &slot, FilterUnits const &/*units*/) {
    NRPixBlock *in = slot.get(_input);
    if (!in) {
        g_warning("Missing source image for feColorMatrix (in=%d)", _input);
        return 1;
    }

    NRPixBlock *out = new NRPixBlock;
    if ((type==COLORMATRIX_SATURATE || type==COLORMATRIX_HUEROTATE) && in->mode != NR_PIXBLOCK_MODE_R8G8B8A8N) {
        // saturate and hueRotate do not touch the alpha channel and are linear (per-pixel) operations, so no premultiplied -> non-premultiplied operation is necessary
        nr_pixblock_setup_fast(out, NR_PIXBLOCK_MODE_R8G8B8A8P,
                               in->area.x0, in->area.y0, in->area.x1, in->area.y1,
                               true);
    } else {
        nr_pixblock_setup_fast(out, NR_PIXBLOCK_MODE_R8G8B8A8N,
                               in->area.x0, in->area.y0, in->area.x1, in->area.y1,
                               true);
    }

    // this primitive is defined for non-premultiplied RGBA values,
    // thus convert them to that format
    //   However, since not all operations care, the input is only transformed if necessary.
    bool free_in_on_exit = false;
    if (in->mode != out->mode) {
        NRPixBlock *original_in = in;
        in = new NRPixBlock;
        nr_pixblock_setup_fast(in, out->mode,
                               original_in->area.x0, original_in->area.y0,
                               original_in->area.x1, original_in->area.y1,
                               true);
        nr_blit_pixblock_pixblock(in, original_in);
        free_in_on_exit = true;
    }

    unsigned char *in_data = NR_PIXBLOCK_PX(in);
    unsigned char *out_data = NR_PIXBLOCK_PX(out);
    unsigned char r,g,b,a;
    int x,y,x0,y0,x1,y1,i;
    x0=in->area.x0;
    y0=in->area.y0;
    x1=in->area.x1;
    y1=in->area.y1;

    switch(type){
        case COLORMATRIX_MATRIX:
            {
                if (values.size()!=20) {
                    g_warning("ColorMatrix: values parameter error. Wrong size: %i.", static_cast<int>(values.size()));
                    return -1;
                }
                double a04 = 255*values[4];
                double a14 = 255*values[9];
                double a24 = 255*values[14];
                double a34 = 255*values[19];
                for (x=x0;x<x1;x++){
                    for (y=y0;y<y1;y++){
                        i = ((x-x0) + (x1-x0)*(y-y0))*4;
                        r = in_data[i];
                        g = in_data[i+1];
                        b = in_data[i+2];
                        a = in_data[i+3];
                        out_data[i] = CLAMP_D_TO_U8( r*values[0] + g*values[1] + b*values[2] + a*values[3] + a04 ); // CLAMP includes rounding!
                        out_data[i+1] = CLAMP_D_TO_U8( r*values[5] + g*values[6] + b*values[7] + a*values[8] + a14 );
                        out_data[i+2] = CLAMP_D_TO_U8( r*values[10] + g*values[11] + b*values[12] + a*values[13] + a24 );
                        out_data[i+3] = CLAMP_D_TO_U8( r*values[15] + g*values[16] + b*values[17] + a*values[18] + a34 );
                    }
                }
            }
            break;
        case COLORMATRIX_SATURATE:
            {
                double v = std::max(0.0,std::min(1.0,value)); // The standard says it should be between 0 and 1, and clamping it here makes it unnecessary to clamp the color values.
                double a00 = 0.213+0.787*v, a01 = 0.715-0.715*v, a02 = 0.072-0.072*v;
                double a10 = 0.213-0.213*v, a11 = 0.715+0.285*v, a12 = 0.072-0.072*v;
                double a20 = 0.213-0.213*v, a21 = 0.715-0.715*v, a22 = 0.072+0.928*v;
                for (x=x0;x<x1;x++){
                    for (y=y0;y<y1;y++){
                        i = ((x-x0) + (x1-x0)*(y-y0))*4;
                        r = in_data[i];
                        g = in_data[i+1];
                        b = in_data[i+2];
                        a = in_data[i+3];
                        out_data[i]   = static_cast<unsigned char>( r*a00 + g*a01 + b*a02 + .5 );
                        out_data[i+1] = static_cast<unsigned char>( r*a10 + g*a11 + b*a12 + .5 );
                        out_data[i+2] = static_cast<unsigned char>( r*a20 + g*a21 + b*a22 + .5 );
                        out_data[i+3] = a;
                    }
                }
            }
            break;
        case COLORMATRIX_HUEROTATE:
            {
                double coshue = cos(value * M_PI/180.0);
                double sinhue = sin(value * M_PI/180.0);
                double a00 = 0.213 + coshue*( 0.787) + sinhue*(-0.213);
                double a01 = 0.715 + coshue*(-0.715) + sinhue*(-0.715);
                double a02 = 0.072 + coshue*(-0.072) + sinhue*( 0.928);
                double a10 = 0.213 + coshue*(-0.213) + sinhue*( 0.143);
                double a11 = 0.715 + coshue*( 0.285) + sinhue*( 0.140);
                double a12 = 0.072 + coshue*(-0.072) + sinhue*(-0.283);
                double a20 = 0.213 + coshue*(-0.213) + sinhue*(-0.787);
                double a21 = 0.715 + coshue*(-0.715) + sinhue*( 0.715);
                double a22 = 0.072 + coshue*( 0.928) + sinhue*( 0.072);
                if (in->mode==NR_PIXBLOCK_MODE_R8G8B8A8P) {
                    // Although it does not change the alpha channel, it can give "out-of-bound" results, and in this case the bound is determined by the alpha channel
                    for (x=x0;x<x1;x++){
                        for (y=y0;y<y1;y++){
                            i = ((x-x0) + (x1-x0)*(y-y0))*4;
                            r = in_data[i];
                            g = in_data[i+1];
                            b = in_data[i+2];
                            a = in_data[i+3];

                            out_data[i]   = static_cast<unsigned char>(std::max(0.0,std::min((double)a, r*a00 + g*a01 + b*a02 + .5 )));
                            out_data[i+1] = static_cast<unsigned char>(std::max(0.0,std::min((double)a, r*a10 + g*a11 + b*a12 + .5 )));
                            out_data[i+2] = static_cast<unsigned char>(std::max(0.0,std::min((double)a, r*a20 + g*a21 + b*a22 + .5 )));
                            out_data[i+3] = a;
                        }
                    }
                } else {
                    for (x=x0;x<x1;x++){
                        for (y=y0;y<y1;y++){
                            i = ((x-x0) + (x1-x0)*(y-y0))*4;
                            r = in_data[i];
                            g = in_data[i+1];
                            b = in_data[i+2];
                            a = in_data[i+3];

                            out_data[i]   = CLAMP_D_TO_U8( r*a00 + g*a01 + b*a02);
                            out_data[i+1] = CLAMP_D_TO_U8( r*a10 + g*a11 + b*a12);
                            out_data[i+2] = CLAMP_D_TO_U8( r*a20 + g*a21 + b*a22);
                            out_data[i+3] = a;
                        }
                    }
                }
            }
            break;
        case COLORMATRIX_LUMINANCETOALPHA:
            for (x=x0;x<x1;x++){
                for (y=y0;y<y1;y++){
                    i = ((x-x0) + (x1-x0)*(y-y0))*4;
                    r = in_data[i];
                    g = in_data[i+1];
                    b = in_data[i+2];
                    out_data[i] = 0;
                    out_data[i+1] = 0;
                    out_data[i+2] = 0;
                    out_data[i+3] = static_cast<unsigned char>( r*0.2125 + g*0.7154 + b*0.0721 + .5 );
                }
            }
            break;
        case COLORMATRIX_ENDTYPE:
            break;
    }

    if (free_in_on_exit) {
        nr_pixblock_release(in);
        delete in;
    }

    out->empty = FALSE;
    slot.set(_output, out);
    return 0;
}
#endif

struct ColorMatrixMatrix {
    ColorMatrixMatrix(std::vector<double> const &values) {
        unsigned limit = std::min(20ul, values.size());
        for (unsigned i = 0; i < limit; ++i) {
            if (i % 5 == 4) {
                _v[i] = round(values[i]*255*255);
            } else {
                _v[i] = round(values[i]*255);
            }
        }
        for (unsigned i = limit; i < 20; ++i) {
            _v[i] = 0;
        }
    }

    static inline guint32 premul_alpha(guint32 color, guint32 alpha)
    {
        guint32 temp = alpha * color + 128;
        return (temp + (temp >> 8)) >> 8;
    }

    guint32 operator()(guint32 in) {
        EXTRACT_ARGB32(in, a, r, g, b)
        // we need to un-premultiply alpha values for this type of matrix
        // TODO: unpremul can be ignored if there is an identity mapping on the alpha channel
        if (a != 0) {
            r = (r * 255 + a/2) / a;
            b = (b * 255 + a/2) / a;
            g = (g * 255 + a/2) / a;
        }

        gint32 ro = r*_v[0]  + g*_v[1]  + b*_v[2]  + a*_v[3]  + _v[4];
        gint32 go = r*_v[5]  + g*_v[6]  + b*_v[7]  + a*_v[8]  + _v[9];
        gint32 bo = r*_v[10] + g*_v[11] + b*_v[12] + a*_v[13] + _v[14];
        gint32 ao = r*_v[15] + g*_v[16] + b*_v[17] + a*_v[18] + _v[19];
        ro = (pxclamp(ro, 0, 255*255) + 127) / 255;
        go = (pxclamp(go, 0, 255*255) + 127) / 255;
        bo = (pxclamp(bo, 0, 255*255) + 127) / 255;
        ao = (pxclamp(ao, 0, 255*255) + 127) / 255;

        ro = premul_alpha(ro, ao);
        go = premul_alpha(go, ao);
        bo = premul_alpha(bo, ao);

        ASSEMBLE_ARGB32(pxout, ao, ro, go, bo)
        return pxout;
    }
private:
    gint32 _v[20];
};

struct ColorMatrixSaturate {
    ColorMatrixSaturate(double v_in) {
        // clamp parameter instead of clamping color values
        double v = CLAMP(v_in, 0.0, 1.0);
        _v[0] = 0.213+0.787*v; _v[1] = 0.715-0.715*v; _v[2] = 0.072-0.072*v;
        _v[3] = 0.213-0.213*v; _v[4] = 0.715+0.285*v; _v[5] = 0.072-0.072*v;
        _v[6] = 0.213-0.213*v; _v[7] = 0.715-0.715*v; _v[8] = 0.072+0.928*v;
    }

    guint32 operator()(guint32 in) {
        EXTRACT_ARGB32(in, a, r, g, b)

        // Note: this cannot be done in fixed point, because the loss of precision
        //       causes overflow for some values of v
        guint32 ro = r*_v[0] + g*_v[1] + b*_v[2] + 0.5;
        guint32 go = r*_v[3] + g*_v[4] + b*_v[5] + 0.5;
        guint32 bo = r*_v[6] + g*_v[7] + b*_v[8] + 0.5;

        ASSEMBLE_ARGB32(pxout, a, ro, go, bo)
        return pxout;
    }
private:
    double _v[9];
};

struct ColorMatrixHueRotate {
    ColorMatrixHueRotate(double v) {
        double sinhue, coshue;
        sincos(v * M_PI/180.0, &sinhue, &coshue);

        _v[0] = round((0.213 +0.787*coshue -0.213*sinhue)*255);
        _v[1] = round((0.715 -0.715*coshue -0.715*sinhue)*255);
        _v[2] = round((0.072 -0.072*coshue +0.928*sinhue)*255);

        _v[3] = round((0.213 -0.213*coshue +0.143*sinhue)*255);
        _v[4] = round((0.715 +0.285*coshue +0.140*sinhue)*255);
        _v[5] = round((0.072 -0.072*coshue -0.283*sinhue)*255);

        _v[6] = round((0.213 -0.213*coshue -0.787*sinhue)*255);
        _v[7] = round((0.715 -0.715*coshue +0.715*sinhue)*255);
        _v[8] = round((0.072 +0.928*coshue +0.072*sinhue)*255);
    }
    guint32 operator()(guint32 in) {
        EXTRACT_ARGB32(in, a, r, g, b)
        gint32 maxpx = a*255;
        gint32 ro = r*_v[0] + g*_v[1] + b*_v[2];
        gint32 go = r*_v[3] + g*_v[4] + b*_v[5];
        gint32 bo = r*_v[6] + g*_v[7] + b*_v[8];
        ro = (pxclamp(ro, 0, maxpx) + 127) / 255;
        go = (pxclamp(go, 0, maxpx) + 127) / 255;
        bo = (pxclamp(bo, 0, maxpx) + 127) / 255;

        ASSEMBLE_ARGB32(pxout, a, ro, go, bo)
        return pxout;
    }
private:
    gint32 _v[9];
};

struct ColorMatrixLuminanceToAlpha {
    guint32 operator()(guint32 in) {
        // original computation in double: r*0.2125 + g*0.7154 + b*0.0721
        EXTRACT_ARGB32(in, a, r, g, b)
        // unpremultiply color values
        if (a != 0) {
            r = (r * 255 + a/2) / a;
            b = (b * 255 + a/2) / a;
            g = (g * 255 + a/2) / a;
        }
        guint32 ao = r*54 + g*182 + b*18;
        return ((ao + 127) / 255) << 24;
    }
};

void FilterColorMatrix::render_cairo(FilterSlot &slot)
{
    cairo_surface_t *input = slot.getcairo(_input);
    cairo_surface_t *out = NULL;
    if (type == COLORMATRIX_LUMINANCETOALPHA) {
        out = ink_cairo_surface_create_same_size(input, CAIRO_CONTENT_ALPHA);
    } else {
        out = ink_cairo_surface_create_identical(input);
    }

    switch (type) {
    case COLORMATRIX_MATRIX:
        ink_cairo_surface_filter(input, out, ColorMatrixMatrix(values));
        break;
    case COLORMATRIX_SATURATE:
        ink_cairo_surface_filter(input, out, ColorMatrixSaturate(value));
        break;
    case COLORMATRIX_HUEROTATE:
        ink_cairo_surface_filter(input, out, ColorMatrixHueRotate(value));
        break;
    case COLORMATRIX_LUMINANCETOALPHA:
        ink_cairo_surface_filter(input, out, ColorMatrixLuminanceToAlpha());
        break;
    case COLORMATRIX_ENDTYPE:
    default:
        break;
    }

    slot.set(_output, out);
    cairo_surface_destroy(out);
}

bool FilterColorMatrix::can_handle_affine(Geom::Matrix const &)
{
    return true;
}

void FilterColorMatrix::area_enlarge(NRRectL &/*area*/, Geom::Matrix const &/*trans*/)
{
}

void FilterColorMatrix::set_type(FilterColorMatrixType t){
        type = t;
}

void FilterColorMatrix::set_value(gdouble v){
        value = v;
}

void FilterColorMatrix::set_values(std::vector<gdouble> const &v){
        values = v;
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
