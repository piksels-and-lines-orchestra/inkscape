#ifndef EXTENSION_INTERNAL_PDF_LATEX_RENDERER_H_SEEN
#define EXTENSION_INTERNAL_PDF_LATEX_RENDERER_H_SEEN

/** \file
 * Declaration of PDFLaTeXRenderer, used for rendering the accompanying LaTeX file when saving PDF output + LaTeX 
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

class SPClipPath;
class SPMask;
class SPItem;

namespace Inkscape {
namespace Extension {
namespace Internal {

class PDFLaTeXRenderer {
public:
    PDFLaTeXRenderer();
    virtual ~PDFLaTeXRenderer();

    bool setTargetFile(gchar const *filename);

    void setStateForItem(SPItem const *item);

//    void applyClipPath(CairoRenderContext *ctx, SPClipPath const *cp);
//    void applyMask(CairoRenderContext *ctx, SPMask const *mask);

    /** Initializes the PDFLaTeXRenderer according to the specified
    SPDocument. Important to set the boundingbox to the pdf boundingbox */
    bool setupDocument(SPDocument *doc, bool pageBoundingBox, SPItem *base);

    /** Traverses the object tree and invokes the render methods. */
    void renderItem(SPItem *item);

protected:
    FILE * _stream;
    gchar * _filename;

    void transform(Geom::Matrix const &transform);
    Geom::Matrix _m; // the transform for current item
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
