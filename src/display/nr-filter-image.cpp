/*
 * feImage filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Tavmjong Bah <tavmjong@free.fr>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include "document.h"
#include "sp-item.h"
#include "display/cairo-utils.h"
#include "display/nr-arena.h"
#include "display/nr-arena-item.h"
#include "display/nr-filter.h"
#include "display/nr-filter-image.h"
#include "display/nr-filter-units.h"
#include "libnr/nr-rect-l.h"

namespace Inkscape {
namespace Filters {

FilterImage::FilterImage()
    : SVGElem(0)
    , document(0)
    , feImageHref(0)
    , image_surface(0)
    , broken_ref(false)
{ }

FilterPrimitive * FilterImage::create() {
    return new FilterImage();
}

FilterImage::~FilterImage()
{
    if (feImageHref)
        g_free(feImageHref);
}

void FilterImage::render_cairo(FilterSlot &slot)
{
    if (!feImageHref)
        return;

    //cairo_surface_t *input = slot.getcairo(_input);

    if (from_element) {
        if (!SVGElem) return;

        return; //not ready yet
/*
        // prep the document
        // TODO: do not recreate the rendering tree every time
        sp_document_ensure_up_to_date(document);
        NRArena* arena = NRArena::create();
        unsigned const key = sp_item_display_key_new(1);
        NRArenaItem* ai = sp_item_invoke_show(SVGElem, arena, key, SP_ITEM_SHOW_DISPLAY);
        if (!ai) {
            g_warning("feImage renderer: error creating NRArenaItem for SVG Element");
            nr_object_unref((NRObject *) arena);
            return;
        }

        Geom::OptRect optarea = SVGElem->getBounds(Geom::identity());
        if (!optarea) return;

        Geom::Rect area = *optarea;
        Geom::Matrix itrans = slot.get_units().get_matrix_display2pb();

        NRRectL const &slot_area = slot.get_slot_area();
        NRRectL rect;
        rect.x0 = floor(area->left());
        rect.x1 = ceil(area->right());
        rect.y0 = floor(area->top());
        rect.y1 = ceil(area->bottom());

        cairo_surface_t *out = ink_cairo_surface_create_same_size(in, CAIRO_CONTENT_COLOR_ALPHA);
        cairo_t *ct = cairo_create(out);
        cairo_translate(ct, -slot_area.x0, -slot_area.y0);
        ink_cairo_transform(ct, itrans);
        cairo_translate(ct, rect.x0, rect.y0);

        // Update to renderable state
        NRGC gc(NULL);
        Geom::Matrix t = Geom::identity();
        nr_arena_item_set_transform(ai, &t);
        gc.transform.setIdentity();
        nr_arena_item_invoke_update( ai, NULL, &gc,
                                             NR_ARENA_ITEM_STATE_ALL,
                                             NR_ARENA_ITEM_STATE_NONE );

        nr_arena_item_invoke_render(ct, ai, &rect, NULL, NR_ARENA_ITEM_RENDER_NO_CACHE);

        slot.set(_output, out);
        cairo_surface_destroy(out);
        return;*/
    }

    if (!image && !broken_ref) {
        broken_ref = true;
        try {
            /* TODO: If feImageHref is absolute, then use that (preferably handling the
             * case that it's not a file URI).  Otherwise, go up the tree looking
             * for an xml:base attribute, and use that as the base URI for resolving
             * the relative feImageHref URI.  Otherwise, if document && document->base,
             * then use that as the base URI.  Otherwise, use feImageHref directly
             * (i.e. interpreting it as relative to our current working directory).
             * (See http://www.w3.org/TR/xmlbase/#resolution .) */
            gchar *fullname = feImageHref;
            if ( !g_file_test( fullname, G_FILE_TEST_EXISTS ) ) {
                // Try to load from relative postion combined with document base
                if( document ) {
                    fullname = g_build_filename( document->base, feImageHref, NULL );
                }
            }
            if ( !g_file_test( fullname, G_FILE_TEST_EXISTS ) ) {
                // Should display Broken Image png.
                g_warning("FilterImage::render: Can not find: %s", feImageHref  );
            }
            image = Gdk::Pixbuf::create_from_file(fullname);
            if( fullname != feImageHref ) g_free( fullname );
        }
        catch (const Glib::FileError & e)
        {
            g_warning("caught Glib::FileError in FilterImage::render %i", e.code() );
            return;
        }
        catch (const Gdk::PixbufError & e)
        {
            g_warning("Gdk::PixbufError in FilterImage::render: %i", e.code() );
            return;
        }
        if ( !image ) return;

        broken_ref = false;

        bool has_alpha = image->get_has_alpha();
        if (!has_alpha) {
            image = image->add_alpha(false, 0, 0, 0);
        }

        // Native size of image
        //width = image->get_width();
        //height = image->get_height();
        //rowstride = image->get_rowstride();

        convert_pixbuf_normal_to_argb32(image->gobj());

        image_surface = cairo_image_surface_create_for_data(image->get_pixels(),
            CAIRO_FORMAT_ARGB32, image->get_width(), image->get_height(), image->get_rowstride());
    }

    NRRectL const &sa = slot.get_slot_area();
    cairo_surface_t *out = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
        sa.x1 - sa.x0, sa.y1 - sa.y0);

    cairo_t *ct = cairo_create(out);
    cairo_translate(ct, -sa.x0, -sa.y0);
    // now ct is in pb coordinates
    ink_cairo_transform(ct, slot.get_units().get_matrix_primitiveunits2pb());
    // now ct is in the coordinates of feImageX etc.

    // Get the object bounding box in user coordinates. Image is placed with respect to box.
    // Array values:  0: width; 3: height; 4: -x; 5: -y.
    Geom::Matrix object_bbox = slot.get_units().get_matrix_user2filterunits();

    // feImage is suppose to use the same parameters as a normal SVG image.
    // If a width or height is set to zero, the image is not suppose to be displayed.
    // This does not seem to be what Firefox or Opera does, nor does the W3C displacement
    // filter test expect this behavior. If the width and/or height are zero, we use
    // the width and height of the object bounding box.
    if( feImageWidth  == 0 ) feImageWidth  = object_bbox[0];
    if( feImageHeight == 0 ) feImageHeight = object_bbox[3];

    double scaleX = feImageWidth / image->get_width();
    double scaleY = feImageHeight / image->get_height();

    cairo_translate(ct, feImageX, feImageY);
    cairo_scale(ct, scaleX, scaleY);
    cairo_set_source_surface(ct, image_surface, 0, 0);
    cairo_paint(ct);
    cairo_destroy(ct);

    slot.set(_output, out);
}

bool FilterImage::can_handle_affine(Geom::Matrix const &)
{
    return true;
}

void FilterImage::set_href(const gchar *href){
    if (feImageHref) g_free (feImageHref);
    feImageHref = (href) ? g_strdup (href) : NULL;

    if (image_surface) {
        cairo_surface_destroy(image_surface);
    }
    image.reset();
}

void FilterImage::set_document(SPDocument *doc){
    document = doc;
}

void FilterImage::set_region(SVGLength x, SVGLength y, SVGLength width, SVGLength height){
        feImageX=x.computed;
        feImageY=y.computed;
        feImageWidth=width.computed;
        feImageHeight=height.computed;
}

} /* namespace Filters */
} /* namespace Inkscape */

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
