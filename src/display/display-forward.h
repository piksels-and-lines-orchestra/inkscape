#ifndef SEEN_DISPLAY_DISPLAY_FORWARD_H
#define SEEN_DISPLAY_DISPLAY_FORWARD_H

#include <glib-object.h>

struct SPCanvas;
struct SPCanvasClass;
struct SPCanvasItem;
typedef struct _SPCanvasItemClass SPCanvasItemClass;
struct SPCanvasGroup;
struct SPCanvasGroupClass;
class SPCurve;

class NRArena;

namespace Inkscape {
class DrawingContext;
class DrawingSurface;
class DrawingItem;
class DrawingGroup;
class DrawingImage;
class DrawingShape;
class DrawingGlyphs;
class DrawingText;

namespace Display {
    class TemporaryItem;
    class TemporaryItemList;
}
}

#endif /* !SEEN_DISPLAY_DISPLAY_FORWARD_H */

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
