/*
 * feMorphology filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <cmath>
#include <algorithm>
#include "display/cairo-templates.h"
#include "display/cairo-utils.h"
#include "display/nr-filter-morphology.h"
#include "display/nr-filter-slot.h"
#include "display/nr-filter-units.h"

namespace Inkscape {
namespace Filters {

FilterMorphology::FilterMorphology()
{
}

FilterPrimitive * FilterMorphology::create() {
    return new FilterMorphology();
}

FilterMorphology::~FilterMorphology()
{}

struct MorphologyErode : public SurfaceSynth {
    MorphologyErode(cairo_surface_t *in, double xradius, double yradius)
        : SurfaceSynth(in)
        , _xr(round(xradius))
        , _yr(round(yradius))
    {}
    guint32 operator()(int x, int y) {
        int startx = std::max(x - _xr, 0), endx = std::min(x + _xr + 1, _w);
        int starty = std::max(y - _yr, 0), endy = std::min(y + _yr + 1, _h);

        guint32 ao = 255;
        guint32 ro = 255;
        guint32 go = 255;
        guint32 bo = 255;

        if (_alpha) {
            ao = 0xff000000;
            for (int i = starty; i < endy; ++i) {
                for (int j = startx; j < endx; ++j) {
                    guint32 px = pixelAt(j, i);
                    ao = std::min(ao, px & 0xff000000);
                }
            }
            return ao;
        } else {
            for (int i = starty; i < endy; ++i) {
                for (int j = startx; j < endx; ++j) {
                    guint32 px = pixelAt(j, i);
                    EXTRACT_ARGB32(px, a,r,g,b);
                    if (a) {
                        r = unpremul_alpha(r, a);
                        g = unpremul_alpha(g, a);
                        b = unpremul_alpha(b, a);
                        ao = std::min(ao, a);
                        ro = std::min(ro, r);
                        go = std::min(go, g);
                        bo = std::min(bo, b);
                    } else {
                        // zero pixel is guaranteed to be the minimum
                        ao = 0; ro = 0; go = 0; bo = 0;
                        goto end_loop;
                    }
                }
            }
            end_loop:

            ro = premul_alpha(ro, ao);
            go = premul_alpha(go, ao);
            bo = premul_alpha(bo, ao);
            ASSEMBLE_ARGB32(pxout, ao,ro,go,bo)
            return pxout;
        }
    }
private:
    int _xr, _yr;
};

struct MorphologyDilate : public SurfaceSynth {
    MorphologyDilate(cairo_surface_t *in, double xradius, double yradius)
        : SurfaceSynth(in)
        , _xr(round(xradius))
        , _yr(round(yradius))
    {}
    guint32 operator()(int x, int y) {
        int startx = std::max(x - _xr, 0), endx = std::min(x + _xr + 1, _w);
        int starty = std::max(y - _yr, 0), endy = std::min(y + _yr + 1, _h);

        guint32 ao = 0;
        guint32 ro = 0;
        guint32 go = 0;
        guint32 bo = 0;

        if (_alpha) {
            for (int i = starty; i < endy; ++i) {
                for (int j = startx; j < endx; ++j) {
                    guint32 px = pixelAt(j, i);
                    ao = std::max(ao, px & 0xff000000);
                }
            }
            return ao;
        } else {
            for (int i = starty; i < endy; ++i) {
                for (int j = startx; j < endx; ++j) {
                    guint32 px = pixelAt(j, i);
                    EXTRACT_ARGB32(px, a,r,g,b)
                    if (a == 0) continue; // this cannot affect the maximum

                    r = unpremul_alpha(r, a);
                    g = unpremul_alpha(g, a);
                    b = unpremul_alpha(b, a);
                    ao = std::max(ao, a);
                    ro = std::max(ro, r);
                    go = std::max(go, g);
                    bo = std::max(bo, b);
                }
            }

            ro = premul_alpha(ro, ao);
            go = premul_alpha(go, ao);
            bo = premul_alpha(bo, ao);
            ASSEMBLE_ARGB32(pxout, ao,ro,go,bo)
            return pxout;
        }
    }
private:
    int _xr, _yr;
};

void FilterMorphology::render_cairo(FilterSlot &slot)
{
    cairo_surface_t *input = slot.getcairo(_input);
    cairo_surface_t *out = ink_cairo_surface_create_identical(input);

    Geom::Matrix p2pb = slot.get_units().get_matrix_primitiveunits2pb();
    double xr = xradius * p2pb.expansionX();
    double yr = yradius * p2pb.expansionY();

    if (Operator == MORPHOLOGY_OPERATOR_DILATE) {
        ink_cairo_surface_synthesize(out, MorphologyDilate(input, xr, yr));
    } else {
        ink_cairo_surface_synthesize(out, MorphologyErode(input, xr, yr));
    }

    slot.set(_output, out);
    cairo_surface_destroy(out);
}

/*
int FilterMorphology::render(FilterSlot &slot, FilterUnits const &units) {
    NRPixBlock *in = slot.get(_input);
    if (!in) {
        g_warning("Missing source image for feMorphology (in=%d)", _input);
        return 1;
    }

    NRPixBlock *out = new NRPixBlock;

    // this primitive is defined for premultiplied RGBA values,
    // thus convert them to that format
    bool free_in_on_exit = false;
    if (in->mode != NR_PIXBLOCK_MODE_R8G8B8A8P) {
        NRPixBlock *original_in = in;
        in = new NRPixBlock;
        nr_pixblock_setup_fast(in, NR_PIXBLOCK_MODE_R8G8B8A8P,
                               original_in->area.x0, original_in->area.y0,
                               original_in->area.x1, original_in->area.y1,
                               true);
        nr_blit_pixblock_pixblock(in, original_in);
        free_in_on_exit = true;
    }

    Geom::Matrix p2pb = units.get_matrix_primitiveunits2pb();
    int const xradius = (int)round(this->xradius * p2pb.expansionX());
    int const yradius = (int)round(this->yradius * p2pb.expansionY());

    int x0=in->area.x0;
    int y0=in->area.y0;
    int x1=in->area.x1;
    int y1=in->area.y1;
    int w=x1-x0, h=y1-y0;
    int x, y, i, j;
    int rmax,gmax,bmax,amax;
    int rmin,gmin,bmin,amin;

    nr_pixblock_setup_fast(out, in->mode, x0, y0, x1, y1, true);

    unsigned char *in_data = NR_PIXBLOCK_PX(in);
    unsigned char *out_data = NR_PIXBLOCK_PX(out);

    for(x = 0 ; x < w ; x++){
        for(y = 0 ; y < h ; y++){
            rmin = gmin = bmin = amin = 255;
            rmax = gmax = bmax = amax = 0;
            for(i = x - xradius ; i < x + xradius ; i++){
                if (i < 0 || i >= w) continue;
                for(j = y - yradius ; j < y + yradius ; j++){
                    if (j < 0 || j >= h) continue;
                    if(in_data[4*(i + w*j)]>rmax) rmax = in_data[4*(i + w*j)];
                    if(in_data[4*(i + w*j)+1]>gmax) gmax = in_data[4*(i + w*j)+1];
                    if(in_data[4*(i + w*j)+2]>bmax) bmax = in_data[4*(i + w*j)+2];
                    if(in_data[4*(i + w*j)+3]>amax) amax = in_data[4*(i + w*j)+3];

                    if(in_data[4*(i + w*j)]<rmin) rmin = in_data[4*(i + w*j)];
                    if(in_data[4*(i + w*j)+1]<gmin) gmin = in_data[4*(i + w*j)+1];
                    if(in_data[4*(i + w*j)+2]<bmin) bmin = in_data[4*(i + w*j)+2];
                    if(in_data[4*(i + w*j)+3]<amin) amin = in_data[4*(i + w*j)+3];
                }
            }
            if (Operator==MORPHOLOGY_OPERATOR_DILATE){
                out_data[4*(x + w*y)]=rmax;
                out_data[4*(x + w*y)+1]=gmax;
                out_data[4*(x + w*y)+2]=bmax;
                out_data[4*(x + w*y)+3]=amax;
            } else {
                out_data[4*(x + w*y)]=rmin;
                out_data[4*(x + w*y)+1]=gmin;
                out_data[4*(x + w*y)+2]=bmin;
                out_data[4*(x + w*y)+3]=amin;
            }
        }
    }

    if (free_in_on_exit) {
        nr_pixblock_release(in);
        delete in;
    }

    out->empty = FALSE;
    slot.set(_output, out);
    return 0;
}*/

void FilterMorphology::area_enlarge(NRRectL &area, Geom::Matrix const &trans)
{
    int const enlarge_x = (int)std::ceil(this->xradius * (std::fabs(trans[0]) + std::fabs(trans[1])));
    int const enlarge_y = (int)std::ceil(this->yradius * (std::fabs(trans[2]) + std::fabs(trans[3])));

    area.x0 -= enlarge_x;
    area.x1 += enlarge_x;
    area.y0 -= enlarge_y;
    area.y1 += enlarge_y;
}

void FilterMorphology::set_operator(FilterMorphologyOperator &o){
    Operator = o;
}

void FilterMorphology::set_xradius(double x){
    xradius = x;
}

void FilterMorphology::set_yradius(double y){
    yradius = y;
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
