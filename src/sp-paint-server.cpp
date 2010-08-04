/*
 * Base class for gradients and patterns
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2010 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <string.h>
#include "sp-paint-server.h"

#include "sp-gradient.h"
#include "xml/node.h"

static void sp_paint_server_class_init(SPPaintServerClass *psc);
static void sp_paint_server_init(SPPaintServer *ps);

static void sp_paint_server_release(SPObject *object);

static cairo_pattern_t *sp_paint_server_create_dummy_pattern(SPPaintServer *ps, cairo_t *ct, NRRect const *bbox, double opacity);

static SPObjectClass *parent_class;

GType sp_paint_server_get_type (void)
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPPaintServerClass),
            NULL,       /* base_init */
            NULL,       /* base_finalize */
            (GClassInitFunc) sp_paint_server_class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data */
            sizeof(SPPaintServer),
            16, /* n_preallocs */
            (GInstanceInitFunc) sp_paint_server_init,
            NULL,       /* value_table */
        };
        type = g_type_register_static(SP_TYPE_OBJECT, "SPPaintServer", &info, (GTypeFlags) 0);
    }
    return type;
}

static void sp_paint_server_class_init(SPPaintServerClass *psc)
{
    SPObjectClass *sp_object_class = (SPObjectClass *) psc;
    sp_object_class->release = sp_paint_server_release;
    psc->pattern_new = sp_paint_server_create_dummy_pattern;
    parent_class = (SPObjectClass *) g_type_class_ref(SP_TYPE_OBJECT);
}

static void sp_paint_server_init(SPPaintServer *ps)
{
}

static void sp_paint_server_release(SPObject *object)
{
    if (((SPObjectClass *) parent_class)->release) {
        ((SPObjectClass *) parent_class)->release(object);
    }
}

cairo_pattern_t *sp_paint_server_create_pattern(SPPaintServer *ps,
                                                cairo_t *ct,
                                                NRRect const *bbox,
                                                double opacity)
{
    // NOTE: the ct argument is used for when rendering patterns
    // to create a group, instead of explicitly creating a temporary surface
    g_return_val_if_fail(ps != NULL, NULL);
    g_return_val_if_fail(SP_IS_PAINT_SERVER(ps), NULL);
    g_return_val_if_fail(bbox != NULL, NULL);

    cairo_pattern_t *cp = NULL;
    SPPaintServerClass *psc = (SPPaintServerClass *) G_OBJECT_GET_CLASS(ps);
    if ( psc->pattern_new ) {
        cp = (*psc->pattern_new)(ps, ct, bbox, opacity);
    }

    return cp;
}

static cairo_pattern_t *
sp_paint_server_create_dummy_pattern(SPPaintServer */*ps*/,
                                     cairo_t */* ct */,
                                     NRRect const */*bbox*/,
                                     double /* opacity */)
{
    cairo_pattern_t *cp = cairo_pattern_create_rgb(1.0, 0.0, 1.0);
    return cp;
}

bool SPPaintServer::isSwatch() const
{
    bool swatch = false;
    if (SP_IS_GRADIENT(this)) {
        SPGradient *grad = SP_GRADIENT(this);
        if ( SP_GRADIENT_HAS_STOPS(grad) ) {
            gchar const * attr = repr->attribute("osb:paint");
            if (attr && !strcmp(attr, "solid")) {
                swatch = true;
            }
        }
    }
    return swatch;
}

bool SPPaintServer::isSolid() const
{
    bool solid = false;
    if (SP_IS_GRADIENT(this)) {
        SPGradient *grad = SP_GRADIENT(this);
        if ( SP_GRADIENT_HAS_STOPS(grad) && (grad->getStopCount() == 0) ) {
            gchar const * attr = repr->attribute("osb:paint");
            if (attr && !strcmp(attr, "solid")) {
                solid = true;
            }
        }
    }
    return solid;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
