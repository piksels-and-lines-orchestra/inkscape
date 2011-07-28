#define __NR_ARENA_ITEM_C__

/*
 * RGBA display list system for inkscape
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define noNR_ARENA_ITEM_VERBOSE
#define noNR_ARENA_ITEM_DEBUG_CASCADE

#include <cstring>
#include <string>
#include <cairomm/cairomm.h>

#include "display/cairo-utils.h"
#include "display/cairo-templates.h"
#include "display/drawing-context.h"
#include "display/drawing-surface.h"
#include "display/canvas-arena.h"
#include "nr-arena.h"
#include "nr-arena-item.h"
#include "gc-core.h"
#include "helper/geom.h"

#include "nr-filter.h"
#include "nr-arena-group.h"
#include "preferences.h"

namespace GC = Inkscape::GC;

static void nr_arena_item_class_init (NRArenaItemClass *klass);
static void nr_arena_item_init (NRArenaItem *item);
static void nr_arena_item_private_finalize (NRObject *object);

static NRObjectClass *parent_class;

NRType 
nr_arena_item_get_type (void)
{
    static NRType type = 0;
    if (!type) {
        type = nr_object_register_type (NR_TYPE_OBJECT,
                                        "NRArenaItem",
                                        sizeof (NRArenaItemClass),
                                        sizeof (NRArenaItem),
                                        (void (*)(NRObjectClass *))
                                        nr_arena_item_class_init,
                                        (void (*)(NRObject *))
                                        nr_arena_item_init);
    }
    return type;
}

static void
nr_arena_item_class_init (NRArenaItemClass *klass)
{
    NRObjectClass *object_class;

    object_class = (NRObjectClass *) klass;

    parent_class = ((NRObjectClass *) klass)->parent;

    object_class->finalize = nr_arena_item_private_finalize;
    object_class->cpp_ctor = NRObject::invoke_ctor < NRArenaItem >;
}

static void
nr_arena_item_init (NRArenaItem *item)
{
    item->arena = NULL;
    item->parent = NULL;
    item->next = item->prev = NULL;

    item->key = 0;

    item->state = 0;
    item->sensitive = TRUE;
    item->visible = TRUE;

    memset (&item->bbox, 0, sizeof (item->bbox));
    memset (&item->drawbox, 0, sizeof (item->drawbox));
    item->transform = NULL;
    item->ctm.setIdentity();
    item->opacity = 255;
    item->render_opacity = FALSE;
    item->render_cache = FALSE;

    item->transform = NULL;
    item->clip = NULL;
    item->mask = NULL;
    item->cache = NULL;
    item->data = NULL;
    item->filter = NULL;
    item->background_new = false;
}

static void
nr_arena_item_private_finalize (NRObject *object)
{
    NRArenaItem *item = static_cast < NRArenaItem * >(object);

    item->transform = NULL;

    if (item->clip)
        nr_arena_item_detach(item, item->clip);
    if (item->mask)
        nr_arena_item_detach(item, item->mask);
    delete item->cache;

    ((NRObjectClass *) (parent_class))->finalize (object);
}

NRArenaItem *
nr_arena_item_children (NRArenaItem *item)
{
    nr_return_val_if_fail (item != NULL, NULL);
    nr_return_val_if_fail (NR_IS_ARENA_ITEM (item), NULL);

    if (NR_ARENA_ITEM_VIRTUAL (item, children))
        return NR_ARENA_ITEM_VIRTUAL (item, children) (item);

    return NULL;
}

NRArenaItem *
nr_arena_item_last_child (NRArenaItem *item)
{
    nr_return_val_if_fail (item != NULL, NULL);
    nr_return_val_if_fail (NR_IS_ARENA_ITEM (item), NULL);

    if (NR_ARENA_ITEM_VIRTUAL (item, last_child)) {
        return NR_ARENA_ITEM_VIRTUAL (item, last_child) (item);
    } else {
        NRArenaItem *ref = nr_arena_item_children (item);
        if (ref)
            while (ref->next)
                ref = ref->next;
        return ref;
    }
}

void
nr_arena_item_add_child (NRArenaItem *item, NRArenaItem *child,
                         NRArenaItem *ref)
{
    nr_return_if_fail (item != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (item));
    nr_return_if_fail (child != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (child));
    nr_return_if_fail (child->parent == NULL);
    nr_return_if_fail (child->prev == NULL);
    nr_return_if_fail (child->next == NULL);
    nr_return_if_fail (child->arena == item->arena);
    nr_return_if_fail (child != ref);
    nr_return_if_fail (!ref || NR_IS_ARENA_ITEM (ref));
    nr_return_if_fail (!ref || (ref->parent == item));

    if (NR_ARENA_ITEM_VIRTUAL (item, add_child))
        NR_ARENA_ITEM_VIRTUAL (item, add_child) (item, child, ref);
}

void
nr_arena_item_remove_child (NRArenaItem *item, NRArenaItem *child)
{
    nr_return_if_fail (item != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (item));
    nr_return_if_fail (child != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (child));
    nr_return_if_fail (child->parent == item);

    if (NR_ARENA_ITEM_VIRTUAL (item, remove_child))
        NR_ARENA_ITEM_VIRTUAL (item, remove_child) (item, child);
}

void
nr_arena_item_set_child_position (NRArenaItem *item, NRArenaItem *child,
                                  NRArenaItem *ref)
{
    nr_return_if_fail (item != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (item));
    nr_return_if_fail (child != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (child));
    nr_return_if_fail (child->parent == item);
    nr_return_if_fail (!ref || NR_IS_ARENA_ITEM (ref));
    nr_return_if_fail (!ref || (ref->parent == item));

    if (NR_ARENA_ITEM_VIRTUAL (item, set_child_position))
        NR_ARENA_ITEM_VIRTUAL (item, set_child_position) (item, child, ref);
}

NRArenaItem *
nr_arena_item_ref (NRArenaItem *item)
{
    nr_object_ref ((NRObject *) item);

    return item;
}

NRArenaItem *
nr_arena_item_unref (NRArenaItem *item)
{
    nr_object_unref ((NRObject *) item);

    return NULL;
}

unsigned int
nr_arena_item_invoke_update (NRArenaItem *item, Geom::IntRect const &area, NRGC *gc,
                             unsigned int state, unsigned int reset)
{
    NRGC childgc (gc);
    bool filter = (item->arena->rendermode == Inkscape::RENDERMODE_NORMAL);
    bool outline = (item->arena->rendermode == Inkscape::RENDERMODE_OUTLINE);

    nr_return_val_if_fail (item != NULL, NR_ARENA_ITEM_STATE_INVALID);
    nr_return_val_if_fail (NR_IS_ARENA_ITEM (item),
                           NR_ARENA_ITEM_STATE_INVALID);
    nr_return_val_if_fail (!(state & NR_ARENA_ITEM_STATE_INVALID),
                           NR_ARENA_ITEM_STATE_INVALID);

#ifdef NR_ARENA_ITEM_DEBUG_CASCADE
    printf ("Update %s:%p %x %x %x\n",
            nr_type_name_from_instance ((GTypeInstance *) item), item, state,
            item->state, reset);
#endif

    /* return if in error */
    if (item->state & NR_ARENA_ITEM_STATE_INVALID)
        return item->state;
    /* Set reset flags according to propagation status */
    if (item->propagate) {
        reset |= ~item->state;
        item->propagate = FALSE;
    }
    /* Reset our state */
    item->state &= ~reset;
    /* Return if NOP */
    if (!(~item->state & state))
        return item->state;
    /* Test whether to return immediately */
    if (item->state & NR_ARENA_ITEM_STATE_BBOX) {
        // we have up-to-date bbox
        if (!area.intersects(outline ? item->bbox : item->drawbox))
            return item->state;
    }

    /* Set up local gc */
    childgc = *gc;
    if (item->transform) {
        childgc.transform = (*item->transform) * childgc.transform;
    }
    /* Remember the transformation matrix */
    Geom::Affine ctm_change = item->ctm.inverse() * childgc.transform;
    item->ctm = childgc.transform;

    /* Invoke the real method */
    // that will update bbox
    item->state = NR_ARENA_ITEM_VIRTUAL (item, update) (item, area, &childgc, state, reset);
    if (item->state & NR_ARENA_ITEM_STATE_INVALID)
        return item->state;

    /* Enlarge the drawbox to contain filter effects */
    if (item->filter && filter && item->item_bbox) {
        item->drawbox = item->filter->compute_drawbox(item, *item->item_bbox);
    } else {
        item->drawbox = item->bbox;
    }

    /* Clipping */
    if (item->clip) {
        // FIXME: since here we only need bbox, consider passing 
        // ((state & !(NR_ARENA_ITEM_STATE_RENDER)) | NR_ARENA_ITEM_STATE_BBOX)
        // instead of state, so it does not have to create rendering structures in nr_arena_shape_update
        unsigned int newstate = nr_arena_item_invoke_update (item->clip, area, &childgc, state, reset); 
        if (newstate & NR_ARENA_ITEM_STATE_INVALID) {
            item->state |= NR_ARENA_ITEM_STATE_INVALID;
            return item->state;
        }
        if (outline) {
            item->bbox.unionWith(item->clip->bbox);
        } else {
            item->drawbox.intersectWith(item->clip->bbox);
        }
    }
    /* Masking */
    if (item->mask) {
        unsigned int newstate = nr_arena_item_invoke_update (item->mask, area, &childgc, state, reset);
        if (newstate & NR_ARENA_ITEM_STATE_INVALID) {
            item->state |= NR_ARENA_ITEM_STATE_INVALID;
            return item->state;
        }
        if (outline) {
            item->bbox.unionWith(item->mask->bbox);
        } else {
            // for masking, we need full drawbox of mask
            item->drawbox.intersectWith(item->mask->drawbox);
        }
    }

    // update cache if enabled
    if (item->render_cache) {
        Geom::OptIntRect cl = item->arena->cache_limit;
        cl.intersectWith(item->drawbox);
        if (cl) {
            if (item->cache) {
                // this takes care of invalidation on transform
                item->cache->resizeAndTransform(*cl, ctm_change);
            } else {
                item->cache = new Inkscape::DrawingCache(*cl);
                // the cache is initially dirty
            }
        } else {
            // disable cache for this item - not visible
            delete item->cache;
            item->cache = NULL;
        }
    }

    // now that we know drawbox, dirty the corresponding rect on canvas:
    if (!NR_IS_ARENA_GROUP(item) || (item->filter && filter)) {
        // unless filtered, groups do not need to render by themselves, only their members
        if (state & ~NR_ARENA_ITEM_STATE_CACHE) {
            nr_arena_item_request_render (item);
        }
    }

    return item->state;
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

unsigned int
nr_arena_item_invoke_render (Inkscape::DrawingContext &ct, NRArenaItem *item, Geom::IntRect const &area,
                             unsigned int flags)
{
   bool outline = (item->arena->rendermode == Inkscape::RENDERMODE_OUTLINE);
    bool filter = (item->arena->rendermode != Inkscape::RENDERMODE_OUTLINE &&
                   item->arena->rendermode != Inkscape::RENDERMODE_NO_FILTERS);

    nr_return_val_if_fail (item != NULL, NR_ARENA_ITEM_STATE_INVALID);
    nr_return_val_if_fail (NR_IS_ARENA_ITEM (item),
                           NR_ARENA_ITEM_STATE_INVALID);
    nr_return_val_if_fail (item->state & NR_ARENA_ITEM_STATE_BBOX,
                           item->state);

    /* If we are invisible, just return successfully */
    if (!item->visible)
        return item->state | NR_ARENA_ITEM_STATE_RENDER;

    if (outline) {
        // intersect with bbox rather than drawbox, as we want to render things outside
        // of the clipping path as well
        Geom::OptIntRect carea = Geom::intersect(area, item->bbox);
        if (!carea)
            return item->state | NR_ARENA_ITEM_STATE_RENDER;

        // No caching in outline mode for now; investigate if it really gives any advantage with cairo.
        // Also no attempts to clip anything; just render everything: item, clip, mask   
        // First, render the object itself 
        unsigned int state = NR_ARENA_ITEM_VIRTUAL (item, render) (ct, item, *carea, flags);
        if (state & NR_ARENA_ITEM_STATE_INVALID) {
            /* Clean up and return error */
            item->state |= NR_ARENA_ITEM_STATE_INVALID;
            return item->state;
        }

        // render clip and mask, if any
        guint32 saved_rgba = item->arena->outlinecolor; // save current outline color
        // render clippath as an object, using a different color
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        if (item->clip) {
            item->arena->outlinecolor = prefs->getInt("/options/wireframecolors/clips", 0x00ff00ff); // green clips
            NR_ARENA_ITEM_VIRTUAL (item->clip, render) (ct, item->clip, *carea, flags);
        }
        // render mask as an object, using a different color
        if (item->mask) {
            item->arena->outlinecolor = prefs->getInt("/options/wireframecolors/masks", 0x0000ffff); // blue masks
            NR_ARENA_ITEM_VIRTUAL (item->mask, render) (ct, item->mask, *carea, flags);
        }
        item->arena->outlinecolor = saved_rgba; // restore outline color

        return item->state | NR_ARENA_ITEM_STATE_RENDER;
    }

    // carea is the bounding box for intermediate rendering.
    Geom::OptIntRect carea = Geom::intersect(area, item->drawbox);
    if (!carea)
        return item->state | NR_ARENA_ITEM_STATE_RENDER;

    // render from cache
    if (item->render_cache && item->cache) {
        if(item->cache->paintFromCache(ct, *carea))
            return item->state | NR_ARENA_ITEM_STATE_RENDER;
    }

    // expand carea to contain the dependent area of filters.
    if (item->filter && filter) {
        item->filter->area_enlarge(*carea, item);
        carea.intersectWith(item->drawbox);
    }

    using namespace Inkscape;

    unsigned state;
    unsigned retstate;

    // determine whether this shape needs intermediate rendering.
    bool needs_intermediate_rendering = false;
    bool &nir = needs_intermediate_rendering;
    bool needs_opacity = (item->opacity != 255 && !item->render_opacity);

    // this item needs an intermediate rendering if:
    nir |= (item->clip != NULL); // 1. it has a clipping path
    nir |= (item->mask != NULL); // 2. it has a mask
    nir |= (item->filter != NULL && filter); // 3. it has a filter
    nir |= needs_opacity; // 4. it is non-opaque

    double opacity = static_cast<double>(item->opacity) / 255.0;

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
        if (item->render_cache && item->cache) {
            Inkscape::DrawingContext cachect(*item->cache);
            cachect.rectangle(area);
            cachect.clip();

            {   // 1. clear the corresponding part of cache
                Inkscape::DrawingContext::Save save(cachect);
                cachect.setSource(0,0,0,0);
                cachect.setOperator(CAIRO_OPERATOR_SOURCE);
                cachect.paint();
            }
            // 2. render to cache
            state = NR_ARENA_ITEM_VIRTUAL (item, render) (cachect, item, *carea, flags);
            if (state & NR_ARENA_ITEM_STATE_INVALID) {
                item->state |= NR_ARENA_ITEM_STATE_INVALID;
                return item->state;
            }
            // 3. copy from cache to output
            Inkscape::DrawingContext::Save save(ct);
            ct.rectangle(*carea);
            ct.clip();
            ct.setSource(item->cache);
            ct.paint();
            // 4. mark as clean
            item->cache->markClean(area);
            return item->state | NR_ARENA_ITEM_STATE_RENDER;
        } else {
            state = NR_ARENA_ITEM_VIRTUAL (item, render) (ct, item, *carea, flags);
            if (state & NR_ARENA_ITEM_STATE_INVALID) {
                item->state |= NR_ARENA_ITEM_STATE_INVALID;
                return item->state;
            }
            return item->state | NR_ARENA_ITEM_STATE_RENDER;
        }
    }

    DrawingSurface intermediate(*carea);
    DrawingContext ict(intermediate);

    // 1. Render clipping path with alpha = opacity.
    ict.setSource(0,0,0,opacity);
    // Since clip can be combined with opacity, the result could be incorrect
    // for overlapping clip children. To fix this we use the SOURCE operator
    // instead of the default OVER.
    ict.setOperator(CAIRO_OPERATOR_SOURCE);
    if (item->clip) {
        state = nr_arena_item_invoke_clip(ict, item->clip, *carea); // fixme: carea or area?
        if (state & NR_ARENA_ITEM_STATE_INVALID) {
            retstate = (item->state |= NR_ARENA_ITEM_STATE_INVALID);
            goto cleanup;
        }
    } else {
        // if there is no clipping path, fill the entire surface with alpha = opacity.
        ict.paint();
    }
    // reset back to default
    ict.setOperator(CAIRO_OPERATOR_OVER);

    // 2. Render the mask if present and compose it with the clipping path + opacity.
    if (item->mask) {
        ict.pushGroup();
        state = NR_ARENA_ITEM_VIRTUAL (item->mask, render) (ict, item->mask, *carea, flags);
        if (state & NR_ARENA_ITEM_STATE_INVALID) {
            retstate = (item->state |= NR_ARENA_ITEM_STATE_INVALID);
            goto cleanup;
        }
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
    state = NR_ARENA_ITEM_VIRTUAL (item, render) (ict, item, *carea, flags);
    if (state & NR_ARENA_ITEM_STATE_INVALID) {
        retstate = (item->state |= NR_ARENA_ITEM_STATE_INVALID);
        goto cleanup;
    }

    // 4. Apply filter.
    if (item->filter && filter) {
        item->filter->render(item, ct, ict);
        // Note that because the object was rendered to a group,
        // the internals of the filter need to use cairo_get_group_target()
        // instead of cairo_get_target().
    }

    // 5. Render object inside the composited mask + clip
    ict.popGroupToSource();
    ict.setOperator(CAIRO_OPERATOR_IN);
    ict.paint();

    // 6. Paint the completed rendering onto the base context (or into cache)
    if (item->render_cache && item->cache) {
        DrawingContext cachect(*item->cache);
        cachect.rectangle(area);
        cachect.clip();
        cachect.setOperator(CAIRO_OPERATOR_SOURCE);
        cachect.setSource(&intermediate);
        cachect.paint();
        item->cache->markClean(area);
    }
    ct.setSource(&intermediate);
    ct.paint();
    ct.setSource(0,0,0,0);
    // the call above is to clear a ref on the intermediate surface held by ct

    retstate = item->state | NR_ARENA_ITEM_STATE_RENDER;

    cleanup:
    return retstate;
}

unsigned int
nr_arena_item_invoke_clip (Inkscape::DrawingContext &ct, NRArenaItem *item, Geom::IntRect const &area)
{
    nr_return_val_if_fail (item != NULL, NR_ARENA_ITEM_STATE_INVALID);
    nr_return_val_if_fail (NR_IS_ARENA_ITEM (item),
                           NR_ARENA_ITEM_STATE_INVALID);

    unsigned retstate = 0;

    // don't bother if the object does not implement clipping (e.g. NRArenaImage)
    if (!((NRArenaItemClass *) NR_OBJECT_GET_CLASS (item))->clip)
        return retstate;

    if (item->visible && area.intersects(item->bbox)) {
        // The item used as the clipping path itself has a clipping path.
        // Render this item's clipping path onto a temporary surface, then composite it
        // with the item using the IN operator
        if (item->clip) {
            ct.pushAlphaGroup();
            {   Inkscape::DrawingContext::Save save(ct);
                ct.setSource(0,0,0,1);
                nr_arena_item_invoke_clip(ct, item->clip, area);
            }
            ct.pushAlphaGroup();
        }

        // rasterize the clipping path
        retstate = ((NRArenaItemClass *) NR_OBJECT_GET_CLASS (item))->
            clip (ct, item, area);
        
        if (item->clip) {
            ct.popGroupToSource();
            ct.setOperator(CAIRO_OPERATOR_IN);
            ct.paint();
            ct.popGroupToSource();
            ct.setOperator(CAIRO_OPERATOR_SOURCE);
            ct.paint();
        }
    }

    return retstate;
}

NRArenaItem *
nr_arena_item_invoke_pick (NRArenaItem *item, Geom::Point const &p, double delta,
                           unsigned int sticky)
{
    nr_return_val_if_fail (item != NULL, NULL);
    nr_return_val_if_fail (NR_IS_ARENA_ITEM (item), NULL);

    // Sometimes there's no BBOX in item->state, reason unknown (bug 992817); I made this not an assert to remove the warning
    if (!(item->state & NR_ARENA_ITEM_STATE_BBOX)
        || !(item->state & NR_ARENA_ITEM_STATE_PICK))
        return NULL;

    if (!sticky && !(item->visible && item->sensitive))
        return NULL;

    if (!item->bbox) return NULL;
    Geom::Rect expanded(*item->bbox);
    expanded.expandBy(delta);

    if (expanded.contains(p)) {
        if (((NRArenaItemClass *) NR_OBJECT_GET_CLASS (item))->pick)
            return ((NRArenaItemClass *) NR_OBJECT_GET_CLASS (item))->
                pick (item, p, delta, sticky);
    }

    return NULL;
}

void
nr_arena_item_request_update (NRArenaItem *item, unsigned int reset,
                              unsigned int propagate)
{
    nr_return_if_fail (item != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (item));
    nr_return_if_fail (!(reset & NR_ARENA_ITEM_STATE_INVALID));

    if (propagate && !item->propagate)
        item->propagate = TRUE;

    if (item->state & reset) {
        item->state &= ~reset;
        if (item->parent) {
            nr_arena_item_request_update (item->parent, reset, FALSE);
        } else {
            nr_arena_request_update (item->arena, item);
        }
    }
}

void
nr_arena_item_request_render (NRArenaItem *item)
{
    nr_return_if_fail (item != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (item));

    bool outline = (item->arena->rendermode == Inkscape::RENDERMODE_OUTLINE);
    Geom::OptIntRect dirty = outline ? item->bbox : item->drawbox;
    if (!dirty) return;

    // dirty the caches of all parents
    for (NRArenaItem *i = item; i; i = i->parent) {
        if (i->render_cache && i->cache) {
            i->cache->markDirty(*dirty);
        }
    }

    nr_arena_request_render_rect (item->arena, dirty);
}

/* Public */

NRArenaItem *
nr_arena_item_unparent (NRArenaItem *item)
{
    nr_return_val_if_fail (item != NULL, NULL);
    nr_return_val_if_fail (NR_IS_ARENA_ITEM (item), NULL);

    nr_arena_item_request_render (item);

    if (item->parent) {
        nr_arena_item_remove_child (item->parent, item);
    }

    return NULL;
}

void
nr_arena_item_append_child (NRArenaItem *parent, NRArenaItem *child)
{
    nr_return_if_fail (parent != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (parent));
    nr_return_if_fail (child != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (child));
    nr_return_if_fail (parent->arena == child->arena);
    nr_return_if_fail (child->parent == NULL);
    nr_return_if_fail (child->prev == NULL);
    nr_return_if_fail (child->next == NULL);

    nr_arena_item_add_child (parent, child, nr_arena_item_last_child (parent));
}

void
nr_arena_item_set_transform (NRArenaItem *item, Geom::Affine const &transform)
{
    Geom::Affine const t (transform);
    nr_arena_item_set_transform (item, &t);
}

void
nr_arena_item_set_transform (NRArenaItem *item, Geom::Affine const *transform)
{
    nr_return_if_fail (item != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (item));

    if (!transform && !item->transform)
        return;

    const Geom::Affine *md = (item->transform) ? item->transform : &GEOM_MATRIX_IDENTITY;
    const Geom::Affine *ms = (transform) ? transform : &GEOM_MATRIX_IDENTITY;

    if (!Geom::matrix_equalp(*md, *ms, NR_EPSILON)) {
        // mark the area where the object was for redraw.
        nr_arena_item_request_render (item);
        if (!transform || transform->isIdentity()) {
            /* Set to identity affine */
            item->transform = NULL;
        } else {
            if (!item->transform)
                item->transform = new (GC::ATOMIC) Geom::Affine ();
            *item->transform = *transform;
        }
        // when update is called, the area where the object was moved
        // will be redrawn as well
        nr_arena_item_request_update (item, NR_ARENA_ITEM_STATE_ALL, TRUE);
    }
}

void
nr_arena_item_set_opacity (NRArenaItem *item, double opacity)
{
    nr_return_if_fail (item != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (item));

    nr_arena_item_request_render (item);

    item->opacity = (unsigned int) (opacity * 255.9999);
}

void
nr_arena_item_set_sensitive (NRArenaItem *item, unsigned int sensitive)
{
    nr_return_if_fail (item != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (item));

    /* fixme: mess with pick/repick... */

    item->sensitive = sensitive;
}

void
nr_arena_item_set_visible (NRArenaItem *item, unsigned int visible)
{
    nr_return_if_fail (item != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (item));

    item->visible = visible;

    nr_arena_item_request_render (item);
}

void
nr_arena_item_set_clip (NRArenaItem *item, NRArenaItem *clip)
{
    nr_return_if_fail (item != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (item));
    nr_return_if_fail (!clip || NR_IS_ARENA_ITEM (clip));

    if (clip != item->clip) {
        nr_arena_item_request_render (item);
        if (item->clip)
            item->clip = nr_arena_item_detach (item, item->clip);
        if (clip)
            item->clip = nr_arena_item_attach (item, clip, NULL, NULL);
        nr_arena_item_request_update (item, NR_ARENA_ITEM_STATE_ALL, TRUE);
    }
}

void
nr_arena_item_set_mask (NRArenaItem *item, NRArenaItem *mask)
{
    nr_return_if_fail (item != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (item));
    nr_return_if_fail (!mask || NR_IS_ARENA_ITEM (mask));

    if (mask != item->mask) {
        nr_arena_item_request_render (item);
        if (item->mask)
            item->mask = nr_arena_item_detach (item, item->mask);
        if (mask)
            item->mask = nr_arena_item_attach (item, mask, NULL, NULL);
        nr_arena_item_request_update (item, NR_ARENA_ITEM_STATE_ALL, TRUE);
    }
}

void
nr_arena_item_set_order (NRArenaItem *item, int order)
{
    nr_return_if_fail (item != NULL);
    nr_return_if_fail (NR_IS_ARENA_ITEM (item));

    if (!item->parent)
        return;

    NRArenaItem *children = nr_arena_item_children (item->parent);

    NRArenaItem *ref = NULL;
    int pos = 0;
    for (NRArenaItem *child = children; child != NULL; child = child->next) {
        if (pos >= order)
            break;
        if (child != item) {
            ref = child;
            pos += 1;
        }
    }

    nr_arena_item_set_child_position (item->parent, item, ref);
}

void
nr_arena_item_set_item_bbox (NRArenaItem *item, Geom::OptRect const &bbox)
{
    nr_return_if_fail(item != NULL);
    nr_return_if_fail(NR_IS_ARENA_ITEM(item));

    item->item_bbox = bbox;
}

void
nr_arena_item_set_cache (NRArenaItem *item, bool cache)
{
    if (cache) {
        item->render_cache = TRUE;
        item->arena->cached_items.insert(item);
    } else {
        item->render_cache = FALSE;
        item->arena->cached_items.erase(item);
    }
    nr_arena_item_request_update(item, NR_ARENA_ITEM_STATE_ALL, FALSE);
}

/** Returns a background image for use with filter effects. */
NRPixBlock *nr_arena_item_get_background(NRArenaItem const * /*item*/)
{
    return NULL;
}

/* Helpers */

NRArenaItem *
nr_arena_item_attach (NRArenaItem *parent, NRArenaItem *child,
                      NRArenaItem *prev, NRArenaItem *next)
{
    nr_return_val_if_fail (parent != NULL, NULL);
    nr_return_val_if_fail (NR_IS_ARENA_ITEM (parent), NULL);
    nr_return_val_if_fail (child != NULL, NULL);
    nr_return_val_if_fail (NR_IS_ARENA_ITEM (child), NULL);
    nr_return_val_if_fail (child->parent == NULL, NULL);
    nr_return_val_if_fail (child->prev == NULL, NULL);
    nr_return_val_if_fail (child->next == NULL, NULL);
    nr_return_val_if_fail (!prev || NR_IS_ARENA_ITEM (prev), NULL);
    nr_return_val_if_fail (!prev || (prev->parent == parent), NULL);
    nr_return_val_if_fail (!prev || (prev->next == next), NULL);
    nr_return_val_if_fail (!next || NR_IS_ARENA_ITEM (next), NULL);
    nr_return_val_if_fail (!next || (next->parent == parent), NULL);
    nr_return_val_if_fail (!next || (next->prev == prev), NULL);

    child->parent = parent;
    child->prev = prev;
    child->next = next;

    if (prev)
        prev->next = child;
    if (next)
        next->prev = child;

    return child;
}

NRArenaItem *
nr_arena_item_detach (NRArenaItem *parent, NRArenaItem *child)
{
    nr_return_val_if_fail (parent != NULL, NULL);
    nr_return_val_if_fail (NR_IS_ARENA_ITEM (parent), NULL);
    nr_return_val_if_fail (child != NULL, NULL);
    nr_return_val_if_fail (NR_IS_ARENA_ITEM (child), NULL);
    nr_return_val_if_fail (child->parent == parent, NULL);

    NRArenaItem *prev = child->prev;
    NRArenaItem *next = child->next;

    child->parent = NULL;
    child->prev = NULL;
    child->next = NULL;

    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;

    return next;
}

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
