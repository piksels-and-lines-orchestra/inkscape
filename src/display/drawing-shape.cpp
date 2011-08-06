/**
 * @file
 * @brief Shape (styled path) belonging to an SVG drawing
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib.h>
#include <2geom/curves.h>
#include <2geom/pathvector.h>
#include <2geom/svg-path.h>
#include <2geom/svg-path-parser.h>

#include "display/cairo-utils.h"
#include "display/canvas-arena.h"
#include "display/canvas-bpath.h"
#include "display/curve.h"
#include "display/drawing-context.h"
#include "display/drawing-group.h"
#include "display/drawing-shape.h"
#include "display/nr-arena.h"
#include "helper/geom-curves.h"
#include "helper/geom.h"
#include "libnr/nr-convert2geom.h"
#include "preferences.h"
#include "style.h"
#include "svg/svg.h"

namespace Inkscape {

DrawingShape::DrawingShape(Drawing *drawing)
    : DrawingItem(drawing)
    , _curve(NULL)
    , _style(NULL)
    , _last_pick(NULL)
    , _repick_after(0)
{}

DrawingShape::~DrawingShape()
{
    if (_style)
        sp_style_unref(_style);
    if (_curve)
        _curve->unref();
}

void
DrawingShape::setPath(SPCurve *curve)
{
    _markForRendering();

    if (_curve) {
        _curve->unref();
        _curve = NULL;
    }
    if (curve) {
        _curve = curve;
        curve->ref();
    }

    _markForUpdate(STATE_ALL, false);
}

void
DrawingShape::setStyle(SPStyle *style)
{
    _setStyleCommon(_style, style);
    _nrstyle.set(style);
}

void
DrawingShape::setPaintBox(Geom::Rect const &box)
{
    _paintbox = box;
    _markForUpdate(STATE_ALL, false);
}

unsigned
DrawingShape::_updateItem(Geom::IntRect const &area, UpdateContext const &ctx, unsigned flags, unsigned reset)
{
    Geom::OptRect boundingbox;

    unsigned beststate = STATE_ALL;

    // update markers
    for (ChildrenList::iterator i = _children.begin(); i != _children.end(); ++i) {
        i->update(area, ctx, flags, reset);
    }

    if (!(flags & STATE_RENDER)) {
        /* We do not have to create rendering structures */
        if (flags & STATE_BBOX) {
            if (_curve) {
                boundingbox = bounds_exact_transformed(_curve->get_pathvector(), ctx.ctm);
                if (boundingbox) {
                    _bbox = boundingbox->roundOutwards();
                } else {
                    _bbox = Geom::OptIntRect();
                }
            }
            if (beststate & STATE_BBOX) {
                for (ChildrenList::iterator i = _children.begin(); i != _children.end(); ++i) {
                    _bbox.unionWith(i->geometricBounds());
                }
            }
        }
        return (flags | _state);
    }

    boundingbox = Geom::OptRect();
    bool outline = (_drawing->rendermode == RENDERMODE_OUTLINE);

    // clear Cairo data to force update
    _nrstyle.update();

    if (_curve) {
        boundingbox = bounds_exact_transformed(_curve->get_pathvector(), ctx.ctm);

        if (boundingbox && (_nrstyle.stroke.type != NRStyle::PAINT_NONE || outline)) {
            float width, scale;
            scale = ctx.ctm.descrim();
            width = std::max(0.125f, _nrstyle.stroke_width * scale);
            if ( fabs(_nrstyle.stroke_width * scale) > 0.01 ) { // FIXME: this is always true
                boundingbox->expandBy(width);
            }
            // those pesky miters, now
            float miterMax = width * _nrstyle.miter_limit;
            if ( miterMax > 0.01 ) {
                // grunt mode. we should compute the various miters instead
                // (one for each point on the curve)
                boundingbox->expandBy(miterMax);
            }
        }
    }

    _bbox = boundingbox ? boundingbox->roundOutwards() : Geom::OptIntRect();

    if (!_curve || 
        !_style ||
        _curve->is_empty() ||
        (( _nrstyle.fill.type != NRStyle::PAINT_NONE ) &&
         ( _nrstyle.stroke.type != NRStyle::PAINT_NONE && !outline) ))
    {
        return STATE_ALL;
    }

    if (beststate & STATE_BBOX) {
        for (ChildrenList::iterator i = _children.begin(); i != _children.end(); ++i) {
            _bbox.unionWith(i->geometricBounds());
        }
    }

    return STATE_ALL;
}

void
DrawingShape::_renderItem(DrawingContext &ct, Geom::IntRect const &area, unsigned flags)
{
    if (!_curve || !_style) return;
    if (!area.intersects(_bbox)) return; // skip if not within bounding box

    bool outline = (_drawing->rendermode == RENDERMODE_OUTLINE);

    if (outline) {
        guint32 rgba = _drawing->outlinecolor;

        {   Inkscape::DrawingContext::Save save(ct);
            ct.transform(_ctm);
            ct.path(_curve->get_pathvector());
        }
        {   Inkscape::DrawingContext::Save save(ct);
            ct.setSource(rgba);
            ct.setLineWidth(0.5);
            ct.setTolerance(1.25);
            ct.stroke();
        }
    } else {
        bool has_stroke, has_fill;
        // we assume the context has no path
        Inkscape::DrawingContext::Save save(ct);
        ct.transform(_ctm);

        // update fill and stroke paints.
        // this cannot be done during nr_arena_shape_update, because we need a Cairo context
        // to render svg:pattern
        has_fill   = _nrstyle.prepareFill(ct, _paintbox);
        has_stroke = _nrstyle.prepareStroke(ct, _paintbox);
        has_stroke &= (_nrstyle.stroke_width != 0);

        if (has_fill || has_stroke) {
            // TODO: remove segments outside of bbox when no dashes present
            ct.path(_curve->get_pathvector());
            if (has_fill) {
                _nrstyle.applyFill(ct);
                ct.fillPreserve();
            }
            if (has_stroke) {
                _nrstyle.applyStroke(ct);
                ct.strokePreserve();
            }
            ct.newPath(); // clear path
        } // has fill or stroke pattern
    }

    // marker rendering
    for (ChildrenList::iterator i = _children.begin(); i != _children.end(); ++i) {
        i->render(ct, area, flags);
    }
}

void
DrawingShape::_clipItem(DrawingContext &ct, Geom::IntRect const &area)
{
    if (!_curve) return;

    Inkscape::DrawingContext::Save save(ct);
    // handle clip-rule
    if (_style) {
        if (_style->clip_rule.computed == SP_WIND_RULE_EVENODD) {
            ct.setFillRule(CAIRO_FILL_RULE_EVEN_ODD);
        } else {
            ct.setFillRule(CAIRO_FILL_RULE_WINDING);
        }
    }
    ct.transform(_ctm);
    ct.path(_curve->get_pathvector());
    ct.fill();
}

DrawingItem *
DrawingShape::_pickItem(Geom::Point const &p, double delta, bool /*sticky*/)
{
    if (_repick_after > 0)
        --_repick_after;

    if (_repick_after > 0) // we are a slow, huge path
        return _last_pick; // skip this pick, returning what was returned last time

    if (!_curve) return NULL;
    if (!_style) return NULL;

    bool outline = (_drawing->rendermode == RENDERMODE_OUTLINE);

    if (SP_SCALE24_TO_FLOAT(_style->opacity.value) == 0 && !outline) 
        // fully transparent, no pick unless outline mode
        return NULL;

    GTimeVal tstart, tfinish;
    g_get_current_time (&tstart);

    double width;
    if (outline) {
        width = 0.5;
    } else if (_nrstyle.stroke.type != NRStyle::PAINT_NONE && _nrstyle.stroke.opacity > 1e-3) {
        float const scale = _ctm.descrim();
        width = std::max(0.125f, _nrstyle.stroke_width * scale) / 2;
    } else {
        width = 0;
    }

    double dist = Geom::infinity();
    int wind = 0;
    bool needfill = (_nrstyle.fill.type != NRStyle::PAINT_NONE 
             && _nrstyle.fill.opacity > 1e-3 && !outline);

    if (_drawing->canvasarena) {
        Geom::Rect viewbox = _drawing->canvasarena->item.canvas->getViewbox();
        viewbox.expandBy (width);
        pathv_matrix_point_bbox_wind_distance(_curve->get_pathvector(), _ctm, p, NULL, needfill? &wind : NULL, &dist, 0.5, &viewbox);
    } else {
        pathv_matrix_point_bbox_wind_distance(_curve->get_pathvector(), _ctm, p, NULL, needfill? &wind : NULL, &dist, 0.5, NULL);
    }

    g_get_current_time (&tfinish);
    glong this_pick = (tfinish.tv_sec - tstart.tv_sec) * 1000000 + (tfinish.tv_usec - tstart.tv_usec);
    //g_print ("pick time %lu\n", this_pick);

    if (this_pick > 10000) { // slow picking, remember to skip several new picks
        _repick_after = this_pick / 5000;
    }

    // covered by fill?
    if (needfill) {
        if (!_style->fill_rule.computed) {
            if (wind != 0) {
                _last_pick = this;
                return this;
            }
        } else {
            if (wind & 0x1) {
                _last_pick = this;
                return this;
            }
        }
    }

    // close to the edge, as defined by strokewidth and delta?
    // this ignores dashing (as if the stroke is solid) and always works as if caps are round
    if (needfill || width > 0) { // if either fill or stroke visible,
        if ((dist - width) < delta) {
            _last_pick = this;
            return this;
        }
    }

    // if not picked on the shape itself, try its markers
    for (ChildrenList::iterator i = _children.begin(); i != _children.end(); ++i) {
        DrawingItem *ret = i->pick(p, delta, false);
        if (ret) {
            _last_pick = this;
            return this;
        }
    }

    _last_pick = NULL;
    return NULL;
}

bool
DrawingShape::_canClip()
{
    return true;
}

} // end namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
