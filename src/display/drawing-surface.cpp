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

#include "display/drawing-surface.h"
#include "display/cairo-utils.h"

namespace Inkscape {

using Geom::X;
using Geom::Y;


/** @class DrawingSurface
 * @brief Drawing surface that remembers its origin.
 *
 * This is a very minimalistic wrapper over cairo_surface_t. The main
 * extra functionality provided by this class is that it automates
 * the mapping from "logical space" (coordinates in the rendering)
 * and the "physical space" (surface pixels). For example, patterns
 * have to be rendered on surfaces which have possibly non-integer
 * widths and heights.
 */

/** @brief Creates a surface with the given physical extents.
 * When a drawing context is created for this surface, its pixels
 * will cover the area under the given rectangle. */
DrawingSurface::DrawingSurface(Geom::IntRect const &area)
    : _surface(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, area.width(), area.height()))
    , _origin(area.min())
    , _scale(1, 1)
{}

/** @brief Creates a surface with the given logical extents.
 * When a drawing context is created for this surface, its pixels
 * will cover the area under the given rectangle. If the rectangle
 * has non-integer width, there will be slightly more than 1 pixel
 * per logical unit. */
DrawingSurface::DrawingSurface(Geom::Rect const &area)
    : _surface(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ceil(area.width()), ceil(area.height())))
    , _origin(area.min())
    , _scale(ceil(area.width()) / area.width(), ceil(area.height()) / area.height())
{}

/** @brief Creates a surface with the given logical and physical extents.
 * When a drawing context is created for this surface, its pixels
 * will cover the area under the given rectangle. IT will contain
 * the number of pixels specified by the second argument.
 * @param logbox Logical extents of the surface
 * @param pixdims Pixel dimensions of the surface. */
DrawingSurface::DrawingSurface(Geom::Rect const &logbox, Geom::IntPoint const &pixdims)
    : _surface(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixdims[X], pixdims[Y]))
    , _origin(logbox.min())
    , _scale(pixdims[X] / logbox.width(), pixdims[Y] / logbox.height())
{}

/** @brief Wrap a cairo_surface_t.
 * This constructor will take an extra reference on @a surface, which will
 * be released on destruction. */
DrawingSurface::DrawingSurface(cairo_surface_t *surface, Geom::Point const &origin)
    : _surface(surface)
    , _origin(origin)
    , _scale(1, 1)
{
    cairo_surface_reference(surface);
}

DrawingSurface::~DrawingSurface()
{
    cairo_surface_destroy(_surface);
}

/// Get the logical extents of the surface.
Geom::Rect
DrawingSurface::area() const
{
    Geom::Rect r = Geom::Rect::from_xywh(_origin, dimensions());
    return r;
}

/// Get the logical width and weight of the surface as a point.
Geom::Point
DrawingSurface::dimensions() const
{
    double w = cairo_image_surface_get_width(_surface);
    double h = cairo_image_surface_get_height(_surface);
    Geom::Point logical_dims(w / _scale[X], h / _scale[Y]);
    return logical_dims;
}

Geom::Point
DrawingSurface::origin() const
{
    return _origin;
}

Geom::Scale
DrawingSurface::scale() const
{
    return _scale;
}

/// Get the transformation applied to the drawing context on construction.
Geom::Affine
DrawingSurface::drawingTransform() const
{
    Geom::Affine ret = _scale * Geom::Translate(-_origin);
    return ret;
}

cairo_surface_type_t
DrawingSurface::type() const
{
    // currently hardcoded
    return CAIRO_SURFACE_TYPE_IMAGE;
}

/** @brief Create a drawing context for this surface.
 * It's better to use the surface constructor of DrawingContext. */
cairo_t *
DrawingSurface::createRawContext()
{
    cairo_t *ct = cairo_create(_surface);
    if (_scale != Geom::Scale::identity()) {
        cairo_scale(ct, _scale[X], _scale[Y]);
    }
    cairo_translate(ct, -_origin[X], -_origin[Y]);
    return ct;
}

} // end namespace Inkscape

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
