#define __NR_ARENA_SHAPE_C__

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

#include <2geom/svg-path.h>
#include <2geom/svg-path-parser.h>
#include <display/canvas-arena.h>
#include <display/nr-arena.h>
#include <display/nr-arena-shape.h>
#include "display/curve.h"
#include <libnr/nr-pixops.h>
#include <libnr/nr-blit.h>
#include <libnr/nr-convert2geom.h>
#include <2geom/pathvector.h>
#include <2geom/curves.h>
#include <livarot/Path.h>
#include <livarot/float-line.h>
#include <livarot/int-line.h>
#include <style.h>
#include "inkscape-cairo.h"
#include "helper/geom.h"
#include "helper/geom-curves.h"
#include "sp-filter.h"
#include "sp-filter-reference.h"
#include "display/nr-filter.h"
#include <typeinfo>
#include <cairo.h>
#include "preferences.h"

#include <glib.h>
#include "svg/svg.h"
#include <fenv.h>

//int  showRuns=0;
void nr_pixblock_render_shape_mask_or(NRPixBlock &m,Shape* theS);

static void nr_arena_shape_class_init(NRArenaShapeClass *klass);
static void nr_arena_shape_init(NRArenaShape *shape);
static void nr_arena_shape_finalize(NRObject *object);

static NRArenaItem *nr_arena_shape_children(NRArenaItem *item);
static void nr_arena_shape_add_child(NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref);
static void nr_arena_shape_remove_child(NRArenaItem *item, NRArenaItem *child);
static void nr_arena_shape_set_child_position(NRArenaItem *item, NRArenaItem *child, NRArenaItem *ref);

static guint nr_arena_shape_update(NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset);
static unsigned int nr_arena_shape_render(cairo_t *ct, NRArenaItem *item, NRRectL *area, NRPixBlock *pb, unsigned int flags);
static guint nr_arena_shape_clip(cairo_t *ct, NRArenaItem *item, NRRectL *area);
static NRArenaItem *nr_arena_shape_pick(NRArenaItem *item, Geom::Point p, double delta, unsigned int sticky);

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
    shape->paintbox.x0 = shape->paintbox.y0 = 0.0F;
    shape->paintbox.x1 = shape->paintbox.y1 = 256.0F;

    shape->ctm.setIdentity();

    shape->delayed_shp = false;

    shape->fill_pattern = NULL;
    shape->stroke_pattern = NULL;
    shape->path = NULL;

    shape->approx_bbox.x0 = shape->approx_bbox.y0 = 0;
    shape->approx_bbox.x1 = shape->approx_bbox.y1 = 0;

    shape->markers = NULL;

    shape->last_pick = NULL;
    shape->repick_after = 0;
}

static void
nr_arena_shape_finalize(NRObject *object)
{
    NRArenaShape *shape = (NRArenaShape *) object;

    if (shape->fill_pattern) cairo_pattern_destroy(shape->fill_pattern);
    if (shape->stroke_pattern) cairo_pattern_destroy(shape->stroke_pattern);
    if (shape->path) cairo_path_destroy(shape->path);

    if (shape->style) sp_style_unref(shape->style);
    if (shape->curve) shape->curve->unref();

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
nr_arena_shape_update(NRArenaItem *item, NRRectL *area, NRGC *gc, guint state, guint reset)
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
        shape->ctm = gc->transform;
        if (state & NR_ARENA_ITEM_STATE_BBOX) {
            if (shape->curve) {
                boundingbox = bounds_exact_transformed(shape->curve->get_pathvector(), gc->transform);
                if (boundingbox) {
                    item->bbox.x0 = static_cast<NR::ICoord>(floor((*boundingbox)[0][0])); // Floor gives the coordinate in which the point resides
                    item->bbox.y0 = static_cast<NR::ICoord>(floor((*boundingbox)[1][0]));
                    item->bbox.x1 = static_cast<NR::ICoord>(ceil ((*boundingbox)[0][1])); // Ceil gives the first coordinate beyond the point
                    item->bbox.y1 = static_cast<NR::ICoord>(ceil ((*boundingbox)[1][1]));
                } else {
                    item->bbox = NR_RECT_L_EMPTY;
                }
            }
            if (beststate & NR_ARENA_ITEM_STATE_BBOX) {
                for (NRArenaItem *child = shape->markers; child != NULL; child = child->next) {
                    nr_rect_l_union(&item->bbox, &item->bbox, &child->bbox);
                }
            }
        }
        return (state | item->state);
    }

    shape->delayed_shp=true;
    shape->ctm = gc->transform;
    boundingbox = Geom::OptRect();

    bool outline = (NR_ARENA_ITEM(shape)->arena->rendermode == Inkscape::RENDERMODE_OUTLINE);

    // clear Cairo data to force update
    if (shape->fill_pattern) {
        cairo_pattern_destroy(shape->fill_pattern);
        shape->fill_pattern = NULL;
    }
    if (shape->stroke_pattern) {
        cairo_pattern_destroy(shape->stroke_pattern);
        shape->stroke_pattern = NULL;
    }
    if (shape->path) {
        cairo_path_destroy(shape->path);
        shape->path = NULL;
    }

    if (shape->curve) {
        boundingbox = bounds_exact_transformed(shape->curve->get_pathvector(), gc->transform);

        if (boundingbox && (shape->_stroke.paint.type() != NRArenaShape::Paint::NONE || outline)) {
            float width, scale;
            scale = gc->transform.descrim();
            width = MAX(0.125, shape->_stroke.width * scale);
            if ( fabs(shape->_stroke.width * scale) > 0.01 ) {
                boundingbox->expandBy(width);
            }
            // those pesky miters, now
            float miterMax=width*shape->_stroke.mitre_limit;
            if ( miterMax > 0.01 ) {
                // grunt mode. we should compute the various miters instead (one for each point on the curve)
                boundingbox->expandBy(miterMax);
            }
        }
    }

    /// \todo  just write item->bbox = boundingbox
    if (boundingbox) {
        shape->approx_bbox.x0 = static_cast<NR::ICoord>(floor((*boundingbox)[0][0]));
        shape->approx_bbox.y0 = static_cast<NR::ICoord>(floor((*boundingbox)[1][0]));
        shape->approx_bbox.x1 = static_cast<NR::ICoord>(ceil ((*boundingbox)[0][1]));
        shape->approx_bbox.y1 = static_cast<NR::ICoord>(ceil ((*boundingbox)[1][1]));
    } else {
        shape->approx_bbox = NR_RECT_L_EMPTY;
    }
    if ( area && nr_rect_l_test_intersect_ptr(area, &shape->approx_bbox) ) shape->delayed_shp=false;

    // TODO: compute a better bounding box that respects miters
    item->bbox = shape->approx_bbox;

    if (!shape->curve || 
        !shape->style ||
        shape->curve->is_empty() ||
        (( shape->_fill.paint.type() == NRArenaShape::Paint::NONE ) &&
         ( shape->_stroke.paint.type() == NRArenaShape::Paint::NONE && !outline) ))
    {
        //item->bbox = shape->approx_bbox;
        return NR_ARENA_ITEM_STATE_ALL;
    }

    if (beststate & NR_ARENA_ITEM_STATE_BBOX) {
        for (NRArenaItem *child = shape->markers; child != NULL; child = child->next) {
            nr_rect_l_union(&item->bbox, &item->bbox, &child->bbox);
        }
    }

    return NR_ARENA_ITEM_STATE_ALL;
}

// cairo outline rendering:
static unsigned int
cairo_arena_shape_render_outline(cairo_t *ct, NRArenaItem *item, Geom::OptRect area)
{
    NRArenaShape *shape = NR_ARENA_SHAPE(item);

    if (!ct) 
        return item->state;

    guint32 rgba = NR_ARENA_ITEM(shape)->arena->outlinecolor;
    // FIXME: we use RGBA buffers but cairo writes BGRA (on i386), so we must cheat 
    // by setting color channels in the "wrong" order
    cairo_set_source_rgba(ct, SP_RGBA32_B_F(rgba), SP_RGBA32_G_F(rgba), SP_RGBA32_R_F(rgba), SP_RGBA32_A_F(rgba));

    cairo_set_line_width(ct, 0.5);
    cairo_set_tolerance(ct, 1.25); // low quality, but good enough for outline mode
    cairo_new_path(ct);

    feed_pathvector_to_cairo (ct, shape->curve->get_pathvector(), shape->ctm, area, true, 0);

    cairo_stroke(ct);

    return item->state;
}

/**
 * Renders the item.  Markers are just composed into the parent buffer.
 */
static unsigned int
nr_arena_shape_render(cairo_t *ct, NRArenaItem *item, NRRectL *area, NRPixBlock *pb, unsigned int flags)
{
    NRArenaShape *shape = NR_ARENA_SHAPE(item);

    if (!shape->curve) return item->state;
    if (!shape->style) return item->state;
    if (!ct) return item->state;

    // skip if not within bounding box
    if (!nr_rect_l_test_intersect_ptr(area, &item->bbox)) {
        return item->state;
    }

    bool outline = (NR_ARENA_ITEM(shape)->arena->rendermode == Inkscape::RENDERMODE_OUTLINE);
    //bool print_colors_preview = (NR_ARENA_ITEM(shape)->arena->rendermode == Inkscape::RENDERMODE_PRINT_COLORS_PREVIEW);

    if (outline) { // cairo outline rendering

        pb->empty = FALSE;
        unsigned int ret = cairo_arena_shape_render_outline (ct, item, to_2geom((&pb->area)->upgrade()));
        if (ret & NR_ARENA_ITEM_STATE_INVALID) return ret;

    } else {
        SPStyle const *style = shape->style;

        // set up context and feed path
        float opacity = SP_SCALE24_TO_FLOAT(shape->style->opacity.value);
        bool needs_opacity = ((1.0 - opacity) >= 1e-3);

        // we assume the context has no path
        cairo_save(ct);
        cairo_translate(ct, -area->x0, -area->y0);
        ink_cairo_transform(ct, shape->ctm);

        // update fill and stroke paints.
        // this cannot be done during nr_arena_shape_update, because we need a Cairo context
        // to render svg:pattern
        if (!shape->fill_pattern) {
            switch (shape->_fill.paint.type()) {
            case NRArenaShape::Paint::SERVER: {
                SPPaintServer *ps = shape->_fill.paint.server();
                shape->fill_pattern = sp_paint_server_create_pattern(ps, ct, &shape->paintbox, shape->_fill.opacity);
                } break;
            case NRArenaShape::Paint::COLOR: {
                SPColor const &c = shape->_fill.paint.color();
                shape->fill_pattern = cairo_pattern_create_rgba(
                    c.v.c[0], c.v.c[1], c.v.c[2], shape->_fill.opacity);
                } break;
            default: break;
            }
        }

        if (!shape->stroke_pattern) {
            switch (shape->_stroke.paint.type()) {
            case NRArenaShape::Paint::SERVER: {
                SPPaintServer *ps = shape->_stroke.paint.server();
                shape->stroke_pattern = sp_paint_server_create_pattern(ps, ct, &shape->paintbox, shape->_stroke.opacity);
                } break;
            case NRArenaShape::Paint::COLOR: {
                SPColor const &c = shape->_stroke.paint.color();
                shape->stroke_pattern = cairo_pattern_create_rgba(
                    c.v.c[0], c.v.c[1], c.v.c[2], shape->_stroke.opacity);
                } break;
            default: break;
            }
        }

        if (shape->fill_pattern || shape->stroke_pattern) {

            if (needs_opacity) {
                cairo_push_group(ct);
            }

            // TODO: remove segments outside of bbox when no dashes present
            feed_pathvector_to_cairo(ct, shape->curve->get_pathvector());

            if (shape->fill_pattern) {
                cairo_set_fill_rule(ct, shape->_fill.rule);
                cairo_set_source(ct, shape->fill_pattern);
                cairo_fill_preserve(ct);
            }

            if (shape->stroke_pattern) {
                cairo_set_line_width(ct, shape->_stroke.width);
                cairo_set_line_cap(ct, shape->_stroke.cap);
                cairo_set_line_join(ct, shape->_stroke.join);
                cairo_set_miter_limit (ct, style->stroke_miterlimit.value);

                // dashes
                if (style->stroke_dash.n_dash) {
                    cairo_set_dash (ct, style->stroke_dash.dash, style->stroke_dash.n_dash,
                        style->stroke_dash.offset);
                }
                cairo_set_source(ct, shape->stroke_pattern);
                cairo_stroke_preserve(ct);
            }
            cairo_new_path(ct); // clear path

            if (needs_opacity) {
                cairo_pop_group_to_source(ct);
                cairo_paint_with_alpha(ct, opacity);
            }
        } // has fill or stroke pattern

        cairo_restore(ct);

    } // non-cairo non-outline branch

    /* Render markers into parent buffer */
    for (NRArenaItem *child = shape->markers; child != NULL; child = child->next) {
        unsigned int ret = nr_arena_item_invoke_render(ct, child, area, pb, flags);
        if (ret & NR_ARENA_ITEM_STATE_INVALID) return ret;
    }

    return item->state;
}


static guint
nr_arena_shape_clip(cairo_t *ct, NRArenaItem *item, NRRectL *area)
{
    // NOTE: for now this is incorrect, because it doesn't honor clip-rule,
    // and will be incorrect for nested clipping paths.
    NRArenaShape *shape = NR_ARENA_SHAPE(item);
    if (!shape->curve) return item->state;

    cairo_save(ct);
    cairo_translate(ct, -area->x0, -area->y0);
    ink_cairo_transform(ct, shape->ctm);
    feed_pathvector_to_cairo(ct, shape->curve->get_pathvector());
    cairo_restore(ct);

    return item->state;
}

static NRArenaItem *
nr_arena_shape_pick(NRArenaItem *item, Geom::Point p, double delta, unsigned int /*sticky*/)
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
    } else if (shape->_stroke.paint.type() != NRArenaShape::Paint::NONE && shape->_stroke.opacity > 1e-3) {
        float const scale = shape->ctm.descrim();
        width = MAX(0.125, shape->_stroke.width * scale) / 2;
    } else {
        width = 0;
    }

    double dist = NR_HUGE;
    int wind = 0;
    bool needfill = (shape->_fill.paint.type() != NRArenaShape::Paint::NONE 
             && shape->_fill.opacity > 1e-3 && !outline);

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
void nr_arena_shape_set_path(NRArenaShape *shape, SPCurve *curve,bool justTrans)
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

void NRArenaShape::setFill(SPPaintServer *server) {
    _fill.paint.set(server);
    _invalidateCachedFill();
}

void NRArenaShape::setFill(SPColor const &color) {
    _fill.paint.set(color);
    _invalidateCachedFill();
}

void NRArenaShape::setFillOpacity(double opacity) {
    _fill.opacity = opacity;
    _invalidateCachedFill();
}

void NRArenaShape::setFillRule(cairo_fill_rule_t rule) {
    _fill.rule = rule;
    _invalidateCachedFill();
}

void NRArenaShape::setStroke(SPPaintServer *server) {
    _stroke.paint.set(server);
    _invalidateCachedStroke();
}

void NRArenaShape::setStroke(SPColor const &color) {
    _stroke.paint.set(color);
    _invalidateCachedStroke();
}

void NRArenaShape::setStrokeOpacity(double opacity) {
    _stroke.opacity = opacity;
    _invalidateCachedStroke();
}

void NRArenaShape::setStrokeWidth(double width) {
    _stroke.width = width;
    _invalidateCachedStroke();
}

void NRArenaShape::setMitreLimit(double limit) {
    _stroke.mitre_limit = limit;
    _invalidateCachedStroke();
}

void NRArenaShape::setLineCap(cairo_line_cap_t cap) {
    _stroke.cap = cap;
    _invalidateCachedStroke();
}

void NRArenaShape::setLineJoin(cairo_line_join_t join) {
    _stroke.join = join;
    _invalidateCachedStroke();
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

    if (style) sp_style_ref(style);
    if (shape->style) sp_style_unref(shape->style);
    shape->style = style;

    if ( style->fill.isPaintserver() ) {
        shape->setFill(style->getFillPaintServer());
    } else if ( style->fill.isColor() ) {
        shape->setFill(style->fill.value.color);
    } else if ( style->fill.isNone() ) {
        shape->setFill(NULL);
    } else {
        g_assert_not_reached();
    }
    shape->setFillOpacity(SP_SCALE24_TO_FLOAT(style->fill_opacity.value));
    switch (style->fill_rule.computed) {
        case SP_WIND_RULE_EVENODD: {
            shape->setFillRule(CAIRO_FILL_RULE_EVEN_ODD);
            break;
        }
        case SP_WIND_RULE_NONZERO: {
            shape->setFillRule(CAIRO_FILL_RULE_WINDING);
            break;
        }
        default: {
            g_assert_not_reached();
        }
    }

    if ( style->stroke.isPaintserver() ) {
        shape->setStroke(style->getStrokePaintServer());
    } else if ( style->stroke.isColor() ) {
        shape->setStroke(style->stroke.value.color);
    } else if ( style->stroke.isNone() ) {
        shape->setStroke(NULL);
    } else {
        g_assert_not_reached();
    }
    shape->setStrokeWidth(style->stroke_width.computed);
    shape->setStrokeOpacity(SP_SCALE24_TO_FLOAT(style->stroke_opacity.value));
    switch (style->stroke_linecap.computed) {
        case SP_STROKE_LINECAP_ROUND: {
            shape->setLineCap(CAIRO_LINE_CAP_ROUND);
            break;
        }
        case SP_STROKE_LINECAP_SQUARE: {
            shape->setLineCap(CAIRO_LINE_CAP_SQUARE);
            break;
        }
        case SP_STROKE_LINECAP_BUTT: {
            shape->setLineCap(CAIRO_LINE_CAP_BUTT);
            break;
        }
        default: {
            g_assert_not_reached();
        }
    }
    switch (style->stroke_linejoin.computed) {
        case SP_STROKE_LINEJOIN_ROUND: {
            shape->setLineJoin(CAIRO_LINE_JOIN_ROUND);
            break;
        }
        case SP_STROKE_LINEJOIN_BEVEL: {
            shape->setLineJoin(CAIRO_LINE_JOIN_BEVEL);
            break;
        }
        case SP_STROKE_LINEJOIN_MITER: {
            shape->setLineJoin(CAIRO_LINE_JOIN_MITER);
            break;
        }
        default: {
            g_assert_not_reached();
        }
    }
    shape->setMitreLimit(style->stroke_miterlimit.value);

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

    if ((pbox->x0 < pbox->x1) && (pbox->y0 < pbox->y1)) {
        shape->paintbox = *pbox;
    } else {
        /* fixme: We kill warning, although not sure what to do here (Lauris) */
        shape->paintbox.x0 = shape->paintbox.y0 = 0.0F;
        shape->paintbox.x1 = shape->paintbox.y1 = 256.0F;
    }

    nr_arena_item_request_update(shape, NR_ARENA_ITEM_STATE_ALL, FALSE);
}

void NRArenaShape::setPaintBox(Geom::Rect const &pbox)
{
    paintbox.x0 = pbox.min()[Geom::X];
    paintbox.y0 = pbox.min()[Geom::Y];
    paintbox.x1 = pbox.max()[Geom::X];
    paintbox.y1 = pbox.max()[Geom::Y];

    nr_arena_item_request_update(this, NR_ARENA_ITEM_STATE_ALL, FALSE);
}

static void
shape_run_A8_OR(raster_info &dest,void */*data*/,int st,float vst,int en,float ven)
{
    if ( st >= en ) return;
    if ( vst < 0 ) vst=0;
    if ( vst > 1 ) vst=1;
    if ( ven < 0 ) ven=0;
    if ( ven > 1 ) ven=1;
    float   sv=vst;
    float   dv=ven-vst;
    int     len=en-st;
    unsigned char*   d=(unsigned char*)dest.buffer;
    d+=(st-dest.startPix);
    if ( fabs(dv) < 0.001 ) {
        if ( vst > 0.999 ) {
            /* Simple copy */
            while (len > 0) {
                d[0] = 255;
                d += 1;
                len -= 1;
            }
        } else {
            sv*=256;
            unsigned int c0_24=(int)sv;
            c0_24&=0xFF;
            while (len > 0) {
                /* Draw */
                d[0] = NR_COMPOSEA_111(c0_24,d[0]);
                d += 1;
                len -= 1;
            }
        }
    } else {
        if ( en <= st+1 ) {
            sv=0.5*(vst+ven);
            sv*=256;
            unsigned int c0_24=(int)sv;
            c0_24&=0xFF;
            /* Draw */
            d[0] = NR_COMPOSEA_111(c0_24,d[0]);
        } else {
            dv/=len;
            sv+=0.5*dv; // correction trapezoidale
            sv*=16777216;
            dv*=16777216;
            int c0_24 = static_cast<int>(CLAMP(sv, 0, 16777216));
            int s0_24 = static_cast<int>(dv);
            while (len > 0) {
                unsigned int ca;
                /* Draw */
                ca = c0_24 >> 16;
                if ( ca > 255 ) ca=255;
                d[0] = NR_COMPOSEA_111(ca,d[0]);
                d += 1;
                c0_24 += s0_24;
                c0_24 = CLAMP(c0_24, 0, 16777216);
                len -= 1;
            }
        }
    }
}

void nr_pixblock_render_shape_mask_or(NRPixBlock &m,Shape* theS)
{
    theS->CalcBBox();
    float l = theS->leftX, r = theS->rightX, t = theS->topY, b = theS->bottomY;
    int    il,ir,it,ib;
    il=(int)floor(l);
    ir=(int)ceil(r);
    it=(int)floor(t);
    ib=(int)ceil(b);

    if ( il >= m.area.x1 || ir <= m.area.x0 || it >= m.area.y1 || ib <= m.area.y0 ) return;
    if ( il < m.area.x0 ) il=m.area.x0;
    if ( it < m.area.y0 ) it=m.area.y0;
    if ( ir > m.area.x1 ) ir=m.area.x1;
    if ( ib > m.area.y1 ) ib=m.area.y1;

    /* This is the FloatLigne version.  See svn (prior to Apr 2006) for versions using BitLigne or direct BitLigne. */
    int    curPt;
    float  curY;
    theS->BeginQuickRaster(curY, curPt);

    FloatLigne *theI = new FloatLigne();
    IntLigne *theIL = new IntLigne();

    theS->DirectQuickScan(curY, curPt, (float) it, true, 1.0);

    char *mdata = (char*)m.data.px;
    if ( m.size == NR_PIXBLOCK_SIZE_TINY ) mdata=(char*)m.data.p;
    uint32_t *ligStart = ((uint32_t*)(mdata + ((il - m.area.x0) + m.rs * (it - m.area.y0))));
    for (int y = it; y < ib; y++) {
        theI->Reset();
        theS->QuickScan(curY, curPt, ((float)(y+1)), theI, 1.0);
        theI->Flatten();
        theIL->Copy(theI);

        raster_info  dest;
        dest.startPix=il;
        dest.endPix=ir;
        dest.sth=il;
        dest.stv=y;
        dest.buffer=ligStart;
        theIL->Raster(dest, NULL, shape_run_A8_OR);
        ligStart=((uint32_t*)(((char*)ligStart)+m.rs));
    }
    theS->EndQuickRaster();
    delete theI;
    delete theIL;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
