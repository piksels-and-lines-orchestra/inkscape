#define INKSCAPE_CTRL_C

/*
 * SPCtrl
 *
 * We render it by hand to reduce allocing/freeing svps & to get clean
 *    (non-aa) images
 *
 */

#include <2geom/transforms.h>
#include "sp-canvas-util.h"
#include "display-forward.h"
#include "sodipodi-ctrl.h"
#include "libnr/nr-pixops.h"
#include "display/inkscape-cairo.h"

enum {
    ARG_0,
    ARG_SHAPE,
    ARG_MODE,
    ARG_ANCHOR,
    ARG_SIZE,
    ARG_FILLED,
    ARG_FILL_COLOR,
    ARG_STROKED,
    ARG_STROKE_COLOR,
    ARG_PIXBUF
};


static void sp_ctrl_class_init (SPCtrlClass *klass);
static void sp_ctrl_init (SPCtrl *ctrl);
static void sp_ctrl_destroy (GtkObject *object);
static void sp_ctrl_set_arg (GtkObject *object, GtkArg *arg, guint arg_id);

static void sp_ctrl_update (SPCanvasItem *item, Geom::Matrix const &affine, unsigned int flags);
static void sp_ctrl_render (SPCanvasItem *item, SPCanvasBuf *buf);

static double sp_ctrl_point (SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item);


static SPCanvasItemClass *parent_class;

GtkType
sp_ctrl_get_type (void)
{
    static GtkType ctrl_type = 0;
    if (!ctrl_type) {
        static GTypeInfo const ctrl_info = {
            sizeof (SPCtrlClass),
            NULL,   /* base_init */
            NULL,   /* base_finalize */
            (GClassInitFunc) sp_ctrl_class_init,
            NULL,   /* class_finalize */
            NULL,   /* class_data */
            sizeof (SPCtrl),
            0,   /* n_preallocs */
            (GInstanceInitFunc) sp_ctrl_init,
            NULL
        };
        ctrl_type = g_type_register_static (SP_TYPE_CANVAS_ITEM, "SPCtrl", &ctrl_info, (GTypeFlags)0);
    }
    return ctrl_type;
}

static void
sp_ctrl_class_init (SPCtrlClass *klass)
{
    GtkObjectClass *object_class;
    SPCanvasItemClass *item_class;

    object_class = (GtkObjectClass *) klass;
    item_class = (SPCanvasItemClass *) klass;

    parent_class = (SPCanvasItemClass *)gtk_type_class (sp_canvas_item_get_type ());

    gtk_object_add_arg_type ("SPCtrl::shape", GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_SHAPE);
    gtk_object_add_arg_type ("SPCtrl::mode", GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_MODE);
    gtk_object_add_arg_type ("SPCtrl::anchor", GTK_TYPE_ANCHOR_TYPE, GTK_ARG_READWRITE, ARG_ANCHOR);
    gtk_object_add_arg_type ("SPCtrl::size", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_SIZE);
    gtk_object_add_arg_type ("SPCtrl::pixbuf", GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_PIXBUF);
    gtk_object_add_arg_type ("SPCtrl::filled", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_FILLED);
    gtk_object_add_arg_type ("SPCtrl::fill_color", GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_FILL_COLOR);
    gtk_object_add_arg_type ("SPCtrl::stroked", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_STROKED);
    gtk_object_add_arg_type ("SPCtrl::stroke_color", GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_STROKE_COLOR);

    object_class->destroy = sp_ctrl_destroy;
    object_class->set_arg = sp_ctrl_set_arg;

    item_class->update = sp_ctrl_update;
    item_class->render = sp_ctrl_render;
    item_class->point = sp_ctrl_point;
}

static void
sp_ctrl_init (SPCtrl *ctrl)
{
    ctrl->shape = SP_CTRL_SHAPE_SQUARE;
    ctrl->mode = SP_CTRL_MODE_COLOR;
    ctrl->anchor = GTK_ANCHOR_CENTER;
    ctrl->span = 3;
    ctrl->defined = TRUE;
    ctrl->shown = FALSE;
    ctrl->build = FALSE;
    ctrl->filled = 1;
    ctrl->stroked = 0;
    ctrl->fill_color = 0x000000ff;
    ctrl->stroke_color = 0x000000ff;
    ctrl->_moved = false;

    ctrl->box.x0 = ctrl->box.y0 = ctrl->box.x1 = ctrl->box.y1 = 0;
    ctrl->cache = NULL;
    ctrl->pixbuf = NULL;

    ctrl->_point = Geom::Point(0,0);
}

static void
sp_ctrl_destroy (GtkObject *object)
{
    SPCtrl *ctrl;

    g_return_if_fail (object != NULL);
    g_return_if_fail (SP_IS_CTRL (object));

    ctrl = SP_CTRL (object);

    if (ctrl->cache) {
        g_free(ctrl->cache);
        ctrl->cache = NULL;
    }

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_ctrl_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
    SPCanvasItem *item;
    SPCtrl *ctrl;
    GdkPixbuf * pixbuf = NULL;

    item = SP_CANVAS_ITEM (object);
    ctrl = SP_CTRL (object);

    switch (arg_id) {
        case ARG_SHAPE:
            ctrl->shape = (SPCtrlShapeType)(GTK_VALUE_INT (*arg));
            ctrl->build = FALSE;
            sp_canvas_item_request_update (item);
            break;

        case ARG_MODE:
            ctrl->mode = (SPCtrlModeType)(GTK_VALUE_INT (*arg));
            ctrl->build = FALSE;
            sp_canvas_item_request_update (item);
            break;

        case ARG_ANCHOR:
            ctrl->anchor = (GtkAnchorType)(GTK_VALUE_INT (*arg));
            ctrl->build = FALSE;
            sp_canvas_item_request_update (item);
            break;

        case ARG_SIZE:
            ctrl->span = (gint) ((GTK_VALUE_DOUBLE (*arg) - 1.0) / 2.0 + 0.5);
            ctrl->defined = (ctrl->span > 0);
            ctrl->build = FALSE;
            sp_canvas_item_request_update (item);
            break;

        case ARG_FILLED:
            ctrl->filled = GTK_VALUE_BOOL (*arg);
            ctrl->build = FALSE;
            sp_canvas_item_request_update (item);
            break;

        case ARG_FILL_COLOR: {
            // treat colors with zero alpha as opaque
            guint32 fill = GTK_VALUE_INT (*arg);
            fill = ((fill & 0xff) == 0 && fill) ? fill | 0xff : fill;
            ctrl->fill_color = fill;
            ctrl->build = FALSE;
            sp_canvas_item_request_update (item);
            } break;

        case ARG_STROKED:
            ctrl->stroked = GTK_VALUE_BOOL (*arg);
            ctrl->build = FALSE;
            sp_canvas_item_request_update (item);
            break;

        case ARG_STROKE_COLOR: {
            // treat colors with zero alpha as opaque
            guint32 stroke = GTK_VALUE_INT (*arg);
            stroke = ((stroke & 0xff) == 0 && stroke) ? stroke | 0xff : stroke;
            ctrl->stroke_color = stroke;
            ctrl->build = FALSE;
            sp_canvas_item_request_update (item);
            } break;

        case ARG_PIXBUF:
            pixbuf  = (GdkPixbuf*)(GTK_VALUE_POINTER (*arg));
            if (gdk_pixbuf_get_has_alpha (pixbuf)) {
                ctrl->pixbuf = pixbuf;
            } else {
                ctrl->pixbuf = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);
                gdk_pixbuf_unref (pixbuf);
            }
            ctrl->build = FALSE;
            break;

        default:
            break;
    }
}

static void
sp_ctrl_update (SPCanvasItem *item, Geom::Matrix const &affine, unsigned int flags)
{
    SPCtrl *ctrl;
    gint x, y;

    ctrl = SP_CTRL (item);

    if (((SPCanvasItemClass *) parent_class)->update)
        (* ((SPCanvasItemClass *) parent_class)->update) (item, affine, flags);

    sp_canvas_item_reset_bounds (item);

    if (!ctrl->_moved) return;

    if (ctrl->shown) {
        sp_canvas_request_redraw (item->canvas, ctrl->box.x0, ctrl->box.y0, ctrl->box.x1 + 1, ctrl->box.y1 + 1);
    }

    if (!ctrl->defined) return;

    x = (gint) ((affine[4] > 0) ? (affine[4] + 0.5) : (affine[4] - 0.5)) - ctrl->span;
    y = (gint) ((affine[5] > 0) ? (affine[5] + 0.5) : (affine[5] - 0.5)) - ctrl->span;

    switch (ctrl->anchor) {
        case GTK_ANCHOR_N:
        case GTK_ANCHOR_CENTER:
        case GTK_ANCHOR_S:
            break;

        case GTK_ANCHOR_NW:
        case GTK_ANCHOR_W:
        case GTK_ANCHOR_SW:
            x += ctrl->span;
            break;

        case GTK_ANCHOR_NE:
        case GTK_ANCHOR_E:
        case GTK_ANCHOR_SE:
            x -= (ctrl->span + 1);
            break;
    }

    switch (ctrl->anchor) {
        case GTK_ANCHOR_W:
        case GTK_ANCHOR_CENTER:
        case GTK_ANCHOR_E:
            break;

        case GTK_ANCHOR_NW:
        case GTK_ANCHOR_N:
        case GTK_ANCHOR_NE:
            y += ctrl->span;
            break;

        case GTK_ANCHOR_SW:
        case GTK_ANCHOR_S:
        case GTK_ANCHOR_SE:
            y -= (ctrl->span + 1);
            break;
    }

    ctrl->box.x0 = x;
    ctrl->box.y0 = y;
    ctrl->box.x1 = ctrl->box.x0 + 2 * ctrl->span;
    ctrl->box.y1 = ctrl->box.y0 + 2 * ctrl->span;

    sp_canvas_update_bbox (item, ctrl->box.x0, ctrl->box.y0, ctrl->box.x1 + 1, ctrl->box.y1 + 1);
}

static double
sp_ctrl_point (SPCanvasItem *item, Geom::Point p, SPCanvasItem **actual_item)
{
    SPCtrl *ctrl = SP_CTRL (item);

    *actual_item = item;

    double const x = p[Geom::X];
    double const y = p[Geom::Y];

    if ((x >= ctrl->box.x0) && (x <= ctrl->box.x1) && (y >= ctrl->box.y0) && (y <= ctrl->box.y1)) return 0.0;

    return 1e18;
}

static void
sp_ctrl_build_cache (SPCtrl *ctrl)
{
    //guchar * p, *q;
    //int size, x, y, z, s, a, side, c;
    //guint8 fr, fg, fb, fa, sr, sg, sb, sa;

    /*if (ctrl->filled) {
        fr = (ctrl->fill_color >> 24) & 0xff;
        fg = (ctrl->fill_color >> 16) & 0xff;
        fb = (ctrl->fill_color >> 8) & 0xff;
        fa = (ctrl->fill_color) & 0xff;
    } else {
        fr = 0x00; fg = 0x00; fb = 0x00; fa = 0x00;
    }
    if (ctrl->stroked) {
        sr = (ctrl->stroke_color >> 24) & 0xff;
        sg = (ctrl->stroke_color >> 16) & 0xff;
        sb = (ctrl->stroke_color >> 8) & 0xff;
        sa = (ctrl->stroke_color) & 0xff;
    } else {
        sr = fr; sg = fg; sb = fb; sa = fa;
    }*/

    int w, h; // for clarity; w and h are always odd
    w = h = (ctrl->span * 2 +1);
    int c = ctrl->span ;

    if (ctrl->cache) {
        cairo_surface_finish(ctrl->cache);
        cairo_surface_destroy(ctrl->cache);
    }
    ctrl->cache = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = cairo_create(ctrl->cache);

    bool supress_fill = false;
    bool supress_paint = false;

    switch (ctrl->shape) {
        case SP_CTRL_SHAPE_SQUARE:
            cairo_rectangle(cr, 0, 0, w, h);
            ctrl->build = TRUE;
            break;

        case SP_CTRL_SHAPE_DIAMOND:
            cairo_move_to(cr, c, 0); // c stands for "center" - it is half of the width / height
            cairo_line_to(cr, w, c);
            cairo_line_to(cr, c, h);
            cairo_line_to(cr, 0, c);
            cairo_close_path(cr);
            ctrl->build = TRUE;
            break;

        case SP_CTRL_SHAPE_CIRCLE:
            cairo_arc(cr, 0.5+c, 0.5+c, c, 0, 2*M_PI);
            cairo_close_path(cr);
            ctrl->build = TRUE;
            break;

        case SP_CTRL_SHAPE_CROSS:
            cairo_move_to(cr, 0.5+c, 0); // top stroke
            cairo_line_to(cr, 0.5+c, c);
            cairo_move_to(cr, w, 0.5+c); // right stroke
            cairo_line_to(cr, w, c+1);
            cairo_move_to(cr, 0.5+c, h); // bottom stroke
            cairo_line_to(cr, 0.5+c, c+1);
            cairo_move_to(cr, 0, 0.5+c); // left stroke
            cairo_line_to(cr, c, 0.5+c);
            supress_fill = true;
            ctrl->build = TRUE;
            break;

        case SP_CTRL_SHAPE_BITMAP:
            if (ctrl->pixbuf) {
                gdk_cairo_set_source_pixbuf(cr, ctrl->pixbuf, 0, 0);
                cairo_paint(cr);
                cairo_surface_flush(ctrl->cache);

                // TODO lame!!! find a way to do this without direct pixel manipulation.
                int stride = cairo_image_surface_get_stride(ctrl->cache);
                guint32 *px = reinterpret_cast<guint32*>(cairo_image_surface_get_data(ctrl->cache));

                // fix byte order. fill_color is 0xrrggbbaa, cairo needs 0xaarrggbb.
                // both quantities are native-endian, so it should be portable.
                guint32 fill = ctrl->fill_color;
                guint32 stroke = ctrl->stroke_color;
                fill = ((fill & 0xff) << 24) | ((fill & 0xffffff00) >> 8);
                stroke = ((stroke & 0xff) << 24) | ((stroke & 0xffffff00) >> 8);

                for (int i = 0; i < h; ++i) {
                    for (int j = 0; j < w; ++j) {
                        int index = i * stride / 4 + j;
                        if (px[index] & 0xff000000) {
                            px[index] = px[index] ? stroke : fill;
                        } else {
                            px[index] = 0;
                        }
                    }
                }
                cairo_surface_mark_dirty(ctrl->cache);
                supress_paint = true;
            } else {
                g_print ("control has no pixmap\n");
            }
            ctrl->build = TRUE;
            break;

        case SP_CTRL_SHAPE_IMAGE:
            if (ctrl->pixbuf) {
                gdk_cairo_set_source_pixbuf(cr, ctrl->pixbuf, 0, 0);
                cairo_paint(cr);
                supress_paint = true;
            } else {
                g_print ("control has no pixmap\n");
            }
            ctrl->build = TRUE;
            break;

        default:
            break;
    }

    if (ctrl->build && !supress_paint) {
        if (ctrl->filled && !supress_fill) {
            ink_cairo_set_source_rgba32(cr, ctrl->fill_color);
            cairo_fill_preserve(cr);
        }
        if (ctrl->stroked) {
            ink_cairo_set_source_rgba32(cr, ctrl->stroke_color);
            cairo_set_line_width(cr, 2);
            cairo_clip_preserve(cr);
            cairo_stroke(cr);
        }
    }

    cairo_destroy(cr);
}

// composite background, foreground, alpha for xor mode
#define COMPOSE_X(b,f,a) ( FAST_DIVIDE<255>( ((guchar) b) * ((guchar) (0xff - a)) + ((guchar) ((b ^ ~f) + b/4 - (b>127? 63 : 0))) * ((guchar) a) ) )
// composite background, foreground, alpha for color mode
#define COMPOSE_N(b,f,a) ( FAST_DIVIDE<255>( ((guchar) b) * ((guchar) (0xff - a)) + ((guchar) f) * ((guchar) a) ) )

static void
sp_ctrl_render (SPCanvasItem *item, SPCanvasBuf *buf)
{
    //gint y0, y1, y, x0,x1,x;
    //guchar *p, *q, a;

    SPCtrl *ctrl = SP_CTRL (item);

    if (!ctrl->defined) return;
    if ((!ctrl->filled) && (!ctrl->stroked)) return;

    // the control-image is rendered into ctrl->cache
    if (!ctrl->build) {
        sp_ctrl_build_cache (ctrl);
    }

    cairo_set_source_surface(buf->ct, ctrl->cache,
        ctrl->box.x0 - buf->rect.x0, ctrl->box.y0 - buf->rect.y0);
    cairo_paint(buf->ct);

    /*
    double x0 = ctrl->box.x0;
    double y0 = ctrl->box.y0;
    double w = ctrl->box.x1 - ctrl->box.x0 + 1;
    double h = ctrl->box.y1 - ctrl->box.y0 + 1;
    //guint32 fill = ctrl->fill_color;
    //fill = (fill & 0xff == 0 && fill) ? fill | 0xff : fill;
    

    switch (ctrl->shape) {
    case SP_CTRL_SHAPE_SQUARE:
        cairo_rectangle(buf->ct, x0, y0, w, h);
        break;
    case SP_CTRL_SHAPE_DIAMOND:
        cairo_move_to(buf->ct, x0 + w/2, y0);
        cairo_line_to(buf->ct, x0 + w, y0 + h/2);
        cairo_line_to(buf->ct, x0 + w/2, y0 + h);
        cairo_line_to(buf->ct, x0, y0 + h/2);
        cairo_close_path(buf->ct);
        break;
    //case SP_CTRL_SHAPE_CIRCLE:
    default:
        cairo_arc(buf->ct, x0 + w/2, y0 + h/2, w/2, 0, 2*M_PI);
        cairo_close_path(buf->ct);
        break;
    }

    //if (ctrl->mode == SP_CTRL_MODE_XOR) {
    //    cairo_set_operator(buf->ct, CAIRO_OPERATOR_XOR);
    //}
    if (ctrl->filled) {
        ink_cairo_set_source_rgba32(buf->ct, ctrl->fill_color);
        cairo_fill_preserve(buf->ct);
    }
    if (ctrl->stroked) {
        ink_cairo_set_source_rgba32(buf->ct, ctrl->stroke_color);
        cairo_set_line_width(buf->ct, 2);
        cairo_clip_preserve(buf->ct);
        cairo_stroke_preserve(buf->ct);
    }

    cairo_new_path(buf->ct);
    cairo_restore(buf->ct);*/

    #if 0
    // then we render from ctrl->cache
    y0 = MAX (ctrl->box.y0, buf->rect.y0);
    y1 = MIN (ctrl->box.y1, buf->rect.y1 - 1);
    x0 = MAX (ctrl->box.x0, buf->rect.x0);
    x1 = MIN (ctrl->box.x1, buf->rect.x1 - 1);

    for (y = y0; y <= y1; y++) {
        p = buf->buf + (y - buf->rect.y0) * buf->buf_rowstride + (x0 - buf->rect.x0) * 4;
        q = ctrl->cache + ((y - ctrl->box.y0) * (ctrl->span*2+1) + (x0 - ctrl->box.x0)) * 4;
        for (x = x0; x <= x1; x++) {
            a = *(q + 3);
            // 00000000 is the only way to get invisible; all other colors with alpha 00 are treated as mode_color with alpha ff
            colormode = false;
            if (a == 0x00 && !(q[0] == 0x00 && q[1] == 0x00 && q[2] == 0x00)) {
                a = 0xff;
                colormode = true;
            }
            if (ctrl->mode == SP_CTRL_MODE_COLOR || colormode) {
                p[0] = COMPOSE_N (p[0], q[0], a);
                p[1] = COMPOSE_N (p[1], q[1], a);
                p[2] = COMPOSE_N (p[2], q[2], a);
                q += 4;
                p += 4;
            } else if (ctrl->mode == SP_CTRL_MODE_XOR) {
                p[0] = COMPOSE_X (p[0], q[0], a);
                p[1] = COMPOSE_X (p[1], q[1], a);
                p[2] = COMPOSE_X (p[2], q[2], a);
                q += 4;
                p += 4;
            }
        }
    }
    #endif
    ctrl->shown = TRUE;
}

void SPCtrl::moveto (Geom::Point const p) {
    if (p != _point) {
        sp_canvas_item_affine_absolute (SP_CANVAS_ITEM (this), Geom::Matrix(Geom::Translate (p)));
        _moved = true;
    }
    _point = p;
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
