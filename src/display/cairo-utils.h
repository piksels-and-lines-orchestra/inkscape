/**
 * @file
 * @brief Cairo integration helpers
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2010 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_INKSCAPE_DISPLAY_CAIRO_UTILS_H
#define SEEN_INKSCAPE_DISPLAY_CAIRO_UTILS_H

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairomm/cairomm.h>
#include <2geom/forward.h>

struct SPColor;

namespace Inkscape {

/** @brief RAII idiom for Cairo groups.
 * Groups are temporary surfaces used when rendering e.g. masks and opacity.
 * Use this class to ensure that each group push is matched with a pop. */
class CairoGroup {
public:
    CairoGroup(cairo_t *_ct);
    ~CairoGroup();
    void push();
    void push_with_content(cairo_content_t content);
    cairo_pattern_t *pop();
    Cairo::RefPtr<Cairo::Pattern> popmm();
    void pop_to_source();
private:
    cairo_t *ct;
    bool pushed;
};

/** @brief RAII idiom for Cairo state saving */
class CairoSave {
public:
    CairoSave(cairo_t *_ct, bool save=false)
        : ct(_ct)
        , saved(save)
    {
        if (save) {
            cairo_save(ct);
        }
    }
    void save() {
        if (!saved) {
            cairo_save(ct);
            saved = true;
        }
    }
    ~CairoSave() {
        if (saved)
            cairo_restore(ct);
    }
private:
    cairo_t *ct;
    bool saved;
};

/** @brief Cairo context with Inkscape-specific operations */
class CairoContext : public Cairo::Context {
public:
    CairoContext(cairo_t *obj, bool ref = false);

    void transform(Geom::Matrix const &m);
    void set_source_rgba32(guint32 color);
    void append_path(Geom::PathVector const &pv);

    static Cairo::RefPtr<CairoContext> create(Cairo::RefPtr<Cairo::Surface> const &target);
};

} // namespace Inkscape

void ink_cairo_set_source_color(cairo_t *ct, SPColor const &color, double opacity);
void ink_cairo_set_source_rgba32(cairo_t *ct, guint32 rgba);
void ink_cairo_transform(cairo_t *ct, Geom::Matrix const &m);
void ink_cairo_pattern_set_matrix(cairo_pattern_t *cp, Geom::Matrix const &m);
void ink_cairo_set_source_argb32_pixbuf(cairo_t *ct, GdkPixbuf *pb, double x, double y);

cairo_surface_t *ink_cairo_surface_copy(cairo_surface_t *s);
cairo_surface_t *ink_cairo_surface_create_identical(cairo_surface_t *s);
cairo_surface_t *ink_cairo_surface_create_same_size(cairo_surface_t *s, cairo_content_t c);
cairo_surface_t *ink_cairo_extract_alpha(cairo_surface_t *s);
cairo_surface_t *ink_cairo_surface_create_output(cairo_surface_t *image, cairo_surface_t *bg);
void ink_cairo_surface_blit(cairo_surface_t *src, cairo_surface_t *dest);
int ink_cairo_surface_get_width(cairo_surface_t *surface);
int ink_cairo_surface_get_height(cairo_surface_t *surface);

void convert_pixels_pixbuf_to_argb32(guchar *data, int w, int h, int rs);
void convert_pixels_argb32_to_pixbuf(guchar *data, int w, int h, int rs);
void convert_pixbuf_normal_to_argb32(GdkPixbuf *);
void convert_pixbuf_argb32_to_normal(GdkPixbuf *);

G_GNUC_CONST inline guint32
premul_alpha(guint32 color, guint32 alpha)
{
    guint32 temp = alpha * color + 128;
    return (temp + (temp >> 8)) >> 8;
}
G_GNUC_CONST inline guint32
unpremul_alpha(guint32 color, guint32 alpha)
{
    // NOTE: you must check for alpha != 0 yourself.
    return (255 * color + alpha/2) / alpha;
}

// TODO: move those to 2Geom
void feed_pathvector_to_cairo (cairo_t *ct, Geom::PathVector const &pathv, Geom::Matrix trans, Geom::OptRect area, bool optimize_stroke, double stroke_width);
void feed_pathvector_to_cairo (cairo_t *ct, Geom::PathVector const &pathv);

#endif
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
