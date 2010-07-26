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

enum MorphologyOp {
    ERODE,
    DILATE
};

namespace {

template <MorphologyOp oper> guint32 extreme(guint32 a, guint32 b);
template <> guint32 extreme<ERODE>(guint32 a, guint32 b) { return std::min(a, b); }
template <> guint32 extreme<DILATE>(guint32 a, guint32 b) { return std::max(a, b); }

/* This performs one "half" of the morphology operation by calculating 
 * the componentwise extreme in the specified axis with the given radius.
 * Performing the operation one axis at a time gives us a MASSIVE performance boost
 * at large morphology radii. We can do this, because the morphology operation
 * is separable just like Gaussian blur. */
template <MorphologyOp OP, Geom::Dim2 axis>
struct Morphology : public SurfaceSynth {
    Morphology(cairo_surface_t *in, double xradius)
        : SurfaceSynth(in)
        , _radius(round(xradius))
    {}
    guint32 operator()(int x, int y) {
        int start, end;
        if (axis == Geom::X) {
            start = std::max(0, x - _radius);
            end = std::min(x + _radius + 1, _w);
        } else {
            start = std::max(0, y - _radius);
            end = std::min(y + _radius + 1, _h);
        }

        guint32 ao = (OP == DILATE ? 0 : 255);
        guint32 ro = (OP == DILATE ? 0 : 255);
        guint32 go = (OP == DILATE ? 0 : 255);
        guint32 bo = (OP == DILATE ? 0 : 255);

        if (_alpha) {
            ao = (OP == DILATE ? 0 : 0xff000000);
            for (int i = start; i < end; ++i) {
                guint32 px = (axis == Geom::X ? pixelAt(i, y) : pixelAt(x, i));
                ao = extreme<OP>(ao, px & 0xff000000);
            }
            return ao;
        } else {
            for (int i = start; i < end; ++i) {
                guint32 px = (axis == Geom::X ? pixelAt(i, y) : pixelAt(x, i));
                EXTRACT_ARGB32(px, a,r,g,b);
                if (a) {
                    r = unpremul_alpha(r, a);
                    g = unpremul_alpha(g, a);
                    b = unpremul_alpha(b, a);

                    ao = extreme<OP>(ao, a);
                    ro = extreme<OP>(ro, r);
                    go = extreme<OP>(go, g);
                    bo = extreme<OP>(bo, b);
                } else {
                    if (OP == DILATE) {
                        continue; // zero pixel will not affect the maximum
                    } else {
                        // zero pixel is guaranteed to be the minimum
                        ao = 0; ro = 0; go = 0; bo = 0;
                        break;
                    }
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
    int _radius;
};

} // end anonymous namespace

void FilterMorphology::render_cairo(FilterSlot &slot)
{
    cairo_surface_t *input = slot.getcairo(_input);

    Geom::Matrix p2pb = slot.get_units().get_matrix_primitiveunits2pb();
    double xr = xradius * p2pb.expansionX();
    double yr = yradius * p2pb.expansionY();

    cairo_surface_t *interm = ink_cairo_surface_create_identical(input);

    if (Operator == MORPHOLOGY_OPERATOR_DILATE) {
        ink_cairo_surface_synthesize(interm, Morphology<DILATE, Geom::X>(input, xr));
    } else {
        ink_cairo_surface_synthesize(interm, Morphology<ERODE, Geom::X>(input, xr));
    }

    cairo_surface_t *out = ink_cairo_surface_create_identical(input);

    if (Operator == MORPHOLOGY_OPERATOR_DILATE) {
        ink_cairo_surface_synthesize(out, Morphology<DILATE, Geom::Y>(interm, yr));
    } else {
        ink_cairo_surface_synthesize(out, Morphology<ERODE, Geom::Y>(interm, yr));
    }

    cairo_surface_destroy(interm);

    slot.set(_output, out);
    cairo_surface_destroy(out);
}

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
