/*
 * SVG <image> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Edward Flick (EAF)
 *   Abhishek Sharma
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 1999-2005 Authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

// This has to be included prior to anything that includes setjmp.h, it croaks otherwise
#include <png.h>

#include <cstring>
#include <algorithm>
#include <string>
#include <glib/gstdio.h>
#include <2geom/rect.h>
#include <2geom/transforms.h>
#include <glibmm/i18n.h>

#include "display/drawing-image.h"
#include "display/cairo-utils.h"
#include "display/curve.h"
//Added for preserveAspectRatio support -- EAF
#include "enums.h"
#include "attributes.h"
#include "print.h"
#include "brokenimage.xpm"
#include "document.h"
#include "sp-image.h"
#include "sp-clippath.h"
#include "xml/quote.h"
#include "xml/repr.h"
#include "snap-candidate.h"

#include "io/sys.h"
#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
#include "cms-system.h"
#include "color-profile.h"

#if HAVE_LIBLCMS2
#  include <lcms2.h>
#elif HAVE_LIBLCMS1
#  include <lcms.h>
#endif // HAVE_LIBLCMS2

//#define DEBUG_LCMS
#ifdef DEBUG_LCMS


#define DEBUG_MESSAGE(key, ...)\
{\
    g_message( __VA_ARGS__ );\
}

#include "preferences.h"
#include <gtk/gtk.h>
#endif // DEBUG_LCMS
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
/*
 * SPImage
 */

// TODO: give these constants better names:
#define MAGIC_EPSILON 1e-9
#define MAGIC_EPSILON_TOO 1e-18
// TODO: also check if it is correct to be using two different epsilon values

static void sp_image_class_init (SPImageClass * klass);
static void sp_image_init (SPImage * image);

static void sp_image_build (SPObject * object, SPDocument * document, Inkscape::XML::Node * repr);
static void sp_image_release (SPObject * object);
static void sp_image_set (SPObject *object, unsigned int key, const gchar *value);
static void sp_image_update (SPObject *object, SPCtx *ctx, unsigned int flags);
static void sp_image_modified (SPObject *object, unsigned int flags);
static Inkscape::XML::Node *sp_image_write (SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);

static Geom::OptRect sp_image_bbox(SPItem const *item, Geom::Affine const &transform, SPItem::BBoxType type);
static void sp_image_print (SPItem * item, SPPrintContext *ctx);
static gchar * sp_image_description (SPItem * item);
static void sp_image_snappoints(SPItem const *item, std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs);
static Inkscape::DrawingItem *sp_image_show (SPItem *item, Inkscape::Drawing &drawing, unsigned int key, unsigned int flags);
static Geom::Affine sp_image_set_transform (SPItem *item, Geom::Affine const &xform);
static void sp_image_set_curve(SPImage *image);


static GdkPixbuf *sp_image_repr_read_image( time_t& modTime, gchar*& pixPath, const gchar *href, const gchar *absref, const gchar *base );
static GdkPixbuf *sp_image_pixbuf_force_rgba (GdkPixbuf * pixbuf);
static void sp_image_update_arenaitem (SPImage *img, Inkscape::DrawingImage *ai);
static void sp_image_update_canvas_image (SPImage *image);
static GdkPixbuf * sp_image_repr_read_dataURI (const gchar * uri_data);
static GdkPixbuf * sp_image_repr_read_b64 (const gchar * uri_data);

static SPItemClass *parent_class;


extern "C"
{
    void user_read_data( png_structp png_ptr, png_bytep data, png_size_t length );
    void user_write_data( png_structp png_ptr, png_bytep data, png_size_t length );
    void user_flush_data( png_structp png_ptr );
}


#ifdef DEBUG_LCMS
extern guint update_in_progress;
#define DEBUG_MESSAGE_SCISLAC(key, ...) \
{\
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();\
    bool dump = prefs->getBool("/options/scislac/" #key);\
    bool dumpD = prefs->getBool("/options/scislac/" #key "D");\
    bool dumpD2 = prefs->getBool("/options/scislac/" #key "D2");\
    dumpD &&= ( (update_in_progress == 0) || dumpD2 );\
    if ( dump )\
    {\
        g_message( __VA_ARGS__ );\
\
    }\
    if ( dumpD )\
    {\
        GtkWidget *dialog = gtk_message_dialog_new(NULL,\
                                                   GTK_DIALOG_DESTROY_WITH_PARENT, \
                                                   GTK_MESSAGE_INFO,    \
                                                   GTK_BUTTONS_OK,      \
                                                   __VA_ARGS__          \
                                                   );\
        g_signal_connect_swapped(dialog, "response",\
                                 G_CALLBACK(gtk_widget_destroy),        \
                                 dialog);                               \
        gtk_widget_show_all( dialog );\
    }\
}
#endif // DEBUG_LCMS

namespace Inkscape {
namespace IO {

class PushPull
{
public:
    gboolean    first;
    FILE*       fp;
    guchar*     scratch;
    gsize       size;
    gsize       used;
    gsize       offset;
    GdkPixbufLoader *loader;

    PushPull() : first(TRUE),
                 fp(0),
                 scratch(0),
                 size(0),
                 used(0),
                 offset(0),
                 loader(0) {};

    gboolean readMore()
    {
        gboolean good = FALSE;
        if ( offset )
        {
            g_memmove( scratch, scratch + offset, used - offset );
            used -= offset;
            offset = 0;
        }
        if ( used < size )
        {
            gsize space = size - used;
            gsize got = fread( scratch + used, 1, space, fp );
            if ( got )
            {
                if ( loader )
                {
                    GError *err = NULL;
                    //g_message( " __read %d bytes", (int)got );
                    if ( !gdk_pixbuf_loader_write( loader, scratch + used, got, &err ) )
                    {
                        //g_message("_error writing pixbuf data");
                    }
                }

                used += got;
                good = TRUE;
            }
            else
            {
                good = FALSE;
            }
        }
        return good;
    }

    gsize available() const
    {
        return (used - offset);
    }

    gsize readOut( gpointer data, gsize length )
    {
        gsize giving = available();
        if ( length < giving )
        {
            giving = length;
        }
        g_memmove( data, scratch + offset, giving );
        offset += giving;
        if ( offset >= used )
        {
            offset = 0;
            used = 0;
        }
        return giving;
    }

    void clear()
    {
        offset = 0;
        used = 0;
    }

private:
    PushPull& operator = (const PushPull& other);
    PushPull(const PushPull& other);
};

static void user_read_data( png_structp png_ptr, png_bytep data, png_size_t length )
{
//    g_message( "user_read_data(%d)", length );

    PushPull* youme = (PushPull*)png_get_io_ptr(png_ptr);

    gsize filled = 0;
    gboolean canRead = TRUE;

    while ( filled < length && canRead )
    {
        gsize some = youme->readOut( data + filled, length - filled );
        filled += some;
        if ( filled < length )
        {
            canRead &= youme->readMore();
        }
    }
//    g_message("things out");
}

static void user_write_data( png_structp /*png_ptr*/, png_bytep /*data*/, png_size_t /*length*/ )
{
    //g_message( "user_write_data(%d)", length );
}

static void user_flush_data( png_structp /*png_ptr*/ )
{
    //g_message( "user_flush_data" );
}


static bool readPngAndHeaders( PushPull &youme, gint & dpiX, gint & dpiY )
{
    bool good = true;

    gboolean isPng = !png_sig_cmp( youme.scratch + youme.offset, 0, youme.available() );
    //g_message( "  png? %s", (isPng ? "Yes":"No") );
    if ( isPng ) {
        png_structp pngPtr = png_create_read_struct( PNG_LIBPNG_VER_STRING,
                                                     0, //(png_voidp)user_error_ptr,
                                                     0, //user_error_fn,
                                                     0 //user_warning_fn
            );
        png_infop infoPtr = pngPtr ? png_create_info_struct( pngPtr ) : 0;

        if ( pngPtr && infoPtr ) {
            if ( setjmp(png_jmpbuf(pngPtr)) ) {
                // libpng calls longjmp to return here if an error occurs.
                good = false;
            }

            if (good) {
                png_set_read_fn( pngPtr, &youme, user_read_data );
                //g_message( "In" );

                //png_read_info( pngPtr, infoPtr );
                png_read_png( pngPtr, infoPtr, PNG_TRANSFORM_IDENTITY, 0 );

                //g_message("out");

                /*
                  if ( png_get_valid( pngPtr, infoPtr, PNG_INFO_pHYs ) )
                  {
                  g_message("pHYs chunk now valid" );
                  }
                  if ( png_get_valid( pngPtr, infoPtr, PNG_INFO_sCAL ) )
                  {
                  g_message("sCAL chunk now valid" );
                  }
                */

                png_uint_32 res_x = 0;
                png_uint_32 res_y = 0;
                int unit_type = 0;
                if ( png_get_pHYs( pngPtr, infoPtr, &res_x, &res_y, &unit_type) ) {
//                                     g_message( "pHYs yes (%d, %d) %d (%s)", (int)res_x, (int)res_y, unit_type,
//                                                (unit_type == 1? "per meter" : "unknown")
//                                         );

//                                     g_message( "    dpi: (%d, %d)",
//                                                (int)(0.5 + ((double)res_x)/39.37),
//                                                (int)(0.5 + ((double)res_y)/39.37) );
                    if ( unit_type == PNG_RESOLUTION_METER )
                    {
                        // TODO come up with a more accurate DPI setting
                        dpiX = (int)(0.5 + ((double)res_x)/39.37);
                        dpiY = (int)(0.5 + ((double)res_y)/39.37);
                    }
                } else {
//                                     g_message( "pHYs no" );
                }

/*
  double width = 0;
  double height = 0;
  int unit = 0;
  if ( png_get_sCAL(pngPtr, infoPtr, &unit, &width, &height) )
  {
  gchar* vals[] = {
  "unknown", // PNG_SCALE_UNKNOWN
  "meter", // PNG_SCALE_METER
  "radian", // PNG_SCALE_RADIAN
  "last", //
  NULL
  };

  g_message( "sCAL: (%f, %f) %d (%s)",
  width, height, unit,
  ((unit >= 0 && unit < 3) ? vals[unit]:"???")
  );
  }
*/

#if defined(PNG_sRGB_SUPPORTED)
                {
                    int intent = 0;
                    if ( png_get_sRGB(pngPtr, infoPtr, &intent) ) {
//                                         g_message("Found an sRGB png chunk");
                    }
                }
#endif // defined(PNG_sRGB_SUPPORTED)

#if defined(PNG_cHRM_SUPPORTED)
                {
                    double white_x = 0;
                    double white_y = 0;
                    double red_x = 0;
                    double red_y = 0;
                    double green_x = 0;
                    double green_y = 0;
                    double blue_x = 0;
                    double blue_y = 0;

                    if ( png_get_cHRM(pngPtr, infoPtr,
                                      &white_x, &white_y,
                                      &red_x, &red_y,
                                      &green_x, &green_y,
                                      &blue_x, &blue_y) ) {
//                                         g_message("Found a cHRM png chunk");
                    }
                }
#endif // defined(PNG_cHRM_SUPPORTED)

#if defined(PNG_gAMA_SUPPORTED)
                {
                    double file_gamma = 0;
                    if ( png_get_gAMA(pngPtr, infoPtr, &file_gamma) ) {
//                                         g_message("Found a gAMA png chunk");
                    }
                }
#endif // defined(PNG_gAMA_SUPPORTED)

#if defined(PNG_iCCP_SUPPORTED)
                {
                    png_charp name = 0;
                    int compression_type = 0;
#if (PNG_LIBPNG_VER < 10500)
                    png_charp profile = 0;
#else
                    png_bytep profile = 0;
#endif
                    png_uint_32 proflen = 0;
                    if ( png_get_iCCP(pngPtr, infoPtr, &name, &compression_type, &profile, &proflen) ) {
//                                         g_message("Found an iCCP chunk named [%s] with %d bytes and comp %d", name, proflen, compression_type);
                    }
                }
#endif // defined(PNG_iCCP_SUPPORTED)

            }
        } else {
            g_message("Error when creating PNG read struct");
        }

        // now clean it up.
        if (pngPtr && infoPtr) {
            png_destroy_read_struct( &pngPtr, &infoPtr, 0 );
            pngPtr = 0;
            infoPtr = 0;
        } else if (pngPtr) {
            png_destroy_read_struct( &pngPtr, 0, 0 );
            pngPtr = 0;
        }
    } else {
        good = false; // Was not a png file
    }

    return good;
}

GdkPixbuf* pixbuf_new_from_file( const char *filename, time_t &modTime, gchar*& pixPath, GError **/*error*/ )
{
    GdkPixbuf* buf = NULL;
    PushPull youme;
    gint dpiX = 0;
    gint dpiY = 0;
    modTime = 0;
    if ( pixPath ) {
        g_free(pixPath);
        pixPath = 0;
    }

    //buf = gdk_pixbuf_new_from_file( filename, error );
    dump_fopen_call( filename, "pixbuf_new_from_file" );
    FILE* fp = fopen_utf8name( filename, "r" );
    if ( fp )
    {
        {
            struct stat st;
            memset(&st, 0, sizeof(st));
            int val = g_stat(filename, &st);
            if ( !val ) {
                modTime = st.st_mtime;
                pixPath = g_strdup(filename);
            }
        }

        GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
        if ( loader )
        {
            GError *err = NULL;

            // short buffer
            guchar scratch[1024];
            gboolean latter = FALSE;

            youme.fp = fp;
            youme.scratch = scratch;
            youme.size = sizeof(scratch);
            youme.used = 0;
            youme.offset = 0;
            youme.loader = loader;

            while ( !feof(fp) )
            {
                if ( youme.readMore() ) {
                    if ( youme.first ) {
                        //g_message( "First data chunk" );
                        youme.first = FALSE;
                        if (readPngAndHeaders(youme, dpiX, dpiY))
                        {
                            // TODO set the dpi to be read elsewhere
                        }
                    } else if ( !latter ) {
                        latter = TRUE;
                        //g_message("  READing latter");
                    }
                    // Now clear out the buffer so we can read more.
                    // (dumping out unused)
                    youme.clear();
                }
            }

            gboolean ok = gdk_pixbuf_loader_close(loader, &err);
            if ( ok ) {
                buf = gdk_pixbuf_loader_get_pixbuf( loader );
                if ( buf ) {
                    g_object_ref(buf);

                    if ( dpiX ) {
                        gchar *tmp = g_strdup_printf( "%d", dpiX );
                        if ( tmp ) {
                            //g_message("Need to set DpiX: %s", tmp);
                            //gdk_pixbuf_set_option( buf, "Inkscape::DpiX", tmp );
                            g_free( tmp );
                        }
                    }
                    if ( dpiY ) {
                        gchar *tmp = g_strdup_printf( "%d", dpiY );
                        if ( tmp ) {
                            //g_message("Need to set DpiY: %s", tmp);
                            //gdk_pixbuf_set_option( buf, "Inkscape::DpiY", tmp );
                            g_free( tmp );
                        }
                    }
                }
            } else {
                // do something
                g_message("error loading pixbuf at close");
            }

            g_object_unref(loader);
        } else {
            g_message("error when creating pixbuf loader");
        }
        fclose( fp );
        fp = 0;
    } else {
        g_warning ("Unable to open linked file: %s", filename);
    }

/*
    if ( buf )
    {
        const gchar* bloop = gdk_pixbuf_get_option( buf, "Inkscape::DpiX" );
        if ( bloop )
        {
            g_message("DPI X is [%s]", bloop);
        }
        bloop = gdk_pixbuf_get_option( buf, "Inkscape::DpiY" );
        if ( bloop )
        {
            g_message("DPI Y is [%s]", bloop);
        }
    }
*/

    return buf;
}

GdkPixbuf* pixbuf_new_from_file( const char *filename, GError **error )
{
    time_t modTime = 0;
    gchar* pixPath = 0;
    GdkPixbuf* result = pixbuf_new_from_file( filename, modTime, pixPath, error );
    if (pixPath) {
        g_free(pixPath);
    }
    return result;
}


}
}

GType sp_image_get_type(void)
{
    static GType image_type = 0;
    if (!image_type) {
        GTypeInfo image_info = {
            sizeof (SPImageClass),
            NULL,       /* base_init */
            NULL,       /* base_finalize */
            (GClassInitFunc) sp_image_class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data */
            sizeof (SPImage),
            16, /* n_preallocs */
            (GInstanceInitFunc) sp_image_init,
            NULL,       /* value_table */
        };
        image_type = g_type_register_static (SPItem::getType (), "SPImage", &image_info, (GTypeFlags)0);
    }
    return image_type;
}

static void sp_image_class_init( SPImageClass * klass )
{
    SPObjectClass *sp_object_class = reinterpret_cast<SPObjectClass *>(klass);
    SPItemClass *item_class = reinterpret_cast<SPItemClass *>(klass);

    parent_class = reinterpret_cast<SPItemClass*>(g_type_class_ref(SPItem::getType()));

    sp_object_class->build = sp_image_build;
    sp_object_class->release = sp_image_release;
    sp_object_class->set = sp_image_set;
    sp_object_class->update = sp_image_update;
    sp_object_class->modified = sp_image_modified;
    sp_object_class->write = sp_image_write;

    item_class->bbox = sp_image_bbox;
    item_class->print = sp_image_print;
    item_class->description = sp_image_description;
    item_class->show = sp_image_show;
    item_class->snappoints = sp_image_snappoints;
    item_class->set_transform = sp_image_set_transform;
}

static void sp_image_init( SPImage *image )
{
    image->x.unset();
    image->y.unset();
    image->width.unset();
    image->height.unset();
    image->aspect_align = SP_ASPECT_NONE;
    image->clipbox = Geom::Rect();
    image->sx = image->sy = 1.0;
    image->ox = image->oy = 0.0;

    image->curve = NULL;

    image->href = 0;
#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    image->color_profile = 0;
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    image->pixbuf = 0;
    image->pixPath = 0;
    image->lastMod = 0;
}

static void sp_image_build( SPObject *object, SPDocument *document, Inkscape::XML::Node *repr )
{
    if (((SPObjectClass *) parent_class)->build) {
        ((SPObjectClass *) parent_class)->build (object, document, repr);
    }

    object->readAttr( "xlink:href" );
    object->readAttr( "x" );
    object->readAttr( "y" );
    object->readAttr( "width" );
    object->readAttr( "height" );
    object->readAttr( "preserveAspectRatio" );
    object->readAttr( "color-profile" );

    /* Register */
    document->addResource("image", object);
}

static void sp_image_release( SPObject *object )
{
    SPImage *image = SP_IMAGE(object);

    if (object->document) {
        // Unregister ourselves
        object->document->removeResource("image", object);
    }

    if (image->href) {
        g_free (image->href);
        image->href = NULL;
    }

    if (image->pixbuf) {
        g_object_unref (image->pixbuf);
        image->pixbuf = NULL;
    }

#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    if (image->color_profile) {
        g_free (image->color_profile);
        image->color_profile = NULL;
    }
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)

    if (image->pixPath) {
        g_free(image->pixPath);
        image->pixPath = 0;
    }

    if (image->curve) {
        image->curve = image->curve->unref();
    }

    if (((SPObjectClass *) parent_class)->release) {
        ((SPObjectClass *) parent_class)->release (object);
    }
}

static void sp_image_set( SPObject *object, unsigned int key, const gchar *value )
{
    SPImage *image = SP_IMAGE (object);

    switch (key) {
        case SP_ATTR_XLINK_HREF:
            g_free (image->href);
            image->href = (value) ? g_strdup (value) : NULL;
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_IMAGE_HREF_MODIFIED_FLAG);
            break;
        case SP_ATTR_X:
            if (!image->x.readAbsolute(value)) {
                /* fixme: em, ex, % are probably valid, but require special treatment (Lauris) */
                image->x.unset();
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_Y:
            if (!image->y.readAbsolute(value)) {
                /* fixme: em, ex, % are probably valid, but require special treatment (Lauris) */
                image->y.unset();
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_WIDTH:
            if (!image->width.readAbsolute(value)) {
                /* fixme: em, ex, % are probably valid, but require special treatment (Lauris) */
                image->width.unset();
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_HEIGHT:
            if (!image->height.readAbsolute(value)) {
                /* fixme: em, ex, % are probably valid, but require special treatment (Lauris) */
                image->height.unset();
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_PRESERVEASPECTRATIO:
            /* Do setup before, so we can use break to escape */
            image->aspect_align = SP_ASPECT_NONE;
            image->aspect_clip = SP_ASPECT_MEET;
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_VIEWPORT_MODIFIED_FLAG);
            if (value) {
                int len;
                gchar c[256];
                const gchar *p, *e;
                unsigned int align, clip;
                p = value;
                while (*p && *p == 32) p += 1;
                if (!*p) break;
                e = p;
                while (*e && *e != 32) e += 1;
                len = e - p;
                if (len > 8) break;
                memcpy (c, value, len);
                c[len] = 0;
                /* Now the actual part */
                if (!strcmp (c, "none")) {
                    align = SP_ASPECT_NONE;
                } else if (!strcmp (c, "xMinYMin")) {
                    align = SP_ASPECT_XMIN_YMIN;
                } else if (!strcmp (c, "xMidYMin")) {
                    align = SP_ASPECT_XMID_YMIN;
                } else if (!strcmp (c, "xMaxYMin")) {
                    align = SP_ASPECT_XMAX_YMIN;
                } else if (!strcmp (c, "xMinYMid")) {
                    align = SP_ASPECT_XMIN_YMID;
                } else if (!strcmp (c, "xMidYMid")) {
                    align = SP_ASPECT_XMID_YMID;
                } else if (!strcmp (c, "xMaxYMid")) {
                    align = SP_ASPECT_XMAX_YMID;
                } else if (!strcmp (c, "xMinYMax")) {
                    align = SP_ASPECT_XMIN_YMAX;
                } else if (!strcmp (c, "xMidYMax")) {
                    align = SP_ASPECT_XMID_YMAX;
                } else if (!strcmp (c, "xMaxYMax")) {
                    align = SP_ASPECT_XMAX_YMAX;
                } else {
                    break;
                }
                clip = SP_ASPECT_MEET;
                while (*e && *e == 32) e += 1;
                if (*e) {
                    if (!strcmp (e, "meet")) {
                        clip = SP_ASPECT_MEET;
                    } else if (!strcmp (e, "slice")) {
                        clip = SP_ASPECT_SLICE;
                    } else {
                        break;
                    }
                }
                image->aspect_align = align;
                image->aspect_clip = clip;
            }
            break;
#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
        case SP_PROP_COLOR_PROFILE:
            if ( image->color_profile ) {
                g_free (image->color_profile);
            }
            image->color_profile = (value) ? g_strdup (value) : NULL;
#ifdef DEBUG_LCMS
            if ( value ) {
                DEBUG_MESSAGE( lcmsFour, "<image> color-profile set to '%s'", value );
            } else {
                DEBUG_MESSAGE( lcmsFour, "<image> color-profile cleared" );
            }
#endif // DEBUG_LCMS
            // TODO check on this HREF_MODIFIED flag
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_IMAGE_HREF_MODIFIED_FLAG);
            break;
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
        default:
            if (((SPObjectClass *) (parent_class))->set)
                ((SPObjectClass *) (parent_class))->set (object, key, value);
            break;
    }

    sp_image_set_curve(image); //creates a curve at the image's boundary for snapping
}

static void sp_image_update( SPObject *object, SPCtx *ctx, unsigned int flags )
{
    SPImage *image = SP_IMAGE(object);
    SPDocument *doc = object->document;

    if (((SPObjectClass *) (parent_class))->update) {
        ((SPObjectClass *) (parent_class))->update (object, ctx, flags);
    }

    if (flags & SP_IMAGE_HREF_MODIFIED_FLAG) {
        if (image->pixbuf) {
            g_object_unref (image->pixbuf);
            image->pixbuf = NULL;
        }
        if ( image->pixPath ) {
            g_free(image->pixPath);
            image->pixPath = 0;
        }
        image->lastMod = 0;
        if (image->href) {
            GdkPixbuf *pixbuf;
            pixbuf = sp_image_repr_read_image (
                image->lastMod,
                image->pixPath,

                //XML Tree being used directly while it shouldn't be.
                object->getRepr()->attribute("xlink:href"),

                //XML Tree being used directly while it shouldn't be.
                object->getRepr()->attribute("sodipodi:absref"),
                doc->getBase());
            if (pixbuf) {
                pixbuf = sp_image_pixbuf_force_rgba (pixbuf);
// BLIP
#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
                if ( image->color_profile )
                {
                    int imagewidth = gdk_pixbuf_get_width( pixbuf );
                    int imageheight = gdk_pixbuf_get_height( pixbuf );
                    int rowstride = gdk_pixbuf_get_rowstride( pixbuf );
                    guchar* px = gdk_pixbuf_get_pixels( pixbuf );

                    if ( px ) {
#ifdef DEBUG_LCMS
                        DEBUG_MESSAGE( lcmsFive, "in <image>'s sp_image_update. About to call colorprofile_get_handle()" );
#endif // DEBUG_LCMS
                        guint profIntent = Inkscape::RENDERING_INTENT_UNKNOWN;
                        cmsHPROFILE prof = Inkscape::CMSSystem::getHandle( object->document,
                                                                           &profIntent,
                                                                           image->color_profile );
                        if ( prof ) {
                            cmsProfileClassSignature profileClass = cmsGetDeviceClass( prof );
                            if ( profileClass != cmsSigNamedColorClass ) {
                                int intent = INTENT_PERCEPTUAL;
                                switch ( profIntent ) {
                                    case Inkscape::RENDERING_INTENT_RELATIVE_COLORIMETRIC:
                                        intent = INTENT_RELATIVE_COLORIMETRIC;
                                        break;
                                    case Inkscape::RENDERING_INTENT_SATURATION:
                                        intent = INTENT_SATURATION;
                                        break;
                                    case Inkscape::RENDERING_INTENT_ABSOLUTE_COLORIMETRIC:
                                        intent = INTENT_ABSOLUTE_COLORIMETRIC;
                                        break;
                                    case Inkscape::RENDERING_INTENT_PERCEPTUAL:
                                    case Inkscape::RENDERING_INTENT_UNKNOWN:
                                    case Inkscape::RENDERING_INTENT_AUTO:
                                    default:
                                        intent = INTENT_PERCEPTUAL;
                                }
                                cmsHPROFILE destProf = cmsCreate_sRGBProfile();
                                cmsHTRANSFORM transf = cmsCreateTransform( prof,
                                                                           TYPE_RGBA_8,
                                                                           destProf,
                                                                           TYPE_RGBA_8,
                                                                           intent, 0 );
                                if ( transf ) {
                                    guchar* currLine = px;
                                    for ( int y = 0; y < imageheight; y++ ) {
                                        // Since the types are the same size, we can do the transformation in-place
                                        cmsDoTransform( transf, currLine, currLine, imagewidth );
                                        currLine += rowstride;
                                    }

                                    cmsDeleteTransform( transf );
                                }
#ifdef DEBUG_LCMS
                                else
                                {
                                    DEBUG_MESSAGE( lcmsSix, "in <image>'s sp_image_update. Unable to create LCMS transform." );
                                }
#endif // DEBUG_LCMS
                                cmsCloseProfile( destProf );
                            }
#ifdef DEBUG_LCMS
                            else
                            {
                                DEBUG_MESSAGE( lcmsSeven, "in <image>'s sp_image_update. Profile type is named color. Can't transform." );
                            }
#endif // DEBUG_LCMS
                        }
#ifdef DEBUG_LCMS
                        else
                        {
                            DEBUG_MESSAGE( lcmsEight, "in <image>'s sp_image_update. No profile found." );
                        }
#endif // DEBUG_LCMS
                    }
                }
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
                image->pixbuf = pixbuf;
                // convert to premultiplied native-endian ARGB for display with Cairo
                convert_pixbuf_normal_to_argb32(image->pixbuf);
            }
        }
    }

    if (image->pixbuf) {
        /* fixme: We are slightly violating spec here (Lauris) */
        if (!image->width._set) {
            image->width.computed = gdk_pixbuf_get_width(image->pixbuf);
        }
        if (!image->height._set) {
            image->height.computed = gdk_pixbuf_get_height(image->pixbuf);
        }
    }

    Geom::Point p(image->x.computed, image->y.computed);
    Geom::Point wh(image->width.computed, image->height.computed);
    image->clipbox = Geom::Rect(p, p + wh);

    image->ox = image->x.computed;
    image->oy = image->y.computed;

    int pixwidth = gdk_pixbuf_get_width (image->pixbuf);
    int pixheight = gdk_pixbuf_get_height (image->pixbuf);

    image->sx = image->width.computed / pixwidth;
    image->sy = image->height.computed / pixheight;

    // preserveAspectRatio calculate bounds / clipping rectangle -- EAF
    if (image->pixbuf && (image->aspect_align != SP_ASPECT_NONE)) {
        double x, y;

        switch (image->aspect_align) {
            case SP_ASPECT_XMIN_YMIN:
                x = 0.0;
                y = 0.0;
                break;
            case SP_ASPECT_XMID_YMIN:
                x = 0.5;
                y = 0.0;
                break;
            case SP_ASPECT_XMAX_YMIN:
                x = 1.0;
                y = 0.0;
                break;
            case SP_ASPECT_XMIN_YMID:
                x = 0.0;
                y = 0.5;
                break;
            case SP_ASPECT_XMID_YMID:
                x = 0.5;
                y = 0.5;
                break;
            case SP_ASPECT_XMAX_YMID:
                x = 1.0;
                y = 0.5;
                break;
            case SP_ASPECT_XMIN_YMAX:
                x = 0.0;
                y = 1.0;
                break;
            case SP_ASPECT_XMID_YMAX:
                x = 0.5;
                y = 1.0;
                break;
            case SP_ASPECT_XMAX_YMAX:
                x = 1.0;
                y = 1.0;
                break;
            default:
                x = 0.0;
                y = 0.0;
                break;
        }

        if (image->aspect_clip == SP_ASPECT_SLICE) {
            double scale = std::max(image->sx, image->sy);
            image->sx = scale;
            image->sy = scale;
        } else {
            double scale = std::min(image->sx, image->sy);
            image->sx = scale;
            image->sy = scale;
        }

        double vw = pixwidth * image->sx;
        double vh = pixheight * image->sy;
        image->ox += x * (image->width.computed - vw);
        image->oy += y * (image->height.computed - vh);
    }
    sp_image_update_canvas_image ((SPImage *) object);
}

static void sp_image_modified( SPObject *object, unsigned int flags )
{
    SPImage *image = SP_IMAGE (object);

    if (((SPObjectClass *) (parent_class))->modified) {
      (* ((SPObjectClass *) (parent_class))->modified) (object, flags);
    }

    if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
        for (SPItemView *v = image->display; v != NULL; v = v->next) {
            Inkscape::DrawingImage *img = dynamic_cast<Inkscape::DrawingImage *>(v->arenaitem);
            img->setStyle(object->style);
        }
    }
}

static Inkscape::XML::Node *sp_image_write( SPObject *object, Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags )
{
    SPImage *image = SP_IMAGE (object);

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:image");
    }

    repr->setAttribute("xlink:href", image->href);
    /* fixme: Reset attribute if needed (Lauris) */
    if (image->x._set) {
        sp_repr_set_svg_double(repr, "x", image->x.computed);
    }
    if (image->y._set) {
        sp_repr_set_svg_double(repr, "y", image->y.computed);
    }
    if (image->width._set) {
        sp_repr_set_svg_double(repr, "width", image->width.computed);
    }
    if (image->height._set) {
        sp_repr_set_svg_double(repr, "height", image->height.computed);
    }

    //XML Tree being used directly here while it shouldn't be...
    repr->setAttribute("preserveAspectRatio", object->getRepr()->attribute("preserveAspectRatio"));
#if defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)
    if (image->color_profile) {
        repr->setAttribute("color-profile", image->color_profile);
    }
#endif // defined(HAVE_LIBLCMS1) || defined(HAVE_LIBLCMS2)

    if (((SPObjectClass *) (parent_class))->write) {
        ((SPObjectClass *) (parent_class))->write (object, xml_doc, repr, flags);
    }

    return repr;
}

static Geom::OptRect sp_image_bbox( SPItem const *item,Geom::Affine const &transform, SPItem::BBoxType /*type*/ )
{
    SPImage const &image = *SP_IMAGE(item);
    Geom::OptRect bbox;

    if ((image.width.computed > 0.0) && (image.height.computed > 0.0)) {
        bbox = Geom::Rect::from_xywh(image.x.computed, image.y.computed, image.width.computed, image.height.computed);
        *bbox *= transform;
    }
    return bbox;
}

static void sp_image_print( SPItem *item, SPPrintContext *ctx )
{
    SPImage *image = SP_IMAGE(item);

    if (image->pixbuf && (image->width.computed > 0.0) && (image->height.computed > 0.0) ) {
        GdkPixbuf *pb = gdk_pixbuf_copy(image->pixbuf);
        convert_pixbuf_argb32_to_normal(pb);

        guchar *px = gdk_pixbuf_get_pixels(pb);
        int w = gdk_pixbuf_get_width(pb);
        int h = gdk_pixbuf_get_height(pb);
        int rs = gdk_pixbuf_get_rowstride(pb);
        int pixskip = gdk_pixbuf_get_n_channels(pb) * gdk_pixbuf_get_bits_per_sample(pb) / 8;

        if (image->aspect_align == SP_ASPECT_NONE) {
            Geom::Affine t;
            Geom::Translate tp(image->x.computed, image->y.computed);
            Geom::Scale s(image->width.computed, -image->height.computed);
            Geom::Translate ti(0.0, -1.0);
            t = s * tp;
            t = ti * t;
            sp_print_image_R8G8B8A8_N(ctx, px, w, h, rs, t, item->style);
        } else { // preserveAspectRatio
            double vw = image->width.computed / image->sx;
            double vh = image->height.computed / image->sy;

            int trimwidth = std::min<int>(w, ceil(image->width.computed / vw * w));
            int trimheight = std::min<int>(h, ceil(image->height.computed / vh * h));
            int trimx = std::max<int>(0, floor((image->x.computed - image->ox) / vw * w));
            int trimy = std::max<int>(0, floor((image->y.computed - image->oy) / vh * h));

            double vx = std::max<double>(image->ox, image->x.computed);
            double vy = std::max<double>(image->oy, image->y.computed);
            double vcw = std::min<double>(image->width.computed, vw);
            double vch = std::min<double>(image->height.computed, vh);

            Geom::Affine t;
            Geom::Translate tp(vx, vy);
            Geom::Scale s(vcw, -vch);
            Geom::Translate ti(0.0, -1.0);
            t = s * tp;
            t = ti * t;
            sp_print_image_R8G8B8A8_N(ctx, px + trimx*pixskip + trimy*rs, trimwidth, trimheight, rs, t, item->style);
        }
    }
}

static gchar *sp_image_description( SPItem *item )
{
    SPImage *image = SP_IMAGE(item);
    char *href_desc;
    if (image->href) {
        href_desc = (strncmp(image->href, "data:", 5) == 0)
            ? g_strdup(_("embedded"))
            : xml_quote_strdup(image->href);
    } else {
        g_warning("Attempting to call strncmp() with a null pointer.");
        href_desc = g_strdup("(null_pointer)"); // we call g_free() on href_desc
    }

    char *ret = ( image->pixbuf == NULL
                  ? g_strdup_printf(_("<b>Image with bad reference</b>: %s"), href_desc)
                  : g_strdup_printf(_("<b>Image</b> %d &#215; %d: %s"),
                                    gdk_pixbuf_get_width(image->pixbuf),
                                    gdk_pixbuf_get_height(image->pixbuf),
                                    href_desc) );
    g_free(href_desc);
    return ret;
}

static Inkscape::DrawingItem *sp_image_show( SPItem *item, Inkscape::Drawing &drawing, unsigned int /*key*/, unsigned int /*flags*/ )
{
    SPImage * image = SP_IMAGE(item);
    Inkscape::DrawingImage *ai = new Inkscape::DrawingImage(drawing);

    sp_image_update_arenaitem(image, ai);

    return ai;
}

/*
 * utility function to try loading image from href
 *
 * docbase/relative_src
 * absolute_src
 *
 */

GdkPixbuf *sp_image_repr_read_image( time_t& modTime, char*& pixPath, const gchar *href, const gchar *absref, const gchar *base )
{
    GdkPixbuf *pixbuf = 0;
    modTime = 0;
    if ( pixPath ) {
        g_free(pixPath);
        pixPath = 0;
    }

    const gchar *filename = href;
    if (filename != NULL) {
        if (strncmp (filename,"file:",5) == 0) {
            gchar *fullname = g_filename_from_uri(filename, NULL, NULL);
            if (fullname) {
                // TODO check this. Was doing a UTF-8 to filename conversion here.
                pixbuf = Inkscape::IO::pixbuf_new_from_file (fullname, modTime, pixPath, NULL);
                if (pixbuf != NULL) {
                    return pixbuf;
                }
            }
        } else if (strncmp (filename,"data:",5) == 0) {
            /* data URI - embedded image */
            filename += 5;
            pixbuf = sp_image_repr_read_dataURI (filename);
            if (pixbuf != NULL) {
                return pixbuf;
            }
        } else {

            if (!g_path_is_absolute (filename)) {
                /* try to load from relative pos combined with document base*/
                const gchar *docbase = base;
                if (!docbase) {
                    docbase = ".";
                }
                gchar *fullname = g_build_filename(docbase, filename, NULL);

                // document base can be wrong (on the temporary doc when importing bitmap from a
                // different dir) or unset (when doc is not saved yet), so we check for base+href existence first,
                // and if it fails, we also try to use bare href regardless of its g_path_is_absolute
                if (g_file_test (fullname, G_FILE_TEST_EXISTS) && !g_file_test (fullname, G_FILE_TEST_IS_DIR)) {
                    pixbuf = Inkscape::IO::pixbuf_new_from_file( fullname, modTime, pixPath, NULL );
                    g_free (fullname);
                    if (pixbuf != NULL) {
                        return pixbuf;
                    }
                }
            }

            /* try filename as absolute */
            if (g_file_test (filename, G_FILE_TEST_EXISTS) && !g_file_test (filename, G_FILE_TEST_IS_DIR)) {
                pixbuf = Inkscape::IO::pixbuf_new_from_file( filename, modTime, pixPath, NULL );
                if (pixbuf != NULL) {
                    return pixbuf;
                }
            }
        }
    }

    /* at last try to load from sp absolute path name */
    filename = absref;
    if (filename != NULL) {
        // using absref is outside of SVG rules, so we must at least warn the user
        if ( base != NULL && href != NULL ) {
            g_warning ("<image xlink:href=\"%s\"> did not resolve to a valid image file (base dir is %s), now trying sodipodi:absref=\"%s\"", href, base, absref);
        } else {
            g_warning ("xlink:href did not resolve to a valid image file, now trying sodipodi:absref=\"%s\"", absref);
        }

        pixbuf = Inkscape::IO::pixbuf_new_from_file( filename, modTime, pixPath, NULL );
        if (pixbuf != NULL) {
            return pixbuf;
        }
    }
    /* Nope: We do not find any valid pixmap file :-( */
    pixbuf = gdk_pixbuf_new_from_xpm_data ((const gchar **) brokenimage_xpm);

    /* It should be included xpm, so if it still does not does load, */
    /* our libraries are broken */
    g_assert (pixbuf != NULL);

    return pixbuf;
}

static GdkPixbuf *sp_image_pixbuf_force_rgba( GdkPixbuf * pixbuf )
{
    GdkPixbuf* result;
    if (gdk_pixbuf_get_has_alpha(pixbuf)) {
        result = pixbuf;
    } else {
        result = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
        g_object_unref(pixbuf);
    }
    return result;
}

/* We assert that realpixbuf is either NULL or identical size to pixbuf */
static void
sp_image_update_arenaitem (SPImage *image, Inkscape::DrawingImage *ai)
{
    ai->setStyle(SP_OBJECT(image)->style);
    ai->setARGB32Pixbuf(image->pixbuf);
    ai->setOrigin(Geom::Point(image->ox, image->oy));
    ai->setScale(image->sx, image->sy);
    ai->setClipbox(image->clipbox);
}

static void sp_image_update_canvas_image(SPImage *image)
{
    SPItem *item = SP_ITEM(image);

    for (SPItemView *v = item->display; v != NULL; v = v->next) {
        sp_image_update_arenaitem(image, dynamic_cast<Inkscape::DrawingImage *>(v->arenaitem));
    }
}

static void sp_image_snappoints( SPItem const *item, std::vector<Inkscape::SnapCandidatePoint> &p, Inkscape::SnapPreferences const *snapprefs )
{
    /* An image doesn't have any nodes to snap, but still we want to be able snap one image
    to another. Therefore we will create some snappoints at the corner, similar to a rect. If
    the image is rotated, then the snappoints will rotate with it. Again, just like a rect.
    */

    g_assert(item != NULL);
    g_assert(SP_IS_IMAGE(item));

    if (item->clip_ref->getObject()) {
        //We are looking at a clipped image: do not return any snappoints, as these might be
        //far far away from the visible part from the clipped image
        //TODO Do return snappoints, but only when within visual bounding box
    } else {
        if (snapprefs->isTargetSnappable(Inkscape::SNAPTARGET_IMG_CORNER)) {
            // The image has not been clipped: return its corners, which might be rotated for example
            SPImage &image = *SP_IMAGE(item);
            double const x0 = image.x.computed;
            double const y0 = image.y.computed;
            double const x1 = x0 + image.width.computed;
            double const y1 = y0 + image.height.computed;
            Geom::Affine const i2d (item->i2dt_affine ());
            p.push_back(Inkscape::SnapCandidatePoint(Geom::Point(x0, y0) * i2d, Inkscape::SNAPSOURCE_IMG_CORNER, Inkscape::SNAPTARGET_IMG_CORNER));
            p.push_back(Inkscape::SnapCandidatePoint(Geom::Point(x0, y1) * i2d, Inkscape::SNAPSOURCE_IMG_CORNER, Inkscape::SNAPTARGET_IMG_CORNER));
            p.push_back(Inkscape::SnapCandidatePoint(Geom::Point(x1, y1) * i2d, Inkscape::SNAPSOURCE_IMG_CORNER, Inkscape::SNAPTARGET_IMG_CORNER));
            p.push_back(Inkscape::SnapCandidatePoint(Geom::Point(x1, y0) * i2d, Inkscape::SNAPSOURCE_IMG_CORNER, Inkscape::SNAPTARGET_IMG_CORNER));
        }
    }
}

/*
 * Initially we'll do:
 * Transform x, y, set x, y, clear translation
 */

static Geom::Affine sp_image_set_transform( SPItem *item, Geom::Affine const &xform )
{
    SPImage *image = SP_IMAGE(item);

    /* Calculate position in parent coords. */
    Geom::Point pos( Geom::Point(image->x.computed, image->y.computed) * xform );

    /* This function takes care of translation and scaling, we return whatever parts we can't
       handle. */
    Geom::Affine ret(Geom::Affine(xform).withoutTranslation());
    Geom::Point const scale(hypot(ret[0], ret[1]),
                            hypot(ret[2], ret[3]));
    if ( scale[Geom::X] > MAGIC_EPSILON ) {
        ret[0] /= scale[Geom::X];
        ret[1] /= scale[Geom::X];
    } else {
        ret[0] = 1.0;
        ret[1] = 0.0;
    }
    if ( scale[Geom::Y] > MAGIC_EPSILON ) {
        ret[2] /= scale[Geom::Y];
        ret[3] /= scale[Geom::Y];
    } else {
        ret[2] = 0.0;
        ret[3] = 1.0;
    }

    image->width = image->width.computed * scale[Geom::X];
    image->height = image->height.computed * scale[Geom::Y];

    /* Find position in item coords */
    pos = pos * ret.inverse();
    image->x = pos[Geom::X];
    image->y = pos[Geom::Y];

    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);

    return ret;
}

static GdkPixbuf *sp_image_repr_read_dataURI( const gchar * uri_data )
{
    GdkPixbuf * pixbuf = NULL;

    gint data_is_image = 0;
    gint data_is_base64 = 0;

    const gchar * data = uri_data;

    while (*data) {
        if (strncmp(data,"base64",6) == 0) {
            /* base64-encoding */
            data_is_base64 = 1;
            data_is_image = 1; // Illustrator produces embedded images without MIME type, so we assume it's image no matter what
            data += 6;
        }
        else if (strncmp(data,"image/png",9) == 0) {
            /* PNG image */
            data_is_image = 1;
            data += 9;
        }
        else if (strncmp(data,"image/jpg",9) == 0) {
            /* JPEG image */
            data_is_image = 1;
            data += 9;
        }
        else if (strncmp(data,"image/jpeg",10) == 0) {
            /* JPEG image */
            data_is_image = 1;
            data += 10;
        }
        else { /* unrecognized option; skip it */
            while (*data) {
                if (((*data) == ';') || ((*data) == ',')) {
                    break;
                }
                data++;
            }
        }
        if ((*data) == ';') {
            data++;
            continue;
        }
        if ((*data) == ',') {
            data++;
            break;
        }
    }

    if ((*data) && data_is_image && data_is_base64) {
        pixbuf = sp_image_repr_read_b64(data);
    }

    return pixbuf;
}

static GdkPixbuf *sp_image_repr_read_b64( const gchar * uri_data )
{
    GdkPixbuf * pixbuf = NULL;

    static const gchar B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
    if (loader) {
        bool eos = false;
        bool failed = false;
        const gchar* btr = uri_data;
        gchar ud[4];
        guchar bd[57];

        while (!eos) {
            gint ell = 0;
            for (gint j = 0; j < 19; j++) {
                gint len = 0;
                for (gint k = 0; k < 4; k++) {
                    while (isspace ((int) (*btr))) {
                        if ((*btr) == '\0') break;
                        btr++;
                    }
                    if (eos) {
                        ud[k] = 0;
                        continue;
                    }
                    if (((*btr) == '\0') || ((*btr) == '=')) {
                        eos = true;
                        ud[k] = 0;
                        continue;
                    }
                    ud[k] = 64;
                    for (gint b = 0; b < 64; b++) { /* There must a faster way to do this... ?? */
                        if (B64[b] == (*btr)) {
                            ud[k] = (gchar) b;
                            break;
                        }
                    }
                    if (ud[k] == 64) { /* data corruption ?? */
                        eos = true;
                        ud[k] = 0;
                        continue;
                    }
                    btr++;
                    len++;
                }
                guint32 bits = (guint32) ud[0];
                bits = (bits << 6) | (guint32) ud[1];
                bits = (bits << 6) | (guint32) ud[2];
                bits = (bits << 6) | (guint32) ud[3];
                bd[ell++] = (guchar) ((bits & 0xff0000) >> 16);
                if (len > 2) {
                    bd[ell++] = (guchar) ((bits & 0xff00) >>  8);
                }
                if (len > 3) {
                    bd[ell++] = (guchar)  (bits & 0xff);
                }
            }

            if (!gdk_pixbuf_loader_write (loader, (const guchar *) bd, (size_t) ell, NULL)) {
                failed = true;
                break;
            }
        }

        gdk_pixbuf_loader_close (loader, NULL);

        if (!failed) {
            pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
        }
    }

    return pixbuf;
}

static void sp_image_set_curve( SPImage *image )
{
    //create a curve at the image's boundary for snapping
    if ((image->height.computed < MAGIC_EPSILON_TOO) || (image->width.computed < MAGIC_EPSILON_TOO) || (image->clip_ref->getObject())) {
        if (image->curve) {
            image->curve = image->curve->unref();
        }
    } else {
        Geom::OptRect rect = sp_image_bbox(image, Geom::identity(), SPItem::VISUAL_BBOX);
        SPCurve *c = SPCurve::new_from_rect(*rect, true);

        if (image->curve) {
            image->curve = image->curve->unref();
        }

        if (c) {
            image->curve = c->ref();

            c->unref();
        }
    }
}

/**
 * Return duplicate of curve (if any exists) or NULL if there is no curve
 */
SPCurve *sp_image_get_curve( SPImage *image )
{
    SPCurve *result = 0;
    if (image->curve) {
        result = image->curve->copy();
    }
    return result;
}

void sp_embed_image( Inkscape::XML::Node *image_node, GdkPixbuf *pb, Glib::ustring const &mime_in )
{
    Glib::ustring format, mime;
    if (mime_in == "image/jpeg") {
        mime = mime_in;
        format = "jpeg";
    } else {
        mime = "image/png";
        format = "png";
    }

    gchar *data = 0;
    gsize length = 0;
    gdk_pixbuf_save_to_buffer(pb, &data, &length, format.data(), NULL, NULL);

    // Save base64 encoded data in image node
    // this formula taken from Glib docs
    guint needed_size = length * 4 / 3 + length * 4 / (3 * 72) + 7;
    needed_size += 5 + 8 + mime.size(); // 5 bytes for data:, 8 for ;base64,

    gchar *buffer = (gchar *) g_malloc(needed_size), *buf_work = buffer;
    buf_work += g_sprintf(buffer, "data:%s;base64,", mime.data());

    gint state = 0;
    gint save = 0;
    gsize written = 0;
    written += g_base64_encode_step((guchar*) data, length, TRUE, buf_work, &state, &save);
    written += g_base64_encode_close(TRUE, buf_work + written, &state, &save);
    buf_work[written] = 0; // null terminate

    image_node->setAttribute("xlink:href", buffer);
    g_free(buffer);
}

void sp_image_refresh_if_outdated( SPImage* image )
{
    if ( image->href && image->lastMod ) {
        // It *might* change

        struct stat st;
        memset(&st, 0, sizeof(st));
        int val = g_stat(image->pixPath, &st);
        if ( !val ) {
            // stat call worked. Check time now
            if ( st.st_mtime != image->lastMod ) {
                SPCtx *ctx = 0;
                unsigned int flags = SP_IMAGE_HREF_MODIFIED_FLAG;
                sp_image_update(image, ctx, flags);
            }
        }
    }
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
