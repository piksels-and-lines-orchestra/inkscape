/**
 * @file
 * @brief Group belonging to an SVG drawing element
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "display/cairo-utils.h"
#include "display/drawing-context.h"
#include "display/drawing-item.h"
#include "display/drawing-group.h"
#include "libnr/nr-values.h"
#include "nr-arena.h"
#include "style.h"

namespace Inkscape {

DrawingGroup::DrawingGroup(Drawing *drawing)
    : DrawingItem(drawing)
    , _style(NULL)
    , _child_transform(NULL)
{}

DrawingGroup::~DrawingGroup()
{
    if (_style)
        sp_style_unref(_style);
}

void
DrawingGroup::setPickChildren(bool p)
{
    _pick_children = p;
}

void
DrawingGroup::setStyle(SPStyle *style)
{
    _setStyleCommon(_style, style);
}

void
DrawingGroup::setChildTransform(Geom::Affine const &new_trans)
{
    Geom::Affine current;
    if (_child_transform) {
        current = *_child_transform;
    }
    
    if (!Geom::are_near(current, new_trans, NR_EPSILON)) {
        // mark the area where the object was for redraw.
        _markForRendering();
        if (new_trans.isIdentity()) {
            delete _child_transform; // delete NULL; is safe
            _child_transform = NULL;
        } else {
            _child_transform = new Geom::Affine(new_trans);
        }
        _markForUpdate(STATE_ALL, true);
    }
}

unsigned
DrawingGroup::_updateItem(Geom::IntRect const &area, UpdateContext const &ctx, unsigned flags, unsigned reset)
{
    unsigned beststate = STATE_ALL;
    bool outline = (_drawing->rendermode == RENDERMODE_OUTLINE);

    for (ChildrenList::iterator i = _children.begin(); i != _children.end(); ++i) {
        UpdateContext child_ctx(ctx);
        if (_child_transform) {
            child_ctx.ctm = *_child_transform * ctx.ctm;
        }
        i->update(area, child_ctx, flags, reset);
    }
    if (beststate & STATE_BBOX) {
        _bbox = Geom::OptIntRect();
        for (ChildrenList::iterator i = _children.begin(); i != _children.end(); ++i) {
            if (i->visible()) {
                _bbox.unionWith(outline ? i->geometricBounds() : i->visualBounds());
            }
        }
    }
    return beststate;
}

void
DrawingGroup::_renderItem(DrawingContext &ct, Geom::IntRect const &area, unsigned flags)
{
    for (ChildrenList::iterator i = _children.begin(); i != _children.end(); ++i) {
        i->render(ct, area, flags);
    }
}

void
DrawingGroup::_clipItem(DrawingContext &ct, Geom::IntRect const &area)
{
    for (ChildrenList::iterator i = _children.begin(); i != _children.end(); ++i) {
        i->clip(ct, area);
    }
}

DrawingItem *
DrawingGroup::_pickItem(Geom::Point const &p, double delta)
{
    for (ChildrenList::iterator i = _children.begin(); i != _children.end(); ++i) {
        DrawingItem *picked = i->pick(p, delta, false);
        if (picked) {
            return _pick_children ? picked : this;
        }
    }
    return NULL;
}

bool
DrawingGroup::_canClip()
{
    return true;
}

bool is_drawing_group(DrawingItem *item)
{
    return dynamic_cast<DrawingGroup *>(item) != NULL;
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
