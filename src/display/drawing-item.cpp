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

#include <climits>
#include "display/cairo-utils.h"
#include "display/cairo-templates.h"
#include "display/drawing.h"
#include "display/drawing-context.h"
#include "display/drawing-item.h"
#include "display/drawing-group.h"
#include "display/drawing-surface.h"
#include "nr-filter.h"
#include "preferences.h"
#include "style.h"

namespace Inkscape {

/** @class DrawingItem
 * @brief SVG drawing item for display.
 *
 * This was previously known as NRArenaItem. It represents the renderable
 * portion of the SVG document. Typically this is created by the SP tree,
 * in particular the show() virtual function.
 *
 * @section ObjectLifetime Object lifetime
 * Deleting a DrawingItem will cause all of its children to be deleted as well.
 * This can lead to nasty surprises if you hold references to things
 * which are children of what is being deleted. Therefore, in the SP tree,
 * you always need to delete the item views of children before deleting
 * the view of the parent. Do not call delete on things returned from show()
 * - this will cause dangling pointers inside the SPItem and lead to a crash.
 * Use the corresponing hide() method.
 *
 * Outside of the SP tree, you should not use any references after the root node
 * has been deleted.
 */

DrawingItem::DrawingItem(Drawing &drawing)
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
    , _cached_persistent(0)
    , _has_cache_iterator(0)
    , _propagate(0)
//    , _renders_opacity(0)
    , _clip_child(0)
    , _mask_child(0)
    , _drawing_root(0)
    , _pick_children(0)
{
}

DrawingItem::~DrawingItem()
{
    _drawing.signal_item_deleted.emit(this);
    //if (!_children.empty()) {
    //    g_warning("Removing item with children");
    //}

    // remove from the set of cached items
    if (_cached) {
        _drawing._cached_items.erase(this);
    }
    if (_has_cache_iterator) {
        _drawing._candidate_items.erase(_cache_iterator);
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
    } else if (_drawing_root) {
        _drawing._root = NULL;
    }
    clearChildren();
    delete _transform;
    delete _clip;
    delete _mask;
    delete _filter;
}

DrawingItem *
DrawingItem::parent() const
{
    // initially I wanted to return NULL if we are a clip or mask child,
    // but the previous behavior was just to return the parent
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

/// Delete all regular children of this item (not mask or clip).
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

/// Set the incremental transform for this item
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

/// This is currently unused
void
DrawingItem::setSensitive(bool s)
{
    _sensitive = s;
}

/** @brief Enable / disable storing the rendering in memory.
 * Calling setCached(false, true) will also remove the persistent status
 */
void
DrawingItem::setCached(bool cached, bool persistent)
{
    static const char *cache_env = getenv("_INKSCAPE_DISABLE_CACHE");
    if (cache_env) return;

    if (_cached_persistent && !persistent)
        return;

    _cached = cached;
    _cached_persistent = persistent ? cached : false;
    if (cached) {
        _drawing._cached_items.insert(this);
    } else {
        _drawing._cached_items.erase(this);
        delete _cache;
        _cache = NULL;
    }
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

/// Move this item to the given place in the Z order of siblings.
/// Does nothing if the item has no parent.
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

/** @brief Update derived data before operations.
 * The purpose of this call is to recompute internal data which depends
 * on the attributes of the object, but is not directly settable by the user.
 * Precomputing this data speeds up later rendering, because some items
 * can be omitted.
 *
 * Currently this method handles updating the visual and geometric bounding boxes
 * in pixels, storing the total transformation from item space to the screen
 * and cache invalidation.
 *
 * @param area Area to which the update should be restricted. Only takes effect
 *             if the bounding box is known.
 * @param ctx A structure to store cascading state.
 * @param flags Which internal data should be recomputed. This can be any combination
 *              of StateFlags.
 * @param reset State fields that should be reset before processing them. This is
 *              a means to force a recomputation of internal data even if the item
 *              considers it up to date. Mainly for internal use, such as
 *              propagating bunding box recomputation to children when the item's
 *              transform changes.
 */
void
DrawingItem::update(Geom::IntRect const &area, UpdateContext const &ctx, unsigned flags, unsigned reset)
{
    bool render_filters = _drawing.renderFilters();
    bool outline = _drawing.outline();

    // Set reset flags according to propagation status
    if (_propagate) {
        reset |= ~_state;
        _propagate = FALSE;
    }
    _state &= ~reset; // reset state of this item

    if ((~_state & flags) == 0) return;  // nothing to do

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

    // Update cache score for this item
    if (_has_cache_iterator) {
        // remove old score information
        _drawing._candidate_items.erase(_cache_iterator);
        _has_cache_iterator = false;
    }
    double score = _cacheScore();
    if (score >= _drawing._cache_score_threshold) {
        CacheRecord cr;
        cr.score = score;
        // if _cacheRect() is empty, a negative score will be returnedfrom _cacheScore(),
        // so this will not execute (cache score threshold must be positive)
        cr.cache_size = _cacheRect()->area() * 4;
        cr.item = this;
        _drawing._candidate_items.push_back(cr);
        _cache_iterator = --_drawing._candidate_items.end();
        _has_cache_iterator = true;
    }

    /* Update cache if enabled.
     * General note: here we only tell the cache how it has to transform
     * during the render phase. The transformation is deferred because
     * after the update the item can have its caching turned off,
     * e.g. because its filter was removed. This way we avoid tempoerarily
     * using more memory than the cache budget */
    if (_cache) {
        Geom::OptIntRect cl = _cacheRect();
        if (_visible && cl) { // never create cache for invisible items
            // this takes care of invalidation on transform
            _cache->scheduleTransform(*cl, ctm_change);
        } else {
            // Destroy cache for this item - outside of canvas or invisible.
            // The opposite transition (invisible -> visible or object
            // entering the canvas) is handled during the render phase
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

/** @brief Rasterize items.
 * This method submits the drawing opeartions required to draw this item
 * to the supplied DrawingContext, restricting drawing the the specified area.
 *
 * This method does some common tasks and calls the item-specific rendering
 * function, _renderItem(), to render e.g. paths or bitmaps.
 *
 * @param flags Rendering options. This deals mainly with cache control.
 */
void
DrawingItem::render(DrawingContext &ct, Geom::IntRect const &area, unsigned flags)
{
    bool outline = _drawing.outline();
    bool render_filters = _drawing.renderFilters();

    // If we are invisible, return immediately
    if (!_visible) return;

    // TODO convert outline rendering to a separate virtual function
    if (outline) {
        _renderOutline(ct, area, flags);
        return;
    }

    // carea is the bounding box for intermediate rendering.
    Geom::OptIntRect carea = Geom::intersect(area, _drawbox);
    if (!carea) return;

    // render from cache if possible
    if (_cached) {
        if (_cache) {
            _cache->prepare();
            if (_cache->paintFromCache(ct, *carea))
                return;
        } else {
            // There is no cache. This could be because caching of this item
            // was just turned on after the last update phase, or because
            // we are outside of the canvas.
            Geom::OptIntRect cl = _drawing.cacheLimit();
            cl.intersectWith(_drawbox);
            if (cl) {
                _cache = new DrawingCache(*cl);
            }
        }
    } else {
        // if our caching was turned off after the last update, it was already
        // deleted in setCached()
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
    guint32 saved_rgba = _drawing.outlinecolor; // save current outline color
    // render clippath as an object, using a different color
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (_clip) {
        _drawing.outlinecolor = prefs->getInt("/options/wireframecolors/clips", 0x00ff00ff); // green clips
        _clip->render(ct, *carea, flags);
    }
    // render mask as an object, using a different color
    if (_mask) {
        _drawing.outlinecolor = prefs->getInt("/options/wireframecolors/masks", 0x0000ffff); // blue masks
        _mask->render(ct, *carea, flags);
    }
    _drawing.outlinecolor = saved_rgba; // restore outline color
}

/** @brief Rasterize the clipping path.
 * This method submits drawing operations required to draw a basic filled shape
 * of the item to the supplied drawing context. Rendering is limited to the
 * given area. The rendering of the clipped object is composited into
 * the result of this call using the IN operator. See the implementation
 * of render() for details.
 */
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

/** @brief Get the item under the specified point.
 * Searches the tree for the first item in the Z-order which is closer than
 * @a delta to the given point. The pick should be visual - for example
 * an object with a thick stroke should pick on the entire area of the stroke.
 * @param p Search point
 * @param delta Maximum allowed distance from the point
 * @param sticky Whether the pick should ignore visibility and sensitivity.
 *               When false, only visible and sensitive objects are considered.
 *               When true, invisible and insensitive objects can also be picked.
 */
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
        return _pickItem(p, delta, sticky);
    }
    return NULL;
}

/** Marks the current visual bounding box of the item for redrawing.
 * This is called whenever the object changes its visible appearance.
 * For some cases (such as setting opacity) this is enough, but for others
 * _markForUpdate() also needs to be called.
 */
void
DrawingItem::_markForRendering()
{
    bool outline = _drawing.outline();
    Geom::OptIntRect dirty = outline ? _bbox : _drawbox;
    if (!dirty) return;

    // dirty the caches of all parents
    for (DrawingItem *i = this; i; i = i->_parent) {
        if (i->_cached && i->_cache) {
            i->_cache->markDirty(*dirty);
        }
    }
    _drawing.signal_request_render.emit(*dirty);
}

/** @brief Marks the item as needing a recomputation of internal data.
 *
 * This mechanism avoids traversing the entire rendering tree (which could be vast)
 * on every trivial state changed in any item. Only items marked as needing
 * an update (having some bits in their _state unset) will be traversed
 * during the update call.
 *
 * The _propagate variable is another optimization. We use it to specify that
 * all children should also have the corresponding flags unset before checking
 * whether they need to be traversed. This way there is one less traversal
 * of the tree. Without this we would need to unset state bits in all children.
 * With _propagate we do this during the update call, when we have to traverse
 * the tree anyway.
 */
void
DrawingItem::_markForUpdate(unsigned flags, bool propagate)
{
    // we can't simply assign because a previous markForUpdate call
    // could have had propagate=true even if this one has propagate=false
    if (propagate)
        _propagate = true;

    if (_state & flags) {
        _state &= ~flags;
        if (_parent) {
            _parent->_markForUpdate(flags, false);
        } else {
            _drawing.signal_request_update.emit(this);
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

    // TODO: STATE_ALL unsets too much
    _markForUpdate(STATE_ALL, false);
}

double
DrawingItem::_cacheScore()
{
    Geom::OptIntRect cache_rect = _cacheRect();
    if (!cache_rect) return -1.0;

    // a crude first approximation:
    // the basic score is the number of pixels in the drawbox
    double score = cache_rect->area();
    // this is multiplied by the filter complexity and its expansion
    if (_filter &&_drawing.renderFilters()) {
        score *= _filter->complexity(_ctm);
        Geom::IntRect ref_area = Geom::IntRect::from_xywh(0, 0, 16, 16);
        Geom::IntRect test_area = ref_area;
        Geom::IntRect limit_area(0, INT_MIN, 16, INT_MAX);
        _filter->area_enlarge(test_area, this);
        // area_enlarge never shrinks the rect, so the result of intersection below
        // must be non-empty
        score *= double((test_area & limit_area)->area()) / ref_area.area();
    }
    // if the object is clipped, add 1/2 of its bbox pixels
    if (_clip && _clip->_bbox) {
        score += _clip->_bbox->area() * 0.5;
    }
    // if masked, add mask score
    if (_mask) {
        score += _mask->_cacheScore();
    }
    //g_message("caching score: %f", score);
    return score;
}

Geom::OptIntRect
DrawingItem::_cacheRect()
{
    Geom::OptIntRect r = _drawbox & _drawing.cacheLimit();
    return r;
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
