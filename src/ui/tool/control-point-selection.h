/** @file
 * Node selection - stores a set of nodes and applies transformations
 * to them
 */
/* Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2009 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_UI_TOOL_NODE_SELECTION_H
#define SEEN_UI_TOOL_NODE_SELECTION_H

#include <memory>
#include <tr1/unordered_map>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/optional.hpp>
#include <sigc++/sigc++.h>
#include <2geom/forward.h>
#include <2geom/point.h>
#include "display/display-forward.h"
#include "util/accumulators.h"
#include "util/hash.h"
#include "ui/tool/commit-events.h"
#include "ui/tool/manipulator.h"

namespace std { using namespace tr1; }

class SPDesktop;

namespace Inkscape {
namespace UI {

class TransformHandleSet;
class SelectableControlPoint;

class ControlPointSelection : public Manipulator {
public:
    ControlPointSelection(SPDesktop *d, SPCanvasGroup *th_group);
    ~ControlPointSelection();
    typedef std::list<sigc::connection> connlist_type;
    typedef std::unordered_map< SelectableControlPoint *,
        boost::shared_ptr<connlist_type> > map_type;

    // boilerplate typedefs
    typedef map_type::iterator iterator;
    typedef map_type::const_iterator const_iterator;
    typedef map_type::size_type size_type;

    typedef SelectableControlPoint *value_type;
    typedef SelectableControlPoint *key_type;

    // size
    bool empty() { return _points.empty(); }
    size_type size() { return _points.size(); }

    // iterators
    iterator begin() { return _points.begin(); }
    const_iterator begin() const { return _points.begin(); }
    iterator end() { return _points.end(); }
    const_iterator end() const { return _points.end(); }

    // insert
    std::pair<iterator, bool> insert(const value_type& x);
    template <class InputIterator>
    void insert(InputIterator first, InputIterator last) {
        for (; first != last; ++first) {
            insert(*first);
        }
    }

    // erase
    void clear();
    void erase(iterator pos);
    size_type erase(const key_type& k);
    void erase(iterator first, iterator last);

    // find
    iterator find(const key_type &k) { return _points.find(k); }

    virtual bool event(GdkEvent *);

    void transform(Geom::Matrix const &m);
    void align(Geom::Dim2 d);
    void distribute(Geom::Dim2 d);

    Geom::OptRect pointwiseBounds();
    Geom::OptRect bounds();

    void showTransformHandles(bool v, bool one_node);
    // the two methods below do not modify the state; they are for use in manipulators
    // that need to temporarily hide the handles
    void hideTransformHandles();
    void restoreTransformHandles();
    
    // TODO this is really only applicable to nodes... maybe derive a NodeSelection?
    void setSculpting(bool v) { _sculpt_enabled = v; }

    sigc::signal<void> signal_update;
    sigc::signal<void, SelectableControlPoint *, bool> signal_point_changed;
    sigc::signal<void, CommitEvent> signal_commit;
private:
    void _selectionGrabbed(SelectableControlPoint *, GdkEventMotion *);
    void _selectionDragged(Geom::Point const &, Geom::Point &, GdkEventMotion *);
    void _selectionUngrabbed();
    void _updateTransformHandles(bool preserve_center);
    bool _keyboardMove(GdkEventKey const &, Geom::Point const &);
    bool _keyboardRotate(GdkEventKey const &, int);
    bool _keyboardScale(GdkEventKey const &, int);
    bool _keyboardFlip(Geom::Dim2);
    void _keyboardTransform(Geom::Matrix const &);
    void _commitTransform(CommitEvent ce);
    map_type _points;
    boost::optional<double> _rot_radius;
    TransformHandleSet *_handles;
    SelectableControlPoint *_grabbed_point;
    unsigned _dragging         : 1;
    unsigned _handles_visible  : 1;
    unsigned _one_node_handles : 1;
    unsigned _sculpt_enabled   : 1;
    unsigned _sculpting        : 1;
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
