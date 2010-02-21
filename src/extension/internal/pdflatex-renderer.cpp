#define EXTENSION_INTERNAL_PDF_LATEX_RENDERER_CPP

/** \file
 * Rendering LaTeX file (pdf+latex output)
 */
/*
 * Authors:
 *   Johan Engelen <goejendaagh@zonnet.nl>
 *   Miklos Erdelyi <erdelyim@gmail.com>
 *
 * Copyright (C) 2006-2010 Authors
 *
 * Most of the pre- and postamble is copied from GNUPlot's epslatex terminal output :-)
 *
 * Licensed under GNU GPL
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef PANGO_ENABLE_BACKEND
#define PANGO_ENABLE_BACKEND
#endif

#ifndef PANGO_ENABLE_ENGINE
#define PANGO_ENABLE_ENGINE
#endif


#include <signal.h>
#include <errno.h>

#include "libnr/nr-rect.h"
#include "libnrtype/Layout-TNG.h"
#include <2geom/transforms.h>
#include <2geom/pathvector.h>

#include <glib/gmem.h>

#include <glibmm/i18n.h>
#include "display/nr-arena.h"
#include "display/nr-arena-item.h"
#include "display/nr-arena-group.h"
#include "display/curve.h"
#include "display/canvas-bpath.h"
#include "sp-item.h"
#include "sp-item-group.h"
#include "style.h"
#include "marker.h"
#include "sp-linear-gradient.h"
#include "sp-radial-gradient.h"
#include "sp-root.h"
#include "sp-use.h"
#include "sp-text.h"
#include "sp-flowtext.h"
#include "sp-mask.h"
#include "sp-clippath.h"
#include "text-editing.h"

#include <unit-constants.h>
#include "helper/png-write.h"
#include "helper/pixbuf-ops.h"

#include "pdflatex-renderer.h"
#include "extension/system.h"

#include "io/sys.h"

#include <cairo.h>

// include support for only the compiled-in surface types
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif

//#define TRACE(_args) g_message(_args)
#define TRACE(_args)
//#define TEST(_args) _args
#define TEST(_args)

// FIXME: expose these from sp-clippath/mask.cpp
struct SPClipPathView {
    SPClipPathView *next;
    unsigned int key;
    NRArenaItem *arenaitem;
    NRRect bbox;
};

struct SPMaskView {
    SPMaskView *next;
    unsigned int key;
    NRArenaItem *arenaitem;
    NRRect bbox;
};

namespace Inkscape {
namespace Extension {
namespace Internal {


PDFLaTeXRenderer::PDFLaTeXRenderer(void)
  : _stream(NULL),
    _filename(NULL),
    _width(0),
    _height(0)
{
    push_transform(Geom::identity());
}

PDFLaTeXRenderer::~PDFLaTeXRenderer(void)
{
    if (_stream) {
        writePostamble();

        fclose(_stream);
    }

    /* restore default signal handling for SIGPIPE */
#if !defined(_WIN32) && !defined(__WIN32__)
    (void) signal(SIGPIPE, SIG_DFL);
#endif

    if (_filename) {
        g_free(_filename);
    }

    return;
}

/** This should create the output LaTeX file, and assign it to _stream.
 * @return Returns true when succesfull
 */
bool
PDFLaTeXRenderer::setTargetFile(gchar const *filename) {
    if (filename != NULL) {
        while (isspace(*filename)) filename += 1;
        
        _filename = g_strdup(filename);

        gchar *filename_ext = g_strdup_printf("%s.tex", filename);
        Inkscape::IO::dump_fopen_call(filename_ext, "K");
        FILE *osf = Inkscape::IO::fopen_utf8name(filename_ext, "w+");
        if (!osf) {
            fprintf(stderr, "inkscape: fopen(%s): %s\n",
                    filename_ext, strerror(errno));
            return false;
        }
        _stream = osf;
        g_free(filename_ext);
    }

    if (_stream) {
        /* fixme: this is kinda icky */
#if !defined(_WIN32) && !defined(__WIN32__)
        (void) signal(SIGPIPE, SIG_IGN);
#endif
    }

    fprintf(_stream, "%%%% Creator: Inkscape %s, www.inkscape.org\n", PACKAGE_STRING);
    fprintf(_stream, "%%%% PDF + LaTeX output extension by Johan Engelen, 2010\n");
    fprintf(_stream, "%%%% Accompanies %s.pdf\n", _filename);
    /* flush this to test output stream as early as possible */
    if (fflush(_stream)) {
        if (ferror(_stream)) {
            g_print("Error %d on LaTeX file output stream: %s\n", errno,
                    g_strerror(errno));
        }
        g_print("Output to LaTeX file failed\n");
        /* fixme: should use pclose() for pipes */
        fclose(_stream);
        _stream = NULL;
        fflush(stdout);
        return false;
    }

    writePreamble();

    return true;
}

/* Most of this preamble is copied from GNUPlot's epslatex terminal output :-) */
static char const preamble[] =
"\\begingroup                                                                              \n"
"  \\makeatletter                                                                          \n"
"  \\providecommand\\color[2][]{%%                                                         \n"
"    \\GenericError{(gnuplot) \\space\\space\\space\\@spaces}{%%                           \n"
"      Package color not loaded in conjunction with                                        \n"
"      terminal option `colourtext'%%                                                      \n"
"    }{See the gnuplot documentation for explanation.%%                                    \n"
"    }{Either use 'blacktext' in gnuplot or load the package                               \n"
"      color.sty in LaTeX.}%%                                                              \n"
"    \\renewcommand\\color[2][]{}%%                                                        \n"
"  }%%                                                                                     \n"
"  \\providecommand\\includegraphics[2][]{%%                                               \n"
"    \\GenericError{(gnuplot) \\space\\space\\space\\@spaces}{%%                           \n"
"      Package graphicx or graphics not loaded%%                                           \n"
"    }{See the gnuplot documentation for explanation.%%                                    \n"
"    }{The gnuplot epslatex terminal needs graphicx.sty or graphics.sty.}%%                \n"
"    \\renewcommand\\includegraphics[2][]{}%%                                              \n"
"  }%%                                                                                     \n"
"  \\providecommand\\rotatebox[2]{#2}%%                                                    \n"
"  \\@ifundefined{ifGPcolor}{%%                                                            \n"
"    \\newif\\ifGPcolor                                                                    \n"
"    \\GPcolorfalse                                                                        \n"
"  }{}%%                                                                                   \n"
"  \\@ifundefined{ifGPblacktext}{%%                                                        \n"
"    \\newif\\ifGPblacktext                                                                \n"
"    \\GPblacktexttrue                                                                     \n"
"  }{}%%                                                                                   \n"
"  %% define a \\g@addto@macro without @ in the name:                                      \n"
"  \\let\\gplgaddtomacro\\g@addto@macro                                                    \n"
"  %% define empty templates for all commands taking text:                                 \n"
"  \\gdef\\gplbacktext{}%%                                                                 \n"
"  \\gdef\\gplfronttext{}%%                                                                \n"
"  \\makeatother                                                                           \n"
"  \\ifGPblacktext                                                                         \n"
"    %% no textcolor at all                                                                \n"
"    \\def\\colorrgb#1{}%%                                                                 \n"
"    \\def\\colorgray#1{}%%                                                                \n"
"  \\else                                                                                  \n"
"    %% gray or color?                                                                     \n"
"    \\ifGPcolor                                                                           \n"
"      \\def\\colorrgb#1{\\color[rgb]{#1}}%%                                               \n"
"      \\def\\colorgray#1{\\color[gray]{#1}}%%                                             \n"
"      \\expandafter\\def\\csname LTw\\endcsname{\\color{white}}%%                         \n"
"      \\expandafter\\def\\csname LTb\\endcsname{\\color{black}}%%                         \n"
"      \\expandafter\\def\\csname LTa\\endcsname{\\color{black}}%%                         \n"
"      \\expandafter\\def\\csname LT0\\endcsname{\\color[rgb]{1,0,0}}%%                    \n"
"      \\expandafter\\def\\csname LT1\\endcsname{\\color[rgb]{0,1,0}}%%                    \n"
"      \\expandafter\\def\\csname LT2\\endcsname{\\color[rgb]{0,0,1}}%%                    \n"
"      \\expandafter\\def\\csname LT3\\endcsname{\\color[rgb]{1,0,1}}%%                    \n"
"      \\expandafter\\def\\csname LT4\\endcsname{\\color[rgb]{0,1,1}}%%                    \n"
"      \\expandafter\\def\\csname LT5\\endcsname{\\color[rgb]{1,1,0}}%%                    \n"
"      \\expandafter\\def\\csname LT6\\endcsname{\\color[rgb]{0,0,0}}%%                    \n"
"      \\expandafter\\def\\csname LT7\\endcsname{\\color[rgb]{1,0.3,0}}%%                  \n"
"      \\expandafter\\def\\csname LT8\\endcsname{\\color[rgb]{0.5,0.5,0.5}}%%              \n"
"    \\else                                                                                \n"
"      %% gray                                                                             \n"
"      \\def\\colorrgb#1{\\color{black}}%%                                                 \n"
"      \\def\\colorgray#1{\\color[gray]{#1}}%%                                             \n"
"      \\expandafter\\def\\csname LTw\\endcsname{\\color{white}}%                          \n"
"      \\expandafter\\def\\csname LTb\\endcsname{\\color{black}}%                          \n"
"      \\expandafter\\def\\csname LTa\\endcsname{\\color{black}}%                          \n"
"      \\expandafter\\def\\csname LT0\\endcsname{\\color{black}}%                          \n"
"      \\expandafter\\def\\csname LT1\\endcsname{\\color{black}}%                          \n"
"      \\expandafter\\def\\csname LT2\\endcsname{\\color{black}}%                          \n"
"      \\expandafter\\def\\csname LT3\\endcsname{\\color{black}}%                          \n"
"      \\expandafter\\def\\csname LT4\\endcsname{\\color{black}}%                          \n"
"      \\expandafter\\def\\csname LT5\\endcsname{\\color{black}}%                          \n"
"      \\expandafter\\def\\csname LT6\\endcsname{\\color{black}}%                          \n"
"      \\expandafter\\def\\csname LT7\\endcsname{\\color{black}}%                          \n"
"      \\expandafter\\def\\csname LT8\\endcsname{\\color{black}}%                          \n"
"    \\fi                                                                                  \n"
"  \\fi                                                                                    \n"
"  \\setlength{\\unitlength}{1pt}%                                                         \n";

static char const postamble1[] =
"    }%%                                                                                    \n"
"    \\gplgaddtomacro\\gplfronttext{%                                                       \n"
"    }%%                                                                                    \n"
"    \\gplbacktext                                                                          \n";

static char const postamble2[] =
"    \\gplfronttext                                                                         \n"
"  \\end{picture}%                                                                          \n"
"\\endgroup                                                                                 \n";

void
PDFLaTeXRenderer::writePreamble()
{
    fprintf(_stream, "%s", preamble);
}
void
PDFLaTeXRenderer::writePostamble()
{
    fprintf(_stream, "%s", postamble1);

    // strip pathname on windows, as it is probably desired. It is not possible to work without paths on windows yet. (bug)
#ifdef WIN32
    gchar *figurefile = g_path_get_basename(_filename);
#else
    gchar *figurefile = g_strdup(_filename);
#endif
    fprintf(_stream, "      \\put(0,0){\\includegraphics{%s.pdf}}%%\n", figurefile);
    g_free(figurefile);

    fprintf(_stream, "%s", postamble2);
}

void
PDFLaTeXRenderer::sp_group_render(SPItem *item)
{
    SPGroup *group = SP_GROUP(item);

    GSList *l = g_slist_reverse(group->childList(false));
    while (l) {
        SPObject *o = SP_OBJECT (l->data);
        if (SP_IS_ITEM(o)) {
            renderItem (SP_ITEM (o));
        }
        l = g_slist_remove (l, o);
    }
}

void
PDFLaTeXRenderer::sp_use_render(SPItem *item)
{
/*
    bool translated = false;
    SPUse *use = SP_USE(item);

    if ((use->x._set && use->x.computed != 0) || (use->y._set && use->y.computed != 0)) {
        Geom::Matrix tp(Geom::Translate(use->x.computed, use->y.computed));
        ctx->pushState();
        ctx->transform(&tp);
        translated = true;
    }

    if (use->child && SP_IS_ITEM(use->child)) {
        renderItem(SP_ITEM(use->child));
    }

    if (translated) {
        ctx->popState();
    }
*/
}

void
PDFLaTeXRenderer::sp_text_render(SPItem *item)
{
    SPText *textobj = SP_TEXT (item);

    Geom::Matrix i2doc = sp_item_i2doc_affine(item);
    push_transform(i2doc);

    gchar *str = sp_te_get_string_multiline(item);
    Geom::Point pos = textobj->attributes.firstXY() * transform();
    gchar *alignment = "lb";

    // get rotation
    Geom::Matrix wotransl = i2doc.without_translation();
    double degrees = -180/M_PI * Geom::atan2(wotransl.xAxis());

    pop_transform();

    // write to LaTeX
    Inkscape::SVGOStringStream os;

//    os << "\\put(" << pos[Geom::X] << "," << pos[Geom::Y] << "){\\makebox(0,0)[" << alignment << "]{\\strut{}" << str << "}}%%\n";
    os << "\\put(" << pos[Geom::X] << "," << pos[Geom::Y] << "){";
    if (!Geom::are_near(degrees,0.)) {
        os << "\\rotatebox{" << degrees << "}{";
    }
    os <<   str;
    if (!Geom::are_near(degrees,0.)) {
        os << "}";
    }
    os << "}%%\n";

    fprintf(_stream, "%s", os.str().c_str());
}

void
PDFLaTeXRenderer::sp_flowtext_render(SPItem *item)
{
/*    SPFlowtext *group = SP_FLOWTEXT(item);

    // write to LaTeX
    Inkscape::SVGOStringStream os;

    os << "  \\begin{picture}(" << _width << "," << _height << ")%%\n";
    os << "    \\gplgaddtomacro\\gplbacktext{%%\n";
    os << "      \\csname LTb\\endcsname%%\n";
    os << "\\put(0,0){\\makebox(0,0)[lb]{\\strut{}Position}}%%\n";

    fprintf(_stream, "%s", os.str().c_str());
*/
}

void
PDFLaTeXRenderer::sp_root_render(SPItem *item)
{
    SPRoot *root = SP_ROOT(item);

//    ctx->pushState();
//    setStateForItem(ctx, item);
    Geom::Matrix tempmat (root->c2p);
    push_transform(tempmat);
    sp_group_render(item);
    pop_transform();
}

void
PDFLaTeXRenderer::sp_item_invoke_render(SPItem *item)
{
    // Check item's visibility
    if (item->isHidden()) {
        return;
    }

    g_message("hier?");
    if (SP_IS_ROOT(item)) {
        TRACE(("root\n"));
        return sp_root_render(item);
    } else if (SP_IS_GROUP(item)) {
        TRACE(("group\n"));
        return sp_group_render(item);
    } else if (SP_IS_USE(item)) {
        TRACE(("use begin---\n"));
        sp_use_render(item);
        TRACE(("---use end\n"));
    } else if (SP_IS_TEXT(item)) {
        TRACE(("text\n"));
        return sp_text_render(item);
    } else if (SP_IS_FLOWTEXT(item)) {
        TRACE(("flowtext\n"));
        return sp_flowtext_render(item);
    }
    // We are not interested in writing the other SPItem types to LaTeX
}

void
PDFLaTeXRenderer::setStateForItem(SPItem const *item)
{
/*
    SPStyle const *style = SP_OBJECT_STYLE(item);
    ctx->setStateForStyle(style);

    CairoRenderState *state = ctx->getCurrentState();
    state->clip_path = item->clip_ref->getObject();
    state->mask = item->mask_ref->getObject();
    state->item_transform = Geom::Matrix (item->transform);

    // If parent_has_userspace is true the parent state's transform
    // has to be used for the mask's/clippath's context.
    // This is so because we use the image's/(flow)text's transform for positioning
    // instead of explicitly specifying it and letting the renderer do the
    // transformation before rendering the item.
    if (SP_IS_TEXT(item) || SP_IS_FLOWTEXT(item) || SP_IS_IMAGE(item))
        state->parent_has_userspace = TRUE;
    TRACE(("setStateForItem opacity: %f\n", state->opacity));
*/
}

void
PDFLaTeXRenderer::renderItem(SPItem *item)
{
//    ctx->pushState();
//    setStateForItem(ctx, item);

//    CairoRenderState *state = ctx->getCurrentState();
//    state->need_layer = ( state->mask || state->clip_path || state->opacity != 1.0 );

    // Draw item on a temporary surface so a mask, clip path, or opacity can be applied to it.
//    if (state->need_layer) {
//        state->merge_opacity = FALSE;
//        ctx->pushLayer();
//    }
    Geom::Matrix tempmat (item->transform);
//    ctx->transform(&tempmat);
    sp_item_invoke_render(item);

//    if (state->need_layer)
//        ctx->popLayer();

//    ctx->popState();
}

bool
PDFLaTeXRenderer::setupDocument(SPDocument *doc, bool pageBoundingBox, SPItem *base)
{
// The boundingbox calculation here should be exactly the same as the one by CairoRenderer::setupDocument !

    if (!base)
        base = SP_ITEM(sp_document_root(doc));

    NRRect d;
    if (pageBoundingBox) {
        d.x0 = d.y0 = 0;
        d.x1 = ceil(sp_document_width(doc));
        d.y1 = ceil(sp_document_height(doc));
    } else {
        sp_item_invoke_bbox(base, &d, sp_item_i2d_affine(base), TRUE, SPItem::RENDERING_BBOX);
    }

    // convert from px to pt
    push_transform( Geom::Scale(PT_PER_PX, PT_PER_PX) );

    if (!pageBoundingBox)
    {
        double high = sp_document_height(doc);

        push_transform( Geom::Translate( -d.x0,
                                         -d.y0 ) );
    }

    // flip y-axis
    push_transform( Geom::Scale(1,-1) * Geom::Translate(0, sp_document_height(doc)) );

    _width = (d.x1-d.x0) * PT_PER_PX;
    _height = (d.y1-d.y0) * PT_PER_PX;

    // write the info to LaTeX
    Inkscape::SVGOStringStream os;

    os << "  \\begin{picture}(" << _width << "," << _height << ")%%\n";
    os << "    \\gplgaddtomacro\\gplbacktext{%%\n";
    os << "      \\csname LTb\\endcsname%%\n";

    fprintf(_stream, "%s", os.str().c_str());

    return true;
}

Geom::Matrix const &
PDFLaTeXRenderer::transform()
{
    return _transform_stack.top();
}

void
PDFLaTeXRenderer::push_transform(Geom::Matrix const &tr)
{
    if(_transform_stack.size()){
        Geom::Matrix tr_top = _transform_stack.top();
        _transform_stack.push(tr * tr_top);
    } else {
        _transform_stack.push(tr);
    }
}

void
PDFLaTeXRenderer::pop_transform()
{
    _transform_stack.pop();
}

/*
#include "macros.h" // SP_PRINT_*

// Apply an SVG clip path
void
PDFLaTeXRenderer::applyClipPath(CairoRenderContext *ctx, SPClipPath const *cp)
{
    g_assert( ctx != NULL && ctx->_is_valid );

    if (cp == NULL)
        return;

    CairoRenderContext::CairoRenderMode saved_mode = ctx->getRenderMode();
    ctx->setRenderMode(CairoRenderContext::RENDER_MODE_CLIP);

    Geom::Matrix saved_ctm;
    if (cp->clipPathUnits == SP_CONTENT_UNITS_OBJECTBOUNDINGBOX) {
        //SP_PRINT_DRECT("clipd", cp->display->bbox);
        NRRect clip_bbox(cp->display->bbox);
        Geom::Matrix t(Geom::Scale(clip_bbox.x1 - clip_bbox.x0, clip_bbox.y1 - clip_bbox.y0));
        t[4] = clip_bbox.x0;
        t[5] = clip_bbox.y0;
        t *= ctx->getCurrentState()->transform;
        ctx->getTransform(&saved_ctm);
        ctx->setTransform(&t);
    }

    TRACE(("BEGIN clip\n"));
    SPObject *co = SP_OBJECT(cp);
    for (SPObject *child = sp_object_first_child(co) ; child != NULL; child = SP_OBJECT_NEXT(child) ) {
        if (SP_IS_ITEM(child)) {
            SPItem *item = SP_ITEM(child);

            // combine transform of the item in clippath and the item using clippath:
            Geom::Matrix tempmat (item->transform);
            tempmat = tempmat * (ctx->getCurrentState()->item_transform);

            // render this item in clippath
            ctx->pushState();
            ctx->transform(&tempmat);
            setStateForItem(ctx, item);
            sp_item_invoke_render(item, ctx);
            ctx->popState();
        }
    }
    TRACE(("END clip\n"));

    // do clipping only if this was the first call to applyClipPath
    if (ctx->getClipMode() == CairoRenderContext::CLIP_MODE_PATH
        && saved_mode == CairoRenderContext::RENDER_MODE_NORMAL)
        cairo_clip(ctx->_cr);

    if (cp->clipPathUnits == SP_CONTENT_UNITS_OBJECTBOUNDINGBOX)
        ctx->setTransform(&saved_ctm);

    ctx->setRenderMode(saved_mode);
}

// Apply an SVG mask
void
PDFLaTeXRenderer::applyMask(CairoRenderContext *ctx, SPMask const *mask)
{
    g_assert( ctx != NULL && ctx->_is_valid );

    if (mask == NULL)
        return;

    //SP_PRINT_DRECT("maskd", &mask->display->bbox);
    NRRect mask_bbox(mask->display->bbox);
    // TODO: should the bbox be transformed if maskUnits != userSpaceOnUse ?
    if (mask->maskContentUnits == SP_CONTENT_UNITS_OBJECTBOUNDINGBOX) {
        Geom::Matrix t(Geom::Scale(mask_bbox.x1 - mask_bbox.x0, mask_bbox.y1 - mask_bbox.y0));
        t[4] = mask_bbox.x0;
        t[5] = mask_bbox.y0;
        t *= ctx->getCurrentState()->transform;
        ctx->setTransform(&t);
    }

    // Clip mask contents... but...
    // The mask's bounding box is the "geometric bounding box" which doesn't allow for
    // filters which extend outside the bounding box. So don't clip.
    // ctx->addClippingRect(mask_bbox.x0, mask_bbox.y0, mask_bbox.x1 - mask_bbox.x0, mask_bbox.y1 - mask_bbox.y0);

    ctx->pushState();

    TRACE(("BEGIN mask\n"));
    SPObject *co = SP_OBJECT(mask);
    for (SPObject *child = sp_object_first_child(co) ; child != NULL; child = SP_OBJECT_NEXT(child) ) {
        if (SP_IS_ITEM(child)) {
            SPItem *item = SP_ITEM(child);
            renderItem(ctx, item);
        }
    }
    TRACE(("END mask\n"));

    ctx->popState();
}
*/

}  /* namespace Internal */
}  /* namespace Extension */
}  /* namespace Inkscape */

#undef TRACE

/* End of GNU GPL code */

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
