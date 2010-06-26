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
#include <cairomm/cairomm.h>
#include <2geom/forward.h>

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
