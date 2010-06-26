/*
 * Helper functions to use cairo with inkscape
 *
 * Copyright (C) 2007 bulia byak
 * Copyright (C) 2008 Johan Engelen
 *
 * Released under GNU GPL
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdexcept>
#include <cairo.h>
#include <2geom/matrix.h>
#include "display/cairo-utils.h"
#include "display/inkscape-cairo.h"
#include "color.h"

namespace Inkscape {

CairoGroup::CairoGroup(cairo_t *_ct) : ct(_ct), pushed(false) {}
CairoGroup::~CairoGroup() {
    if (pushed) {
        cairo_pattern_t *p = cairo_pop_group(ct);
        cairo_pattern_destroy(p);
    }
}
void CairoGroup::push() {
    cairo_push_group(ct);
    pushed = true;
}
void CairoGroup::push_with_content(cairo_content_t content) {
    cairo_push_group_with_content(ct, content);
    pushed = true;
}
cairo_pattern_t *CairoGroup::pop() {
    if (pushed) {
        cairo_pattern_t *ret = cairo_pop_group(ct);
        pushed = false;
        return ret;
    } else {
        throw std::logic_error("Cairo group popped without pushing it first");
    }
}
Cairo::RefPtr<Cairo::Pattern> CairoGroup::popmm() {
    if (pushed) {
        cairo_pattern_t *ret = cairo_pop_group(ct);
        Cairo::RefPtr<Cairo::Pattern> retmm(new Cairo::Pattern(ret, true));
        pushed = false;
        return retmm;
    } else {
        throw std::logic_error("Cairo group popped without pushing it first");
    }
}
void CairoGroup::pop_to_source() {
    if (pushed) {
        cairo_pop_group_to_source(ct);
        pushed = false;
    }
}

CairoContext::CairoContext(cairo_t *obj, bool ref)
    : Cairo::Context(obj, ref)
{}

void CairoContext::transform(Geom::Matrix const &m)
{
    cairo_matrix_t cm;
    cm.xx = m[0];
    cm.xy = m[2];
    cm.x0 = m[4];
    cm.yx = m[1];
    cm.yy = m[3];
    cm.y0 = m[5];
    cairo_transform(cobj(), &cm);
}

void CairoContext::set_source_rgba32(guint32 color)
{
    double red = SP_RGBA32_R_F(color);
    double gre = SP_RGBA32_G_F(color);
    double blu = SP_RGBA32_B_F(color);
    double alp = SP_RGBA32_A_F(color);
    cairo_set_source_rgba(cobj(), red, gre, blu, alp);
}

void CairoContext::append_path(Geom::PathVector const &pv)
{
    feed_pathvector_to_cairo(cobj(), pv);
}

Cairo::RefPtr<CairoContext> CairoContext::create(Cairo::RefPtr<Cairo::Surface> const &target)
{
    cairo_t *ct = cairo_create(target->cobj());
    Cairo::RefPtr<CairoContext> ret(new CairoContext(ct, true));
    return ret;
}

} // namespace Inkscape

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
