/** @file
 * Multi path manipulator - a tool component that edits multiple paths at once
 */
/* Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2009 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_UI_TOOL_MULTI_PATH_MANIPULATOR_H
#define SEEN_UI_TOOL_MULTI_PATH_MANIPULATOR_H

#include <sigc++/connection.h>
#include "display/display-forward.h"
#include "forward.h"
#include "ui/tool/commit-events.h"
#include "ui/tool/manipulator.h"
#include "ui/tool/node-types.h"
#include "ui/tool/shape-record.h"

struct SPCanvasGroup;

namespace Inkscape {
namespace UI {

class PathManipulator;
class MultiPathManipulator;
struct PathSharedData;

/**
 * Manipulator that manages multiple path manipulators active at the same time.
 * It functions like a boost::ptr_set - manipulators added via insert() are retained.
 */
class MultiPathManipulator : public PointManipulator {
public:
    MultiPathManipulator(PathSharedData const &data, sigc::connection &chg);
    virtual ~MultiPathManipulator();
    virtual bool event(GdkEvent *event);

    bool empty() { return _mmap.empty(); }
    unsigned size() { return _mmap.empty(); }
    // TODO fix this garbage!
    void setItems(std::set<ShapeRecord> const &);
    //std::map<SPPath*, std::pair<Geom::Matrix, guint32> > const &items);
    void clear() { _mmap.clear(); }
    void cleanup();

    void selectSubpaths();
    void selectAll();
    void selectArea(Geom::Rect const &area, bool take);
    void shiftSelection(int dir);
    void linearGrow(int dir);
    void spatialGrow(int dir);
    void invertSelection();
    void invertSelectionInSubpaths();
    void deselect();

    void setNodeType(NodeType t);
    void setSegmentType(SegmentType t);

    void insertNodes();
    void joinNodes();
    void breakNodes();
    void deleteNodes(bool keep_shape = true);
    void joinSegment();
    void deleteSegments();
    void alignNodes(Geom::Dim2 d);
    void distributeNodes(Geom::Dim2 d);
    void reverseSubpaths();
    void move(Geom::Point const &delta);

    void showOutline(bool show);
    void showHandles(bool show);
    void showPathDirection(bool show);
    void updateOutlineColors();
    
    sigc::signal<void> signal_coords_changed;
private:
    typedef std::pair<ShapeRecord, boost::shared_ptr<PathManipulator> > MapPair;
    typedef std::map<ShapeRecord, boost::shared_ptr<PathManipulator> > MapType;

    template <typename R>
    void invokeForAll(R (PathManipulator::*method)()) {
        for (MapType::iterator i = _mmap.begin(); i != _mmap.end(); ++i) {
            ((i->second.get())->*method)();
        }
    }
    template <typename R, typename A>
    void invokeForAll(R (PathManipulator::*method)(A), A a) {
        for (MapType::iterator i = _mmap.begin(); i != _mmap.end(); ++i) {
            ((i->second.get())->*method)(a);
        }
    }
    template <typename R, typename A>
    void invokeForAll(R (PathManipulator::*method)(A const &), A const &a) {
        for (MapType::iterator i = _mmap.begin(); i != _mmap.end(); ++i) {
            ((i->second.get())->*method)(a);
        }
    }
    template <typename R, typename A, typename B>
    void invokeForAll(R (PathManipulator::*method)(A,B), A a, B b) {
        for (MapType::iterator i = _mmap.begin(); i != _mmap.end(); ++i) {
            ((i->second.get())->*method)(a, b);
        }
    }

    void _commit(CommitEvent cps);
    void _done(gchar const *);
    void _doneWithCleanup(gchar const *);
    guint32 _getOutlineColor(ShapeRole role);

    MapType _mmap;
    PathSharedData const &_path_data;
    sigc::connection &_changed;
    bool _show_handles;
    bool _show_outline;
    bool _show_path_direction;
};

} // namespace UI
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
