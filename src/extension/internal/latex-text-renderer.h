#ifndef EXTENSION_INTERNAL_PDF_LATEX_RENDERER_H_SEEN
#define EXTENSION_INTERNAL_PDF_LATEX_RENDERER_H_SEEN

/** \file
 * Declaration of LaTeXTextRenderer, used for rendering the accompanying LaTeX file when saving PDF output + LaTeX 
 */
/*
 * Authors:
 *  Johan Engelen <goejendaagh@zonnet.nl>
 *
 * Copyright (C) 2010 Authors
 * 
 * Licensed under GNU GPL
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "extension/extension.h"
#include <set>
#include <string>

#include "style.h"

#include <cairo.h>

#include <2geom/matrix.h>
#include <stack>

class SPClipPath;
class SPMask;
class SPItem;

namespace Inkscape {
namespace Extension {
namespace Internal {

bool latex_render_document_text_to_file(SPDocument *doc, gchar const *filename, const gchar * const exportId, bool exportDrawing, bool exportCanvas);

class LaTeXTextRenderer {
public:
    LaTeXTextRenderer();
    virtual ~LaTeXTextRenderer();

    bool setTargetFile(gchar const *filename);

    void setStateForItem(SPItem const *item);

//    void applyClipPath(CairoRenderContext *ctx, SPClipPath const *cp);
//    void applyMask(CairoRenderContext *ctx, SPMask const *mask);

    /** Initializes the LaTeXTextRenderer according to the specified
    SPDocument. Important to set the boundingbox to the pdf boundingbox */
    bool setupDocument(SPDocument *doc, bool pageBoundingBox, SPItem *base);

    /** Traverses the object tree and invokes the render methods. */
    void renderItem(SPItem *item);

protected:
    FILE * _stream;
    gchar * _filename;

    void push_transform(Geom::Matrix const &transform);
    Geom::Matrix const & transform();
    void pop_transform();
    std::stack<Geom::Matrix> _transform_stack;
    double _width;
    double _height;

    void writePreamble();
    void writePostamble();

    void sp_item_invoke_render(SPItem *item);
    void sp_root_render(SPItem *item);
    void sp_group_render(SPItem *item);
    void sp_use_render(SPItem *item);
    void sp_text_render(SPItem *item);
    void sp_flowtext_render(SPItem *item);
};

}  /* namespace Internal */
}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /* !EXTENSION_INTERNAL_CAIRO_RENDERER_H_SEEN */

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
