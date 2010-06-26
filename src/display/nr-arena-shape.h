#ifndef __NR_ARENA_SHAPE_H__
#define __NR_ARENA_SHAPE_H__

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

#define NR_TYPE_ARENA_SHAPE (nr_arena_shape_get_type ())
#define NR_ARENA_SHAPE(obj) (NR_CHECK_INSTANCE_CAST ((obj), NR_TYPE_ARENA_SHAPE, NRArenaShape))
#define NR_IS_ARENA_SHAPE(obj) (NR_CHECK_INSTANCE_TYPE ((obj), NR_TYPE_ARENA_SHAPE))

#include <cairo.h>
#include "display/display-forward.h"
#include "display/canvas-bpath.h"
#include "forward.h"
#include "sp-paint-server.h"
#include "nr-arena-item.h"

#include "../color.h"

#include "../livarot/Shape.h"

NRType nr_arena_shape_get_type (void);

struct NRArenaShape : public NRArenaItem {
    class Paint {
    public:
        enum Type {
            NONE,
            COLOR,
            SERVER
        };

        Paint() : _type(NONE), _color(0), _server(NULL) {}
        Paint(Paint const &p) { _assign(p); }
        virtual ~Paint() { clear(); }

        Type type() const { return _type; }
        SPPaintServer *server() const { return _server; }
        SPColor const &color() const { return _color; }

        Paint &operator=(Paint const &p) {
            set(p);
            return *this;
        }

        void set(Paint const &p) {
            clear();
            _assign(p);
        }
        void set(SPColor const &color) {
            clear();
            _type = COLOR;
            _color = color;
        }
        void set(SPPaintServer *server) {
            clear();
            if (server) {
                _type = SERVER;
                _server = server;
                sp_object_ref(_server, NULL);
            }
        }
        void clear() {
            if ( _type == SERVER ) {
                sp_object_unref(_server, NULL);
                _server = NULL;
            }
            _type = NONE;
        }

    private:
        Type _type;
        SPColor _color;
        SPPaintServer *_server;

        void _assign(Paint const &p) {
            _type = p._type;
            _server = p._server;
            _color = p._color;
            if (_server) {
                sp_object_ref(_server, NULL);
            }
        }
    };

    /* Shape data */
    SPCurve *curve;
    SPStyle *style;
    NRRect paintbox;
    /* State data */
    Geom::Matrix ctm;

    cairo_pattern_t *fill_pattern;
    cairo_pattern_t *stroke_pattern;
    cairo_path_t *path;

    // delayed_shp=true means the *_shp polygons are not computed yet
    // they'll be computed on demand in *_render(), *_pick() or *_clip()
    // the goal is to not uncross polygons that are outside the viewing region
    bool    delayed_shp;
    // approximate bounding box, for the case when the polygons have been delayed
    NRRectL approx_bbox;

    /* Markers */
    NRArenaItem *markers;

    NRArenaItem *last_pick;
    guint repick_after;

    static NRArenaShape *create(NRArena *arena) {
        NRArenaShape *obj=reinterpret_cast<NRArenaShape *>(nr_object_new(NR_TYPE_ARENA_SHAPE));
        obj->init(arena);
        obj->key = 0;
        return obj;
    }

    void setFill(SPPaintServer *server);
    void setFill(SPColor const &color);
    void setFillOpacity(double opacity);
    void setFillRule(cairo_fill_rule_t rule);

    void setStroke(SPPaintServer *server);
    void setStroke(SPColor const &color);
    void setStrokeOpacity(double opacity);
    void setStrokeWidth(double width);
    void setLineCap(cairo_line_cap_t cap);
    void setLineJoin(cairo_line_join_t join);
    void setMitreLimit(double limit);

    void setPaintBox(Geom::Rect const &pbox);

    void _invalidateCachedFill() {
    }
    void _invalidateCachedStroke() {
    }

    struct Style {
        Style() : opacity(0.0) {}
        Paint paint;
        double opacity;
    };
    struct FillStyle : public Style {
        FillStyle() : rule(CAIRO_FILL_RULE_EVEN_ODD) {}
        cairo_fill_rule_t rule;
    } _fill;
    struct StrokeStyle : public Style {
        StrokeStyle()
            : cap(CAIRO_LINE_CAP_ROUND), join(CAIRO_LINE_JOIN_ROUND),
              width(0.0), mitre_limit(0.0)
        {}

        cairo_line_cap_t cap;
        cairo_line_join_t join;
        double width;
        double mitre_limit;
    } _stroke;
};

struct NRArenaShapeClass {
    NRArenaItemClass parent_class;
};

void nr_arena_shape_set_path(NRArenaShape *shape, SPCurve *curve, bool justTrans);
void nr_arena_shape_set_style(NRArenaShape *shape, SPStyle *style);
void nr_arena_shape_set_paintbox(NRArenaShape *shape, NRRect const *pbox);


#endif /* !__NR_ARENA_SHAPE_H__ */

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
