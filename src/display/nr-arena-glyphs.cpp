/*
 * RGBA display list system for inkscape
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 *
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <cairo.h>
#include <2geom/affine.h>
#include <2geom/rect.h>
#include "libnr/nr-convert2geom.h"
#include "style.h"
#include "display/nr-arena.h"
#include "display/nr-arena-glyphs.h"
#include "display/cairo-utils.h"
#include "display/drawing-context.h"
#include "helper/geom.h"

#ifdef test_glyph_liv
#include "../display/canvas-bpath.h"
#include "libnrtype/font-instance.h"

// defined in nr-arena-shape.cpp
void nr_pixblock_render_shape_mask_or(NRPixBlock &m, Shape *theS);
#endif

#ifdef ENABLE_SVG_FONTS
#include "nr-svgfonts.h"
#endif //#ifdef ENABLE_SVG_FONTS

static void nr_arena_glyphs_class_init(NRArenaGlyphsClass *klass);
static void nr_arena_glyphs_init(NRArenaGlyphs *glyphs);
static void nr_arena_glyphs_finalize(NRObject *object);

static guint nr_arena_glyphs_update(NRArenaItem *item, Geom::IntRect const &area, NRGC *gc, guint state, guint reset);
static NRArenaItem *nr_arena_glyphs_pick(NRArenaItem *item, Geom::Point const &p, double delta, unsigned int sticky);

static NRArenaItemClass *glyphs_parent_class;

NRType
nr_arena_glyphs_get_type(void)
{
    static NRType type = 0;
    if (!type) {
        type = nr_object_register_type(NR_TYPE_ARENA_ITEM,
                                       "NRArenaGlyphs",
                                       sizeof(NRArenaGlyphsClass),
                                       sizeof(NRArenaGlyphs),
                                       (void (*)(NRObjectClass *)) nr_arena_glyphs_class_init,
                                       (void (*)(NRObject *)) nr_arena_glyphs_init);
    }
    return type;
}

static void
nr_arena_glyphs_class_init(NRArenaGlyphsClass *klass)
{
    NRObjectClass *object_class;
    NRArenaItemClass *item_class;

    object_class = (NRObjectClass *) klass;
    item_class = (NRArenaItemClass *) klass;

    glyphs_parent_class = (NRArenaItemClass *) ((NRObjectClass *) klass)->parent;

    object_class->finalize = nr_arena_glyphs_finalize;
    object_class->cpp_ctor = NRObject::invoke_ctor<NRArenaGlyphs>;

    item_class->update = nr_arena_glyphs_update;
    item_class->pick = nr_arena_glyphs_pick;
}

static void
nr_arena_glyphs_init(NRArenaGlyphs *glyphs)
{
    glyphs->g_transform.setIdentity();
    glyphs->font = NULL;
    glyphs->glyph = 0;
    glyphs->x = glyphs->y = 0.0;
}

static void
nr_arena_glyphs_finalize(NRObject *object)
{
    NRArenaGlyphs *glyphs = static_cast<NRArenaGlyphs *>(object);

    if (glyphs->font) {
        glyphs->font->Unref();
        glyphs->font=NULL;
    }

    ((NRObjectClass *) glyphs_parent_class)->finalize(object);
}

static guint
nr_arena_glyphs_update(NRArenaItem *item, Geom::IntRect const &/*area*/, NRGC *gc, guint /*state*/, guint /*reset*/)
{
    NRArenaGlyphs *glyphs = NR_ARENA_GLYPHS(item);
    NRArenaGlyphsGroup *ggroup = NR_ARENA_GLYPHS_GROUP(item->parent);

    if (!glyphs->font || !ggroup->style)
        return NR_ARENA_ITEM_STATE_ALL;
    if (ggroup->nrstyle.fill.type == NRStyle::PAINT_NONE && ggroup->nrstyle.stroke.type == NRStyle::PAINT_NONE)
        return NR_ARENA_ITEM_STATE_ALL;

    Geom::OptRect b;
    Geom::Affine t = glyphs->g_transform * gc->transform;
    glyphs->x = t[4];
    glyphs->y = t[5];

    b = bounds_exact_transformed(*glyphs->font->PathVector(glyphs->glyph), t);
    if (b && ggroup->nrstyle.stroke.type != NRStyle::PAINT_NONE) {
        float width, scale;
        scale = gc->transform.descrim();
        width = MAX(0.125, ggroup->nrstyle.stroke_width * scale);
        if ( fabs(ggroup->nrstyle.stroke_width * scale) > 0.01 ) { // FIXME: this is always true
            b->expandBy(width);
        }
        // those pesky miters, now
        float miterMax = width * ggroup->nrstyle.miter_limit;
        if ( miterMax > 0.01 ) {
            // grunt mode. we should compute the various miters instead
            // (one for each point on the curve)
            b->expandBy(miterMax);
        }
    }    

    if (b) {
        item->bbox = b->roundOutwards();
    } else {
        item->bbox = Geom::OptIntRect();
    }

    return NR_ARENA_ITEM_STATE_ALL;
}

static NRArenaItem *
nr_arena_glyphs_pick(NRArenaItem *item, Geom::Point const &p, gdouble delta, unsigned int /*sticky*/)
{
    NRArenaGlyphs *glyphs;

    glyphs = NR_ARENA_GLYPHS(item);

    if (!glyphs->font ) return NULL;
    if (!item->bbox) return NULL;

    // With text we take a simple approach: pick if the point is in a characher bbox
    Geom::Rect expanded(*item->bbox);
    expanded.expandBy(delta);
    if (expanded.contains(p))
        return item;
    return NULL;
}

void
nr_arena_glyphs_set_path(NRArenaGlyphs *glyphs, SPCurve */*curve*/, unsigned int /*lieutenant*/, font_instance *font, gint glyph, Geom::Affine const *transform)
{
    nr_return_if_fail(glyphs != NULL);
    nr_return_if_fail(NR_IS_ARENA_GLYPHS(glyphs));

    nr_arena_item_request_render(NR_ARENA_ITEM(glyphs));

    if (transform) {
        glyphs->g_transform = *transform;
    } else {
        glyphs->g_transform.setIdentity();
    }

    if (font) font->Ref();
    if (glyphs->font) glyphs->font->Unref();
    glyphs->font=font;
    glyphs->glyph = glyph;

    nr_arena_item_request_update(NR_ARENA_ITEM(glyphs), NR_ARENA_ITEM_STATE_ALL, FALSE);
}

static void nr_arena_glyphs_group_class_init(NRArenaGlyphsGroupClass *klass);
static void nr_arena_glyphs_group_init(NRArenaGlyphsGroup *group);
static void nr_arena_glyphs_group_finalize(NRObject *object);

static guint nr_arena_glyphs_group_update(NRArenaItem *item, Geom::IntRect const &area, NRGC *gc, guint state, guint reset);
static unsigned int nr_arena_glyphs_group_render(Inkscape::DrawingContext &ct, NRArenaItem *item, Geom::IntRect const &area, unsigned int flags);
static unsigned int nr_arena_glyphs_group_clip(Inkscape::DrawingContext &ct, NRArenaItem *item, Geom::IntRect const &area);
static NRArenaItem *nr_arena_glyphs_group_pick(NRArenaItem *item, Geom::Point const &p, gdouble delta, unsigned int sticky);

static NRArenaGroupClass *group_parent_class;

NRType
nr_arena_glyphs_group_get_type(void)
{
    static NRType type = 0;
    if (!type) {
        type = nr_object_register_type(NR_TYPE_ARENA_GROUP,
                                       "NRArenaGlyphsGroup",
                                       sizeof(NRArenaGlyphsGroupClass),
                                       sizeof(NRArenaGlyphsGroup),
                                       (void (*)(NRObjectClass *)) nr_arena_glyphs_group_class_init,
                                       (void (*)(NRObject *)) nr_arena_glyphs_group_init);
    }
    return type;
}

static void
nr_arena_glyphs_group_class_init(NRArenaGlyphsGroupClass *klass)
{
    NRObjectClass *object_class;
    NRArenaItemClass *item_class;

    object_class = (NRObjectClass *) klass;
    item_class = (NRArenaItemClass *) klass;

    group_parent_class = (NRArenaGroupClass *) ((NRObjectClass *) klass)->parent;

    object_class->finalize = nr_arena_glyphs_group_finalize;
    object_class->cpp_ctor = NRObject::invoke_ctor<NRArenaGlyphsGroup>;

    item_class->update = nr_arena_glyphs_group_update;
    item_class->render = nr_arena_glyphs_group_render;
    item_class->clip = nr_arena_glyphs_group_clip;
    item_class->pick = nr_arena_glyphs_group_pick;
}

static void
nr_arena_glyphs_group_init(NRArenaGlyphsGroup *group)
{
    group->style = NULL;
}

static void
nr_arena_glyphs_group_finalize(NRObject *object)
{
    NRArenaGlyphsGroup *group = static_cast<NRArenaGlyphsGroup *>(object);

    if (group->style) {
        sp_style_unref(group->style);
        group->style = NULL;
    }

    ((NRObjectClass *) group_parent_class)->finalize(object);
}

static guint
nr_arena_glyphs_group_update(NRArenaItem *item, Geom::IntRect const &area, NRGC *gc, guint state, guint reset)
{
    NRArenaGlyphsGroup *group = NR_ARENA_GLYPHS_GROUP(item);

    group->nrstyle.update();

    if (((NRArenaItemClass *) group_parent_class)->update)
        return ((NRArenaItemClass *) group_parent_class)->update(item, area, gc, state, reset);

    return NR_ARENA_ITEM_STATE_ALL;
}


static unsigned int
nr_arena_glyphs_group_render(Inkscape::DrawingContext &ct, NRArenaItem *item, Geom::IntRect const &area, unsigned int /*flags*/)
{
    NRArenaItem *child = 0;

    NRArenaGroup *group = NR_ARENA_GROUP(item);
    NRArenaGlyphsGroup *ggroup = NR_ARENA_GLYPHS_GROUP(item);

    if (item->arena->rendermode == Inkscape::RENDERMODE_OUTLINE) {
        Inkscape::DrawingContext::Save save(ct);
        guint32 rgba = item->arena->outlinecolor;
        ct.setSource(rgba);
        ct.setTolerance(1.25); // low quality, but good enough for outline mode
        ct.newPath();
        ct.transform(ggroup->ctm);

        for (child = group->children; child != NULL; child = child->next) {
            NRArenaGlyphs *g = NR_ARENA_GLYPHS(child);

            Geom::PathVector const * pathv = g->font->PathVector(g->glyph);
            Geom::Affine transform = g->g_transform;

            Inkscape::DrawingContext::Save save(ct);
            ct.transform(transform);
            ct.path(*pathv);
            ct.fill();
        }
        return item->state;
    }

    // NOTE: this is very similar to nr-arena-shape.cpp; the only difference is path feeding
    bool has_stroke, has_fill;

    Inkscape::DrawingContext::Save save(ct);
    ct.transform(ggroup->ctm);

    has_fill   = ggroup->nrstyle.prepareFill(ct, ggroup->paintbox);
    has_stroke = ggroup->nrstyle.prepareStroke(ct, ggroup->paintbox);

    if (has_fill || has_stroke) {
        for (NRArenaItem *child = ggroup->children; child != NULL; child = child->next) {
            NRArenaGlyphs *g = NR_ARENA_GLYPHS(child);
            Geom::PathVector const &pathv = *g->font->PathVector(g->glyph);

            Inkscape::DrawingContext::Save save(ct);
            ct.transform(g->g_transform);
            ct.path(pathv);
        }

        if (has_fill) {
            ggroup->nrstyle.applyFill(ct);
            ct.fillPreserve();
        }
        if (has_stroke) {
            ggroup->nrstyle.applyStroke(ct);
            ct.strokePreserve();
        }
        ct.newPath(); // clear path
    } // has fill or stroke pattern

    return item->state;
}

static unsigned int nr_arena_glyphs_group_clip(Inkscape::DrawingContext &ct, NRArenaItem *item, Geom::IntRect const &/*area*/)
{
    NRArenaGroup *ggroup = NR_ARENA_GLYPHS_GROUP(item);

    Inkscape::DrawingContext::Save save(ct);

    // handle clip-rule
    if (ggroup->style) {
        if (ggroup->style->clip_rule.computed == SP_WIND_RULE_EVENODD) {
            ct.setFillRule(CAIRO_FILL_RULE_EVEN_ODD);
        } else {
            ct.setFillRule(CAIRO_FILL_RULE_WINDING);
        }
    }
    ct.transform(ggroup->ctm);

    for (NRArenaItem *child = ggroup->children; child != NULL; child = child->next) {
        NRArenaGlyphs *g = NR_ARENA_GLYPHS(child);
        Geom::PathVector const &pathv = *g->font->PathVector(g->glyph);

        Inkscape::DrawingContext::Save save(ct);
        ct.transform(g->g_transform);
        ct.path(pathv);
    }
    ct.fill();

    return item->state;
}

static NRArenaItem *
nr_arena_glyphs_group_pick(NRArenaItem *item, Geom::Point const &p, gdouble delta, unsigned int sticky)
{
    NRArenaItem *picked = NULL;

    if (((NRArenaItemClass *) group_parent_class)->pick)
        picked = ((NRArenaItemClass *) group_parent_class)->pick(item, p, delta, sticky);

    if (picked) picked = item;

    return picked;
}

void
nr_arena_glyphs_group_clear(NRArenaGlyphsGroup *sg)
{
    NRArenaGroup *group = NR_ARENA_GROUP(sg);

    nr_arena_item_request_render(NR_ARENA_ITEM(group));

    while (group->children) {
        nr_arena_item_remove_child(NR_ARENA_ITEM(group), group->children);
    }
}

void
nr_arena_glyphs_group_add_component(NRArenaGlyphsGroup *sg, font_instance *font, int glyph, Geom::Affine const &transform)
{
    NRArenaGroup *group;

    group = NR_ARENA_GROUP(sg);

    Geom::PathVector const * pathv = ( font
                                       ? font->PathVector(glyph)
                                       : NULL );
    if ( pathv ) {
        nr_arena_item_request_render(NR_ARENA_ITEM(group));

        NRArenaItem *new_arena = NRArenaGlyphs::create(group->arena);
        nr_arena_item_append_child(NR_ARENA_ITEM(group), new_arena);
        nr_arena_item_unref(new_arena);
        nr_arena_glyphs_set_path(NR_ARENA_GLYPHS(new_arena), NULL, FALSE, font, glyph, &transform);
    }
}

void
nr_arena_glyphs_group_set_style(NRArenaGlyphsGroup *sg, SPStyle *style)
{
    nr_return_if_fail(sg != NULL);
    nr_return_if_fail(NR_IS_ARENA_GLYPHS_GROUP(sg));

    if (style) sp_style_ref(style);
    if (sg->style) sp_style_unref(sg->style);
    sg->style = style;

    sg->nrstyle.set(style);

    nr_arena_item_request_update(NR_ARENA_ITEM(sg), NR_ARENA_ITEM_STATE_ALL, FALSE);
}

void
nr_arena_glyphs_group_set_paintbox(NRArenaGlyphsGroup *gg, NRRect const *pbox)
{
    nr_return_if_fail(gg != NULL);
    nr_return_if_fail(NR_IS_ARENA_GLYPHS_GROUP(gg));
    nr_return_if_fail(pbox != NULL);

    gg->paintbox = pbox->upgrade_2geom();

    nr_arena_item_request_update(NR_ARENA_ITEM(gg), NR_ARENA_ITEM_STATE_ALL, FALSE);
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
