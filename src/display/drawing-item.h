/**
 * @file
 * @brief Canvas item belonging to an SVG drawing element
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_INKSCAPE_DISPLAY_DRAWING_ITEM_H
#define SEEN_INKSCAPE_DISPLAY_DRAWING_ITEM_H

#include <exception>
#include <boost/intrusive/list.hpp>
#include <2geom/rect.h>
#include <2geom/affine.h>

class NRArena;
class SPStyle;
void nr_arena_set_cache_limit(NRArena *, Geom::OptIntRect const &);

namespace Inkscape {

typedef ::NRArena Drawing;
class DrawingContext;
class DrawingCache;
class DrawingItem;
namespace Filters { class Filter; }

struct UpdateContext {
    Geom::Affine ctm;
};

class InvalidItemException : public std::exception {
    virtual const char *what() const throw() {
        return "Invalid item in drawing";
    }
};

typedef boost::intrusive::list_base_hook<> ChildrenListHook;

class DrawingItem
    : public ChildrenListHook
{
public:
    enum RenderFlags {
        RENDER_DEFAULT = 0,
        RENDER_CACHE_ONLY = 1,
        RENDER_BYPASS_CACHE = 2
    };
    enum StateFlags {
        STATE_NONE = 0,
        STATE_BBOX = (1<<0),    // geometric bounding box is up-to-date
        STATE_DRAWBOX = (1<<1), // visual bounding box is up-to-date
        STATE_CACHE = (1<<2),   // cache extents and clean area are up-to-date
        STATE_PICK = (1<<3),    // can process pick requests
        STATE_RENDER = (1<<4),  // can be rendered
        STATE_ALL = (1<<5)-1
    };
    typedef boost::intrusive::list<DrawingItem> ChildrenList;

    DrawingItem(Drawing *drawing);
    virtual ~DrawingItem();

    Geom::OptIntRect geometricBounds() const { return _bbox; }
    Geom::OptIntRect visualBounds() const { return _drawbox; }
    Geom::OptRect itemBounds() const { return _item_bbox; }
    Geom::Affine ctm() const { return _ctm; }
    Geom::Affine transform() const { return _transform ? *_transform : Geom::identity(); }
    Drawing *drawing() const { return _drawing; }
    DrawingItem *parent() const;

    void appendChild(DrawingItem *item);
    void prependChild(DrawingItem *item);
    void clearChildren();

    bool visible() const { return _visible; }
    void setVisible(bool v);
    bool sensitive() const { return _sensitive; }
    void setSensitive(bool v);
    bool cached() const { return _cached; }
    void setCached(bool c);

    void setOpacity(float opacity);
    void setTransform(Geom::Affine const &trans);
    void setClip(DrawingItem *item);
    void setMask(DrawingItem *item);
    void setZOrder(unsigned z);
    void setItemBounds(Geom::OptRect const &bounds);

    void setKey(unsigned key) { _key = key; }
    unsigned key() const { return _key; }
    void setData(void *data) { _user_data = data; }
    void *data() const { return _user_data; }

    void update(Geom::IntRect const &area = Geom::IntRect::infinite(), UpdateContext const &ctx = UpdateContext(), unsigned flags = STATE_ALL, unsigned reset = 0);
    void render(DrawingContext &ct, Geom::IntRect const &area, unsigned flags = 0);
    void clip(DrawingContext &ct, Geom::IntRect const &area);
    DrawingItem *pick(Geom::Point const &p, double delta, bool sticky);

protected:
    void _renderOutline(DrawingContext &ct, Geom::IntRect const &area, unsigned flags);
    void _markForUpdate(unsigned state, bool propagate);
    void _markForRendering();
    void _setStyleCommon(SPStyle *&_style, SPStyle *style);
    virtual unsigned _updateItem(Geom::IntRect const &area, UpdateContext const &ctx,
                                 unsigned flags, unsigned reset) { return 0; }
    virtual void _renderItem(DrawingContext &ct, Geom::IntRect const &area, unsigned flags) {}
    virtual void _clipItem(DrawingContext &ct, Geom::IntRect const &area) {}
    virtual DrawingItem *_pickItem(Geom::Point const &p, double delta, bool sticky = false) { return NULL; }
    virtual bool _canClip() { return false; }

    Drawing *_drawing;
    DrawingItem *_parent;
    ChildrenList _children;

    unsigned _key; ///< Some SPItems can have more than one NRArenaItem;
                   ///  this value is a hack used to distinguish between them
    float _opacity;

    Geom::Affine *_transform; ///< Incremental transform from parent to this item's coords
    Geom::Affine _ctm; ///< Total transform from item coords to display coords
    Geom::OptIntRect _bbox; ///< Bounding box in display (pixel) coords
    Geom::OptIntRect _drawbox; ///< Bounding box enlarged by filters, shrinked by clips and masks
    Geom::OptRect _item_bbox; ///< Bounding box in item coordinates

    DrawingItem *_clip;
    DrawingItem *_mask;
    Inkscape::Filters::Filter *_filter;
    void *_user_data; ///< Used to associate DrawingItems with SPItems that created them
    DrawingCache *_cache;

    unsigned _state : 8;
    unsigned _visible : 1;
    unsigned _sensitive : 1; ///< Whether this item responds to events
    unsigned _cached : 1; ///< Whether the rendering is stored for reuse
    unsigned _propagate : 1; ///< Whether to call update for all children on next update
    //unsigned _renders_opacity : 1; ///< Whether object needs temporary surface for opacity
    unsigned _clip_child : 1; ///< If set, this is not a child of _parent, but a clipping path
    unsigned _mask_child : 1; ///< If set, this is not a child of _parent, but a mask
    unsigned _pick_children : 1; ///< For groups: if true, children are returned from pick(),
                                 ///  otherwise the group is returned

    // temporary hacks until I rewrite NRArena to Inkscape::Drawing
    friend class NRArena;
    friend void ::nr_arena_set_cache_limit(NRArena *, Geom::OptIntRect const &);
};

struct DeleteDisposer {
    void operator()(DrawingItem *item) { delete item; }
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
