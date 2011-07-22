/**
 * @file
 * @brief Cairo surface that remembers its origin
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_INKSCAPE_DISPLAY_DRAWING_SURFACE_H
#define SEEN_INKSCAPE_DISPLAY_DRAWING_SURFACE_H

#include <boost/shared_ptr.hpp>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <2geom/affine.h>
#include <2geom/rect.h>
#include <2geom/transforms.h>

namespace Inkscape {
class DrawingContext;

class DrawingSurface
{
public:
    explicit DrawingSurface(Geom::IntRect const &area);
    explicit DrawingSurface(Geom::Rect const &area);
    DrawingSurface(Geom::Rect const &logbox, Geom::IntPoint const &pixdims);
    DrawingSurface(cairo_surface_t *surface, Geom::Point const &origin);
    virtual ~DrawingSurface();

    Geom::Rect area() const;
    Geom::Point dimensions() const;
    Geom::Point origin() const;
    Geom::Scale scale() const;
    Geom::Affine drawingTransform() const;
    cairo_surface_type_t type() const;

    cairo_surface_t *raw() { return _surface; }
    cairo_t *createRawContext();

protected:
    cairo_surface_t *_surface;
    Geom::Point _origin;
    Geom::Scale _scale;
    bool _has_context;

    friend class DrawingContext;
};

class PixbufSurface
    : public DrawingSurface
{
public:
    explicit PixbufSurface(GdkPixbuf *pb, Geom::Point const &origin = Geom::Point(0,0));
    ~PixbufSurface();
protected:
    GdkPixbuf *pb;

    friend class DrawingContext;
};

} // end namespace Inkscape

#endif // !SEEN_INKSCAPE_DISPLAY_DRAWING_ITEM_H

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
