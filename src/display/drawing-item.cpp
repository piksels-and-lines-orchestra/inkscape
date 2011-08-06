/**
 * @file
 * @brief Canvas item belonging to an SVG drawing element
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "display/cairo-utils.h"
#include "display/cairo-templates.h"
#include "display/drawing-context.h"
#include "display/drawing-item.h"
#include "display/drawing-group.h"
#include "display/drawing-surface.h"
#include "nr-arena.h"
#include "nr-filter.h"
#include "preferences.h"
#include "style.h"

namespace Inkscape {

DrawingItem::DrawingItem(Drawing *drawing)
    : _drawing(drawing)
    , _parent(NULL)
    , _key(0)
    , _opacity(1.0)
    , _transform(NULL)
    , _clip(NULL)
    , _mask(NULL)
    , _filter(NULL)
    , _user_data(NULL)
    , _cache(NULL)
    , _state(0)
    , _visible(true)
    , _sensitive(true)
    , _cached(0)
    , _propagate(0)
//    , _renders_opacity(0)
    , _clip_child(0)
    , _mask_child(0)
    , _pick_children(0)
{
    nr_object_ref(_drawing);
}

DrawingItem::~DrawingItem()
{
    _drawing->item_deleted.emit(this);
    //if (!_children.empty()) {
    //    g_warning("Removing item with children");
    //}

    // remove from the set of cached items
    if (_cached) {
        _drawing->cached_items.erase(this);
    }
    // remove this item from parent's children list
    // due to the effect of clearChildren(), this only happens for the top-level deleted item
    if (_parent) {
        _markForRendering();
        // we cannot call setClip(NULL) or setMask(NULL),
        // because that would be an endless loop
        if (_clip_child) {
            _parent->_clip = NULL;
        } else if (_mask_child) {
            _parent->_mask = NULL;
        } else {
            ChildrenList::iterator ithis = _parent->_children.iterator_to(*this);
            _parent->_children.erase(ithis);
        }
        _parent->_markForUpdate(STATE_ALL, false);
    }
    clearChildren();
    delete _transform;
    delete _clip;
    delete _mask;
    delete _filter;
    nr_object_unref(_drawing);
}

DrawingItem *
DrawingItem::parent() const
{
    //if (_clip_child || _mask_child)
    //    return NULL;

    return _parent;
}

void
DrawingItem::appendChild(DrawingItem *item)
{
    item->_parent = this;
    _children.push_back(*item);
    _markForUpdate(STATE_ALL, false);
}

void
DrawingItem::prependChild(DrawingItem *item)
{
    item->_parent = this;
    _children.push_front(*item);
    _markForUpdate(STATE_ALL, false);
}

void
DrawingItem::clearChildren()
{
    // prevent children from referencing the parent during deletion
    // this way, children won't try to remove themselves from a list
    // from which they have already been removed by clear_and_dispose
    for (ChildrenList::iterator i = _children.begin(); i != _children.end(); ++i) {
        i->_parent = NULL;
    }
    _children.clear_and_dispose(DeleteDisposer());
}

void
DrawingItem::setTransform(Geom::Affine const &new_trans)
{
    Geom::Affine current;
    if (_transform) {
        current = *_transform;
    }
    
    if (!Geom::are_near(current, new_trans, NR_EPSILON)) {
        // mark the area where the object was for redraw.
        _markForRendering();
        if (new_trans.isIdentity()) {
            delete _transform; // delete NULL; is safe
            _transform = NULL;
        } else {
            _transform = new Geom::Affine(new_trans);
        }
        _markForUpdate(STATE_ALL, true);
    }
}

void
DrawingItem::setOpacity(float opacity)
{
    _opacity = opacity;
    _markForRendering();
}

void
DrawingItem::setVisible(bool v)
{
    _visible = v;
    _markForRendering();
}

void
DrawingItem::setSensitive(bool s)
{
    _sensitive = s;
}

void
DrawingItem::setCached(bool c)
{
    _cached = c;
    if (c) {
        _drawing->cached_items.insert(this);
    } else {
        _drawing->cached_items.erase(this);
    }
    _markForUpdate(STATE_CACHE, false);
}

void
DrawingItem::setClip(DrawingItem *item)
{
    _markForRendering();
    delete _clip;
    _clip = item;
    if (item) {
        item->_parent = this;
        item->_clip_child = true;
    }
    _markForUpdate(STATE_ALL, true);
}

void
DrawingItem::setMask(DrawingItem *item)
{
    _markForRendering();
    delete _mask;
    _mask = item;
        if (item) {
        item->_parent = this;
        item->_mask_child = true;
    }
    _markForUpdate(STATE_ALL, true);
}

void
DrawingItem::setZOrder(unsigned z)
{
    if (!_parent) return;

    ChildrenList::iterator it = _parent->_children.iterator_to(*this);
    _parent->_children.erase(it);

    ChildrenList::iterator i = _parent->_children.begin();
    std::advance(i, std::min(z, unsigned(_parent->_children.size())));
    _parent->_children.insert(i, *this);
    _markForRendering();
}

void
DrawingItem::setItemBounds(Geom::OptRect const &bounds)
{
    _item_bbox = bounds;
}

void
DrawingItem::update(Geom::IntRect const &area, UpdateContext const &ctx, unsigned flags, unsigned reset)
{
    bool render_filters = (_drawing->rendermode == Inkscape::RENDERMODE_NORMAL);
    bool outline = (_drawing->rendermode == Inkscape::RENDERMODE_OUTLINE);

    // Set reset flags according to propagation status
    if (_propagate) {
        reset |= ~_state;
        _propagate = FALSE;
    }
    _state &= ~reset; // reset state of this item

    if ((~_state & flags) == 0) return; // nothing to do

    // TODO this might be wrong
    if (_state & STATE_BBOX) {
        // we have up-to-date bbox
        if (!area.intersects(outline ? _bbox : _drawbox)) return;
    }

    UpdateContext child_ctx(ctx);
    if (_transform) {
        child_ctx.ctm = *_transform * ctx.ctm;
    }
    /* Remember the transformation matrix */
    Geom::Affine ctm_change = _ctm.inverse() * child_ctx.ctm;
    _ctm = child_ctx.ctm;

    // update _bbox
    _state = _updateItem(area, child_ctx, flags, reset);

    // compute drawbox
    if (_filter && render_filters && _item_bbox) {
        _drawbox = _filter->compute_drawbox(this, *_item_bbox);
    } else {
        _drawbox = _bbox;
    }

    // Clipping
    if (_clip) {
        _clip->update(area, child_ctx, flags, reset);
        if (outline) {
            _bbox.unionWith(_clip->_bbox);
        } else {
            _drawbox.intersectWith(_clip->_bbox);
        }
    }
    // masking
    if (_mask) {
        _mask->update(area, child_ctx, flags, reset);
        if (outline) {
            _bbox.unionWith(_mask->_bbox);
        } else {
            // for masking, we need full drawbox of mask
            _drawbox.intersectWith(_mask->_drawbox);
        }
    }

    // update cache if enabled
    if (_cached) {
        Geom::OptIntRect cl = _drawing->cache_limit;
        cl.intersectWith(_drawbox);
        if (cl) {
            if (_cache) {
                // this takes care of invalidation on transform
                _cache->resizeAndTransform(*cl, ctm_change);
            } else {
                _cache = new Inkscape::DrawingCache(*cl);
                // the cache is initially dirty
            }
        } else {
            // disable cache for this item - not visible
            delete _cache;
            _cache = NULL;
        }
    }

    // now that we know drawbox, dirty the corresponding rect on canvas
    // unless filtered, groups do not need to render by themselves, only their members
    if (!is_drawing_group(this) || (_filter && render_filters)) {
        if (flags & ~STATE_CACHE) {
            _markForRendering();
        }
    }
}

struct MaskLuminanceToAlpha {
    guint32 operator()(guint32 in) {
        EXTRACT_ARGB32(in, a, r, g, b)
        // the operation of unpremul -> luminance-to-alpha -> multiply by alpha
        // is equivalent to luminance-to-alpha on premultiplied color values
        // original computation in double: r*0.2125 + g*0.7154 + b*0.0721
        guint32 ao = r*109 + g*366 + b*37; // coeffs add up to 512
        return ((ao + 256) << 15) & 0xff000000; // equivalent to ((ao + 256) / 512) << 24
    }
};

void
DrawingItem::render(DrawingContext &ct, Geom::IntRect const &area, unsigned flags)
{
    bool outline = (_drawing->rendermode == Inkscape::RENDERMODE_OUTLINE);
    bool render_filters = (_drawing->rendermode == Inkscape::RENDERMODE_NORMAL);

    /* If we are invisible, just return successfully */
    if (!_visible) return;

    if (outline) {
        _renderOutline(ct, area, flags);
        return;
    }

    // carea is the bounding box for intermediate rendering.
    Geom::OptIntRect carea = Geom::intersect(area, _drawbox);
    if (!carea) return;

    // render from cache
    if (_cached && _cache) {
        if (_cache->paintFromCache(ct, *carea))
            return;
    }

    // expand carea to contain the dependent area of filters.
    if (_filter && render_filters) {
        _filter->area_enlarge(*carea, this);
        carea.intersectWith(_drawbox);
    }

    // determine whether this shape needs intermediate rendering.
    bool needs_intermediate_rendering = false;
    bool &nir = needs_intermediate_rendering;
    bool needs_opacity = (_opacity < 0.995);

    // this item needs an intermediate rendering if:
    nir |= (_clip != NULL); // 1. it has a clipping path
    nir |= (_mask != NULL); // 2. it has a mask
    nir |= (_filter != NULL && render_filters); // 3. it has a filter
    nir |= needs_opacity; // 4. it is non-opaque

    /* How the rendering is done.
     *
     * Clipping, masking and opacity are done by rendering them to a surface
     * and then compositing the object's rendering onto it with the IN operator.
     * The object itself is rendered to a group.
     *
     * Opacity is done by rendering the clipping path with an alpha
     * value corresponding to the opacity. If there is no clipping path,
     * the entire intermediate surface is painted with alpha corresponding
     * to the opacity value.
     */

    // short-circuit the simple case.
    if (!needs_intermediate_rendering) {
        if (_cached && _cache) {
            Inkscape::DrawingContext cachect(*_cache);
            cachect.rectangle(area);
            cachect.clip();

            {   // 1. clear the corresponding part of cache
                Inkscape::DrawingContext::Save save(cachect);
                cachect.setSource(0,0,0,0);
                cachect.setOperator(CAIRO_OPERATOR_SOURCE);
                cachect.paint();
            }
            // 2. render to cache
            _renderItem(cachect, *carea, flags);
            // 3. copy from cache to output
            Inkscape::DrawingContext::Save save(ct);
            ct.rectangle(*carea);
            ct.clip();
            ct.setSource(_cache);
            ct.paint();
            // 4. mark as clean
            _cache->markClean(area);
            return;
        } else {
            _renderItem(ct, *carea, flags);
            return;
        }
    }

    DrawingSurface intermediate(*carea);
    DrawingContext ict(intermediate);

    // 1. Render clipping path with alpha = opacity.
    ict.setSource(0,0,0,_opacity);
    // Since clip can be combined with opacity, the result could be incorrect
    // for overlapping clip children. To fix this we use the SOURCE operator
    // instead of the default OVER.
    ict.setOperator(CAIRO_OPERATOR_SOURCE);
    if (_clip) {
        _clip->clip(ict, *carea); // fixme: carea or area?
    } else {
        // if there is no clipping path, fill the entire surface with alpha = opacity.
        ict.paint();
    }
    ict.setOperator(CAIRO_OPERATOR_OVER); // reset back to default

    // 2. Render the mask if present and compose it with the clipping path + opacity.
    if (_mask) {
        ict.pushGroup();
        _mask->render(ict, *carea, flags);

        cairo_surface_t *mask_s = ict.rawTarget();
        // Convert mask's luminance to alpha
        ink_cairo_surface_filter(mask_s, mask_s, MaskLuminanceToAlpha());
        ict.popGroupToSource();
        ict.setOperator(CAIRO_OPERATOR_IN);
        ict.paint();
        ict.setOperator(CAIRO_OPERATOR_OVER);
    }

    // 3. Render object itself.
    ict.pushGroup();
    _renderItem(ict, *carea, flags);

    // 4. Apply filter.
    if (_filter && render_filters) {
        _filter->render(this, ct, ict);
        // Note that because the object was rendered to a group,
        // the internals of the filter need to use cairo_get_group_target()
        // instead of cairo_get_target().
    }

    // 5. Render object inside the composited mask + clip
    ict.popGroupToSource();
    ict.setOperator(CAIRO_OPERATOR_IN);
    ict.paint();

    // 6. Paint the completed rendering onto the base context (or into cache)
    if (_cached && _cache) {
        DrawingContext cachect(*_cache);
        cachect.rectangle(area);
        cachect.clip();
        cachect.setOperator(CAIRO_OPERATOR_SOURCE);
        cachect.setSource(&intermediate);
        cachect.paint();
        _cache->markClean(area);
    }
    ct.setSource(&intermediate);
    ct.paint();
    ct.setSource(0,0,0,0);
    // the call above is to clear a ref on the intermediate surface held by ct
}

void
DrawingItem::_renderOutline(DrawingContext &ct, Geom::IntRect const &area, unsigned flags)
{
    // intersect with bbox rather than drawbox, as we want to render things outside
    // of the clipping path as well
    Geom::OptIntRect carea = Geom::intersect(area, _bbox);
    if (!carea) return;

    // just render everything: item, clip, mask
    // First, render the object itself
    _renderItem(ct, *carea, flags);

    // render clip and mask, if any
    guint32 saved_rgba = _drawing->outlinecolor; // save current outline color
    // render clippath as an object, using a different color
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (_clip) {
        _drawing->outlinecolor = prefs->getInt("/options/wireframecolors/clips", 0x00ff00ff); // green clips
        _clip->render(ct, *carea, flags);
    }
    // render mask as an object, using a different color
    if (_mask) {
        _drawing->outlinecolor = prefs->getInt("/options/wireframecolors/masks", 0x0000ffff); // blue masks
        _mask->render(ct, *carea, flags);
    }
    _drawing->outlinecolor = saved_rgba; // restore outline color
}

void
DrawingItem::clip(Inkscape::DrawingContext &ct, Geom::IntRect const &area)
{
    // don't bother if the object does not implement clipping (e.g. DrawingImage)
    if (!_canClip()) return;
    if (!_visible) return;
    if (!area.intersects(_bbox)) return;

    // The item used as the clipping path itself has a clipping path.
    // Render this item's clipping path onto a temporary surface, then composite it
    // with the item using the IN operator
    if (_clip) {
        ct.pushAlphaGroup();
        {   Inkscape::DrawingContext::Save save(ct);
            ct.setSource(0,0,0,1);
            _clip->clip(ct, area);
        }
        ct.pushAlphaGroup();
    }

    // rasterize the clipping path
    _clipItem(ct, area);
    
    if (_clip) {
        ct.popGroupToSource();
        ct.setOperator(CAIRO_OPERATOR_IN);
        ct.paint();
        ct.popGroupToSource();
        ct.setOperator(CAIRO_OPERATOR_SOURCE);
        ct.paint();
    }
}

DrawingItem *
DrawingItem::pick(Geom::Point const &p, double delta, bool sticky)
{
    // Sometimes there's no BBOX in state, reason unknown (bug 992817)
    // I made this not an assert to remove the warning
    if (!(_state & STATE_BBOX) || !(_state & STATE_PICK))
        return NULL;

    if (!sticky && !(_visible && _sensitive))
        return NULL;

    if (!_bbox) return NULL;
    Geom::Rect expanded(*_bbox);
    expanded.expandBy(delta);

    if (expanded.contains(p)) {
        return _pickItem(p, delta);
    }
    return NULL;
}

void
DrawingItem::_markForRendering()
{
    bool outline = (_drawing->rendermode == Inkscape::RENDERMODE_OUTLINE);
    Geom::OptIntRect dirty = outline ? _bbox : _drawbox;
    if (!dirty) return;

    // dirty the caches of all parents
    for (DrawingItem *i = this; i; i = i->_parent) {
        if (i->_cached && i->_cache) {
            i->_cache->markDirty(*dirty);
        }
    }

    nr_arena_request_render_rect (_drawing, dirty);
}

void
DrawingItem::_markForUpdate(unsigned flags, bool propagate)
{
    // here we can't simply assign because a previous markForUpdate call
    // could have had propagate=true even if this one has propagate=false
    if (propagate)
        _propagate = true;

    if (_state & flags) {
        _state &= ~flags;
        if (_parent) {
            _parent->_markForUpdate(flags, false);
        } else {
            nr_arena_request_update (_drawing, this);
        }
    }
}

void
DrawingItem::_setStyleCommon(SPStyle *&_style, SPStyle *style)
{
    if (style) sp_style_ref(style);
    if (_style) sp_style_unref(_style);
    _style = style;

    // if group has a filter
    if (style->filter.set && style->getFilter()) {
        if (!_filter) {
            int primitives = sp_filter_primitive_count(SP_FILTER(style->getFilter()));
            _filter = new Inkscape::Filters::Filter(primitives);
        }
        sp_filter_build_renderer(SP_FILTER(style->getFilter()), _filter);
    } else {
        // no filter set for this group
        delete _filter;
        _filter = NULL;
    }

    /*
    if (style && style->enable_background.set
        && style->enable_background.value == SP_CSS_BACKGROUND_NEW) {
        _background_new = true;
    }*/
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
