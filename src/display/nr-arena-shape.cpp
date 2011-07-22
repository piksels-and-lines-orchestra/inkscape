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

#include <cairo.h>
#include <glib.h>
#include <fenv.h>
#include <typeinfo>

#include <2geom/curves.h>
#include <2geom/pathvector.h>
#include <2geom/svg-path.h>
#include <2geom/svg-path-parser.h>
#include "display/cairo-utils.h"
#include "display/canvas-arena.h"
#include "display/canvas-bpath.h"
#include "display/curve.h"
#include "display/drawing-context.h"
#include "display/nr-arena.h"
#include "display/nr-arena-shape.h"
#include "display/nr-filter.h"
#include "helper/geom-curves.h"
#include "helper/geom.h"
#include "libnr/nr-convert2geom.h"
#include "preferences.h"
#include "sp-filter.h"
#include "sp-filter-reference.h"
#include "style.h"
#include "svg/svg.h"

static void nr_arena_shape_class_init(NRArenaShapeClass *klass);
static void nr_arena_shape_init(NRArenaShape *shape);
static void nr_arena_shape_finalize(NRObject *object);

static NRArenaItem *nr_arena_shape_children(NRArenaItem *item);
static void nr_arena_shape_add_child(NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref);
static void nr_arena_shape_remove_child(NRArenaItem *item, NRArenaItem *child);
static void nr_arena_shape_set_child_position(NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref);

static guint nr_arena_shape_update(NRArenaItem *item, Geom::IntRect const &area, NRGC *gc, guint state, guint reset);
static unsigned int nr_arena_shape_render(Inkscape::DrawingContext &ct, NRArenaItem *item, Geom::IntRect const &area, unsigned int flags);
static guint nr_arena_shape_clip(Inkscape::DrawingContext &ct, NRArenaItem *item, Geom::IntRect const &area);
static NRArenaItem *nr_arena_shape_pick(NRArenaItem *item, Geom::Point const &p, double delta, unsigned int sticky);

static NRArenaItemClass *shape_parent_class;

NRType
nr_arena_shape_get_type(void)
{
    static NRType type = 0;
    if (!type) {
        type = nr_object_register_type(NR_TYPE_ARENA_ITEM,
                                       "NRArenaShape",
                                       sizeof(NRArenaShapeClass),
                                       sizeof(NRArenaShape),
                                       (void (*)(NRObjectClass *)) nr_arena_shape_class_init,
                                       (void (*)(NRObject *)) nr_arena_shape_init);
    }
    return type;
}

static void
nr_arena_shape_class_init(NRArenaShapeClass *klass)
{
    NRObjectClass *object_class;
    NRArenaItemClass *item_class;

    object_class = (NRObjectClass *) klass;
    item_class = (NRArenaItemClass *) klass;

    shape_parent_class = (NRArenaItemClass *)  ((NRObjectClass *) klass)->parent;

    object_class->finalize = nr_arena_shape_finalize;
    object_class->cpp_ctor = NRObject::invoke_ctor<NRArenaShape>;

    item_class->children = nr_arena_shape_children;
    item_class->add_child = nr_arena_shape_add_child;
    item_class->set_child_position = nr_arena_shape_set_child_position;
    item_class->remove_child = nr_arena_shape_remove_child;
    item_class->update = nr_arena_shape_update;
    item_class->render = nr_arena_shape_render;
    item_class->clip = nr_arena_shape_clip;
    item_class->pick = nr_arena_shape_pick;
}

/**
 * Initializes the arena shape, setting all parameters to null, 0, false,
 * or other defaults
 */
static void
nr_arena_shape_init(NRArenaShape *shape)
{
    shape->curve = NULL;
    shape->style = NULL;
    shape->markers = NULL;
    shape->last_pick = NULL;
    shape->repick_after = 0;
}

static void
nr_arena_shape_finalize(NRObject *object)
{
    NRArenaShape *shape = (NRArenaShape *) object;

    if (shape->style) sp_style_unref(shape->style);
    if (shape->curve) shape->curve->unref();
    shape->last_pick = NULL;

    ((NRObjectClass *) shape_parent_class)->finalize(object);
}

/**
 * Retrieves the markers from the item
 */
static NRArenaItem *
nr_arena_shape_children(NRArenaItem *item)
{
    NRArenaShape *shape = (NRArenaShape *) item;

    return shape->markers;
}

/**
 * Attaches child to item, and if ref is not NULL, sets it and ref->next as
 * the prev and next items.  If ref is NULL, then it sets the item's markers
 * as the next items.
 */
static void
nr_arena_shape_add_child(NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref)
{
    NRArenaShape *shape = (NRArenaShape *) item;

    if (!ref) {
        shape->markers = nr_arena_item_attach(item, child, NULL, shape->markers);
    } else {
        ref->next = nr_arena_item_attach(item, child, ref, ref->next);
    }

    nr_arena_item_request_update(item, NR_ARENA_ITEM_STATE_ALL, FALSE);
}

/**
 * Removes child from the shape.  If there are no prev items in 
 * the child, it sets items' markers to the next item in the child.
 */
static void
nr_arena_shape_remove_child(NRArenaItem *item, NRArenaItem *child)
{
    NRArenaShape *shape = (NRArenaShape *) item;

    if (child->prev) {
        nr_arena_item_detach(item, child);
    } else {
        shape->markers = nr_arena_item_detach(item, child);
    }

    nr_arena_item_request_update(item, NR_ARENA_ITEM_STATE_ALL, FALSE);
}

/**
 * Detaches child from item, and if there are no previous items in child, it 
 * sets item's markers to the child.  It then attaches the child back onto the item.
 * If ref is null, it sets the markers to be the next item, otherwise it uses
 * the next/prev items in ref.
 */
static void
nr_arena_shape_set_child_position(NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref)
{
    NRArenaShape *shape = (NRArenaShape *) item;

    if (child->prev) {
        nr_arena_item_detach(item, child);
    } else {
        shape->markers = nr_arena_item_detach(item, child);
    }

    if (!ref) {
        shape->markers = nr_arena_item_attach(item, child, NULL, shape->markers);
    } else {
        ref->next = nr_arena_item_attach(item, child, ref, ref->next);
    }

    nr_arena_item_request_render(child);
}

/**
 * Updates the arena shape 'item' and all of its children, including the markers.
 */
static guint
nr_arena_shape_update(NRArenaItem *item, Geom::IntRect const &area, NRGC *gc, guint state, guint reset)
{
    Geom::OptRect boundingbox;

    NRArenaShape *shape = NR_ARENA_SHAPE(item);

    unsigned int beststate = NR_ARENA_ITEM_STATE_ALL;

    // update markers
    unsigned int newstate;
    for (NRArenaItem *child = shape->markers; child != NULL; child = child->next) {
        newstate = nr_arena_item_invoke_update(child, area, gc, state, reset);
        beststate = beststate & newstate;
    }

    if (!(state & NR_ARENA_ITEM_STATE_RENDER)) {
        /* We do not have to create rendering structures */
        if (state & NR_ARENA_ITEM_STATE_BBOX) {
            if (shape->curve) {
                boundingbox = bounds_exact_transformed(shape->curve->get_pathvector(), gc->transform);
                if (boundingbox) {
                    item->bbox = boundingbox->roundOutwards();
                } else {
                    item->bbox = Geom::OptIntRect();
                }
            }
            if (beststate & NR_ARENA_ITEM_STATE_BBOX) {
                for (NRArenaItem *child = shape->markers; child != NULL; child = child->next) {
                    item->bbox.unionWith(child->bbox);
                }
            }
        }
        return (state | item->state);
    }

    boundingbox = Geom::OptRect();

    bool outline = (NR_ARENA_ITEM(shape)->arena->rendermode == Inkscape::RENDERMODE_OUTLINE);

    // clear Cairo data to force update
    shape->nrstyle.update();

    if (shape->curve) {
        boundingbox = bounds_exact_transformed(shape->curve->get_pathvector(), gc->transform);

        if (boundingbox && (shape->nrstyle.stroke.type != NRStyle::PAINT_NONE || outline)) {
            float width, scale;
            scale = gc->transform.descrim();
            width = MAX(0.125, shape->nrstyle.stroke_width * scale);
            if ( fabs(shape->nrstyle.stroke_width * scale) > 0.01 ) { // FIXME: this is always true
                boundingbox->expandBy(width);
            }
            // those pesky miters, now
            float miterMax = width * shape->nrstyle.miter_limit;
            if ( miterMax > 0.01 ) {
                // grunt mode. we should compute the various miters instead
                // (one for each point on the curve)
                boundingbox->expandBy(miterMax);
            }
        }
    }

    item->bbox = boundingbox ? boundingbox->roundOutwards() : Geom::OptIntRect();

    if (!shape->curve || 
        !shape->style ||
        shape->curve->is_empty() ||
        (( shape->nrstyle.fill.type != NRStyle::PAINT_NONE ) &&
         ( shape->nrstyle.stroke.type != NRStyle::PAINT_NONE && !outline) ))
    {
        //item->bbox = shape->approx_bbox;
        return NR_ARENA_ITEM_STATE_ALL;
    }

    if (beststate & NR_ARENA_ITEM_STATE_BBOX) {
        for (NRArenaItem *child = shape->markers; child != NULL; child = child->next) {
            item->bbox.unionWith(child->bbox);
        }
    }

    return NR_ARENA_ITEM_STATE_ALL;
}

// cairo outline rendering:
static unsigned int
cairo_arena_shape_render_outline(Inkscape::DrawingContext &ct, NRArenaItem *item, Geom::IntRect const &/*area*/)
{
    NRArenaShape *shape = NR_ARENA_SHAPE(item);

    guint32 rgba = NR_ARENA_ITEM(shape)->arena->outlinecolor;

    {   Inkscape::DrawingContext::Save save(ct);
        ct.transform(shape->ctm);
        ct.path(shape->curve->get_pathvector());
    }
    {   Inkscape::DrawingContext::Save save(ct);
        ct.setSource(rgba);
        ct.setLineWidth(0.5);
        ct.setTolerance(1.25);
        ct.stroke();
    }

    return item->state;
}

/**
 * Renders the item.  Markers are just composed into the parent buffer.
 */
static unsigned int
nr_arena_shape_render(Inkscape::DrawingContext &ct, NRArenaItem *item, Geom::IntRect const &area, unsigned int flags)
{
    NRArenaShape *shape = NR_ARENA_SHAPE(item);

    if (!shape->curve) return item->state;
    if (!shape->style) return item->state;

    // skip if not within bounding box
    if (!area.intersects(item->bbox)) {
        return item->state;
    }

    bool outline = (NR_ARENA_ITEM(shape)->arena->rendermode == Inkscape::RENDERMODE_OUTLINE);

    if (outline) {
        // cairo outline rendering
        unsigned int ret = cairo_arena_shape_render_outline (ct, item, area);
        if (ret & NR_ARENA_ITEM_STATE_INVALID) return ret;
    } else {
        bool has_stroke, has_fill;
        // we assume the context has no path
        Inkscape::DrawingContext::Save save(ct);
        ct.transform(shape->ctm);

        // update fill and stroke paints.
        // this cannot be done during nr_arena_shape_update, because we need a Cairo context
        // to render svg:pattern
        has_fill   = shape->nrstyle.prepareFill(ct, shape->paintbox);
        has_stroke = shape->nrstyle.prepareStroke(ct, shape->paintbox);
        has_stroke &= (shape->nrstyle.stroke_width != 0);

        if (has_fill || has_stroke) {
            // TODO: remove segments outside of bbox when no dashes present
            ct.path(shape->curve->get_pathvector());
            if (has_fill) {
                shape->nrstyle.applyFill(ct);
                ct.fillPreserve();
            }
            if (has_stroke) {
                shape->nrstyle.applyStroke(ct);
                ct.strokePreserve();
            }
            ct.newPath(); // clear path
        } // has fill or stroke pattern
    }

    // marker rendering
    for (NRArenaItem *child = shape->markers; child != NULL; child = child->next) {
        unsigned int ret = nr_arena_item_invoke_render(ct, child, area, flags);
        if (ret & NR_ARENA_ITEM_STATE_INVALID) return ret;
    }

    return item->state;
}


static guint nr_arena_shape_clip(Inkscape::DrawingContext &ct, NRArenaItem *item, Geom::IntRect const &/*area*/)
{
    NRArenaShape *shape = NR_ARENA_SHAPE(item);
    if (!shape->curve) {
        return item->state;
    }

    Inkscape::DrawingContext::Save save(ct);
    // handle clip-rule
    if (shape->style) {
        if (shape->style->clip_rule.computed == SP_WIND_RULE_EVENODD) {
            ct.setFillRule(CAIRO_FILL_RULE_EVEN_ODD);
        } else {
            ct.setFillRule(CAIRO_FILL_RULE_WINDING);
        }
    }
    ct.transform(shape->ctm);
    ct.path(shape->curve->get_pathvector());
    ct.fill();

    return item->state;
}

static NRArenaItem *
nr_arena_shape_pick(NRArenaItem *item, Geom::Point const &p, double delta, unsigned int /*sticky*/)
{
    NRArenaShape *shape = NR_ARENA_SHAPE(item);

    if (shape->repick_after > 0)
        shape->repick_after--;

    if (shape->repick_after > 0) // we are a slow, huge path. skip this pick, returning what was returned last time
        return shape->last_pick;

    if (!shape->curve) return NULL;
    if (!shape->style) return NULL;

    bool outline = (NR_ARENA_ITEM(shape)->arena->rendermode == Inkscape::RENDERMODE_OUTLINE);

    if (SP_SCALE24_TO_FLOAT(shape->style->opacity.value) == 0 && !outline) 
        // fully transparent, no pick unless outline mode
        return NULL;

    GTimeVal tstart, tfinish;
    g_get_current_time (&tstart);

    double width;
    if (outline) {
        width = 0.5;
    } else if (shape->nrstyle.stroke.type != NRStyle::PAINT_NONE && shape->nrstyle.stroke.opacity > 1e-3) {
        float const scale = shape->ctm.descrim();
        width = MAX(0.125, shape->nrstyle.stroke_width * scale) / 2;
    } else {
        width = 0;
    }

    double dist = Geom::infinity();
    int wind = 0;
    bool needfill = (shape->nrstyle.fill.type != NRStyle::PAINT_NONE 
             && shape->nrstyle.fill.opacity > 1e-3 && !outline);

    if (item->arena->canvasarena) {
        Geom::Rect viewbox = item->arena->canvasarena->item.canvas->getViewbox();
        viewbox.expandBy (width);
        pathv_matrix_point_bbox_wind_distance(shape->curve->get_pathvector(), shape->ctm, p, NULL, needfill? &wind : NULL, &dist, 0.5, &viewbox);
    } else {
        pathv_matrix_point_bbox_wind_distance(shape->curve->get_pathvector(), shape->ctm, p, NULL, needfill? &wind : NULL, &dist, 0.5, NULL);
    }

    g_get_current_time (&tfinish);
    glong this_pick = (tfinish.tv_sec - tstart.tv_sec) * 1000000 + (tfinish.tv_usec - tstart.tv_usec);
    //g_print ("pick time %lu\n", this_pick);

    if (this_pick > 10000) { // slow picking, remember to skip several new picks
        shape->repick_after = this_pick / 5000;
    }

    // covered by fill?
    if (needfill) {
        if (!shape->style->fill_rule.computed) {
            if (wind != 0) {
                shape->last_pick = item;
                return item;
            }
        } else {
            if (wind & 0x1) {
                shape->last_pick = item;
                return item;
            }
        }
    }

    // close to the edge, as defined by strokewidth and delta?
    // this ignores dashing (as if the stroke is solid) and always works as if caps are round
    if (needfill || width > 0) { // if either fill or stroke visible,
        if ((dist - width) < delta) {
            shape->last_pick = item;
            return item;
        }
    }

    // if not picked on the shape itself, try its markers
    for (NRArenaItem *child = shape->markers; child != NULL; child = child->next) {
        NRArenaItem *ret = nr_arena_item_invoke_pick(child, p, delta, 0);
        if (ret) {
            shape->last_pick = item;
            return item;
        }
    }

    shape->last_pick = NULL;
    return NULL;
}

/**
 *
 *  Requests a render of the shape, then if the shape is already a curve it
 *  unrefs the old curve; if the new curve is valid it creates a copy of the
 *  curve and adds it to the shape.  Finally, it requests an update of the
 *  arena for the shape.
 */
void nr_arena_shape_set_path(NRArenaShape *shape, SPCurve *curve, bool /*justTrans*/)
{
    g_return_if_fail(shape != NULL);
    g_return_if_fail(NR_IS_ARENA_SHAPE(shape));

    nr_arena_item_request_render(NR_ARENA_ITEM(shape));

    if (shape->curve) {
        shape->curve->unref();
        shape->curve = NULL;
    }

    if (curve) {
        shape->curve = curve;
        curve->ref();
    }

    nr_arena_item_request_update(NR_ARENA_ITEM(shape), NR_ARENA_ITEM_STATE_ALL, FALSE);
}

/** nr_arena_shape_set_style
 *
 * Unrefs any existing style and ref's to the given one, then requests an update of the arena
 */
void
nr_arena_shape_set_style(NRArenaShape *shape, SPStyle *style)
{
    g_return_if_fail(shape != NULL);
    g_return_if_fail(NR_IS_ARENA_SHAPE(shape));
    g_return_if_fail(style != NULL);

    sp_style_ref(style);
    if (shape->style) sp_style_unref(shape->style);
    shape->style = style;

    shape->nrstyle.set(style);

    //if shape has a filter
    if (style->filter.set && style->getFilter()) {
        if (!shape->filter) {
            int primitives = sp_filter_primitive_count(SP_FILTER(style->getFilter()));
            shape->filter = new Inkscape::Filters::Filter(primitives);
        }
        sp_filter_build_renderer(SP_FILTER(style->getFilter()), shape->filter);
    } else {
        //no filter set for this shape
        delete shape->filter;
        shape->filter = NULL;
    }

    nr_arena_item_request_update(shape, NR_ARENA_ITEM_STATE_ALL, FALSE);
}

void
nr_arena_shape_set_paintbox(NRArenaShape *shape, NRRect const *pbox)
{
    g_return_if_fail(shape != NULL);
    g_return_if_fail(NR_IS_ARENA_SHAPE(shape));
    g_return_if_fail(pbox != NULL);

    shape->paintbox = pbox->upgrade_2geom();

    nr_arena_item_request_update(shape, NR_ARENA_ITEM_STATE_ALL, FALSE);
}

void NRArenaShape::setPaintBox(Geom::Rect const &pbox)
{
    paintbox = pbox;

    nr_arena_item_request_update(this, NR_ARENA_ITEM_STATE_ALL, FALSE);
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
