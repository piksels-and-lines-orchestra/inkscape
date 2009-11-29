/** @file
 * @brief New node tool - implementation
 */
/* Authors:
 *   Krzysztof Kosiński <tweenk@gmail.com>
 *
 * Copyright (C) 2009 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib.h>
#include <glib/gi18n.h>
#include "desktop.h"
#include "desktop-handles.h"
#include "display/canvas-bpath.h"
#include "display/curve.h"
#include "display/sp-canvas.h"
#include "document.h"
#include "message-context.h"
#include "selection.h"
#include "shape-editor.h" // temporary!
#include "sp-clippath.h"
#include "sp-item-group.h"
#include "sp-mask.h"
#include "sp-object-group.h"
#include "sp-path.h"
#include "ui/tool/node-tool.h"
#include "ui/tool/control-point-selection.h"
#include "ui/tool/curve-drag-point.h"
#include "ui/tool/event-utils.h"
#include "ui/tool/manipulator.h"
#include "ui/tool/multi-path-manipulator.h"
#include "ui/tool/path-manipulator.h"
#include "ui/tool/selector.h"

#include "pixmaps/cursor-node.xpm"
#include "pixmaps/cursor-node-d.xpm"

namespace {
SPCanvasGroup *create_control_group(SPDesktop *d);
void ink_node_tool_class_init(InkNodeToolClass *klass);
void ink_node_tool_init(InkNodeTool *node_context);
void ink_node_tool_dispose(GObject *object);

void ink_node_tool_setup(SPEventContext *ec);
gint ink_node_tool_root_handler(SPEventContext *event_context, GdkEvent *event);
gint ink_node_tool_item_handler(SPEventContext *event_context, SPItem *item, GdkEvent *event);
void ink_node_tool_set(SPEventContext *ec, Inkscape::Preferences::Entry *value);

void ink_node_tool_update_tip(InkNodeTool *nt, GdkEvent *event);
void ink_node_tool_selection_changed(InkNodeTool *nt, Inkscape::Selection *sel);
void ink_node_tool_select_area(InkNodeTool *nt, Geom::Rect const &, GdkEventButton *);
void ink_node_tool_select_point(InkNodeTool *nt, Geom::Point const &, GdkEventButton *);
void ink_node_tool_mouseover_changed(InkNodeTool *nt, Inkscape::UI::ControlPoint *p);
} // anonymous namespace

GType ink_node_tool_get_type()
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(InkNodeToolClass),
            NULL, NULL,
            (GClassInitFunc) ink_node_tool_class_init,
            NULL, NULL,
            sizeof(InkNodeTool),
            4,
            (GInstanceInitFunc) ink_node_tool_init,
            NULL,    /* value_table */
        };
        type = g_type_register_static(SP_TYPE_EVENT_CONTEXT, "InkNodeTool", &info, (GTypeFlags)0);
    }
    return type;
}

namespace {

SPCanvasGroup *create_control_group(SPDesktop *d)
{
    return reinterpret_cast<SPCanvasGroup*>(sp_canvas_item_new(
        sp_desktop_controls(d), SP_TYPE_CANVAS_GROUP, NULL));
}

void destroy_group(SPCanvasGroup *g)
{
    gtk_object_destroy(GTK_OBJECT(g));
}

void ink_node_tool_class_init(InkNodeToolClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    SPEventContextClass *event_context_class = (SPEventContextClass *) klass;

    object_class->dispose = ink_node_tool_dispose;

    event_context_class->setup = ink_node_tool_setup;
    event_context_class->set = ink_node_tool_set;
    event_context_class->root_handler = ink_node_tool_root_handler;
    event_context_class->item_handler = ink_node_tool_item_handler;
}

void ink_node_tool_init(InkNodeTool *nt)
{
    SPEventContext *event_context = SP_EVENT_CONTEXT(nt);

    event_context->cursor_shape = cursor_node_xpm;
    event_context->hot_x = 1;
    event_context->hot_y = 1;

    new (&nt->_selection_changed_connection) sigc::connection();
    new (&nt->_mouseover_changed_connection) sigc::connection();
    //new (&nt->_mgroup) Inkscape::UI::ManipulatorGroup(nt->desktop);
    new (&nt->_selected_nodes) CSelPtr();
    new (&nt->_multipath) MultiPathPtr();
    new (&nt->_selector) SelectorPtr();
    new (&nt->_path_data) PathSharedDataPtr();
}

void ink_node_tool_dispose(GObject *object)
{
    InkNodeTool *nt = INK_NODE_TOOL(object);

    nt->enableGrDrag(false);

    nt->_selection_changed_connection.disconnect();
    nt->_mouseover_changed_connection.disconnect();
    nt->_multipath.~MultiPathPtr();
    nt->_selected_nodes.~CSelPtr();
    nt->_selector.~SelectorPtr();
    
    Inkscape::UI::PathSharedData &data = *nt->_path_data;
    destroy_group(data.node_data.node_group);
    destroy_group(data.node_data.handle_group);
    destroy_group(data.node_data.handle_line_group);
    destroy_group(data.outline_group);
    destroy_group(data.dragpoint_group);
    destroy_group(nt->_transform_handle_group);
    
    nt->_path_data.~PathSharedDataPtr();
    nt->_selection_changed_connection.~connection();
    nt->_mouseover_changed_connection.~connection();

    if (nt->_node_message_context) {
        delete nt->_node_message_context;
    }
    if (nt->shape_editor) {
        nt->shape_editor->unset_item(SH_KNOTHOLDER);
        delete nt->shape_editor;
    }

    G_OBJECT_CLASS(g_type_class_peek(g_type_parent(INK_TYPE_NODE_TOOL)))->dispose(object);
}

void ink_node_tool_setup(SPEventContext *ec)
{
    InkNodeTool *nt = INK_NODE_TOOL(ec);

    SPEventContextClass *parent = (SPEventContextClass *) g_type_class_peek(g_type_parent(INK_TYPE_NODE_TOOL));
    if (parent->setup) parent->setup(ec);

    nt->_node_message_context = new Inkscape::MessageContext((ec->desktop)->messageStack());

    nt->_path_data.reset(new Inkscape::UI::PathSharedData());
    Inkscape::UI::PathSharedData &data = *nt->_path_data;
    data.node_data.desktop = nt->desktop;

    // selector has to be created here, so that its hidden control point is on the bottom
    nt->_selector.reset(new Inkscape::UI::Selector(nt->desktop));

    // Prepare canvas groups for controls. This guarantees correct z-order, so that
    // for example a dragpoint won't obscure a node
    data.outline_group = create_control_group(nt->desktop);
    data.node_data.handle_line_group = create_control_group(nt->desktop);
    data.dragpoint_group = create_control_group(nt->desktop);
    nt->_transform_handle_group = create_control_group(nt->desktop);
    data.node_data.node_group = create_control_group(nt->desktop);
    data.node_data.handle_group = create_control_group(nt->desktop);

    Inkscape::Selection *selection = sp_desktop_selection (ec->desktop);
    nt->_selection_changed_connection.disconnect();
    nt->_selection_changed_connection =
        selection->connectChanged(
            sigc::bind<0>(
                sigc::ptr_fun(&ink_node_tool_selection_changed),
                nt));
    nt->_mouseover_changed_connection.disconnect();
    nt->_mouseover_changed_connection = 
        Inkscape::UI::ControlPoint::signal_mouseover_change.connect(
            sigc::bind<0>(
                sigc::ptr_fun(&ink_node_tool_mouseover_changed),
                nt));
    
    nt->_selected_nodes.reset(
        new Inkscape::UI::ControlPointSelection(nt->desktop, nt->_transform_handle_group));
    data.node_data.selection = nt->_selected_nodes.get();
    nt->_multipath.reset(new Inkscape::UI::MultiPathManipulator(data,
        nt->_selection_changed_connection));

    nt->_selector->signal_point.connect(
        sigc::bind<0>(
            sigc::ptr_fun(&ink_node_tool_select_point),
            nt));
    nt->_selector->signal_area.connect(
        sigc::bind<0>(
            sigc::ptr_fun(&ink_node_tool_select_area),
            nt));

    nt->_multipath->signal_coords_changed.connect(
        sigc::bind(
            sigc::mem_fun(*nt->desktop, &SPDesktop::emitToolSubselectionChanged),
            (void*) 0));
    nt->_selected_nodes->signal_point_changed.connect(
        sigc::hide( sigc::hide(
            sigc::bind(
                sigc::bind(
                    sigc::ptr_fun(ink_node_tool_update_tip),
                    (GdkEvent*)0),
                nt))));

    nt->cursor_drag = false;
    nt->show_transform_handles = true;
    nt->single_node_transform_handles = false;
    nt->flash_tempitem = NULL;
    nt->flashed_item = NULL;
    // TODO remove this!
    nt->shape_editor = new ShapeEditor(nt->desktop);

    // read prefs before adding items to selection to prevent momentarily showing the outline
    sp_event_context_read(nt, "show_handles");
    sp_event_context_read(nt, "show_outline");
    sp_event_context_read(nt, "show_path_direction");
    sp_event_context_read(nt, "show_transform_handles");
    sp_event_context_read(nt, "single_node_transform_handles");
    sp_event_context_read(nt, "edit_clipping_paths");
    sp_event_context_read(nt, "edit_masks");

    ink_node_tool_selection_changed(nt, selection);
    ink_node_tool_update_tip(nt, NULL);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getBool("/tools/nodes/selcue")) {
        ec->enableSelectionCue();
    }
    if (prefs->getBool("/tools/nodes/gradientdrag")) {
        ec->enableGrDrag();
    }

    nt->desktop->emitToolSubselectionChanged(NULL); // sets the coord entry fields to inactive
}

void ink_node_tool_set(SPEventContext *ec, Inkscape::Preferences::Entry *value)
{
    InkNodeTool *nt = INK_NODE_TOOL(ec);
    Glib::ustring entry_name = value->getEntryName();

    if (entry_name == "show_handles") {
        nt->_multipath->showHandles(value->getBool(true));
    } else if (entry_name == "show_outline") {
        nt->show_outline = value->getBool();
        nt->_multipath->showOutline(nt->show_outline);
    } else if (entry_name == "show_path_direction") {
        nt->show_path_direction = value->getBool();
        nt->_multipath->showPathDirection(nt->show_path_direction);
    } else if (entry_name == "show_transform_handles") {
        nt->show_transform_handles = value->getBool(true);
        nt->_selected_nodes->showTransformHandles(
            nt->show_transform_handles, nt->single_node_transform_handles);
    } else if (entry_name == "single_node_transform_handles") {
        nt->single_node_transform_handles = value->getBool();
        nt->_selected_nodes->showTransformHandles(
            nt->show_transform_handles, nt->single_node_transform_handles);
    } else if (entry_name == "edit_clipping_paths") {
        nt->edit_clipping_paths = value->getBool();
        ink_node_tool_selection_changed(nt, nt->desktop->selection);
    } else if (entry_name == "edit_masks") {
        nt->edit_masks = value->getBool();
        ink_node_tool_selection_changed(nt, nt->desktop->selection);
    } else {
        SPEventContextClass *parent_class =
            (SPEventContextClass *) g_type_class_peek(g_type_parent(INK_TYPE_NODE_TOOL));
        if (parent_class->set)
            parent_class->set(ec, value);
    }
}

void store_clip_mask_items(SPItem *clipped, SPObject *obj, std::map<SPItem*,
    std::pair<Geom::Matrix, guint32> > &s, Geom::Matrix const &postm, guint32 color)
{
    if (!obj) return;
    if (SP_IS_GROUP(obj) || SP_IS_OBJECTGROUP(obj)) {
        //TODO is checking for obj->children != NULL above better?
        for (SPObject *c = obj->children; c; c = c->next) {
            store_clip_mask_items(clipped, c, s, postm, color);
        }
    } else if (SP_IS_ITEM(obj)) {
        s.insert(std::make_pair(SP_ITEM(obj),
            std::make_pair(sp_item_i2d_affine(clipped) * postm, color)));
    }
}

struct IsPath {
    bool operator()(SPItem *i) const { return SP_IS_PATH(i); }
};

void ink_node_tool_selection_changed(InkNodeTool *nt, Inkscape::Selection *sel)
{
    using namespace Inkscape::UI;
    // TODO this is ugly!!!
    typedef std::map<SPItem*, std::pair<Geom::Matrix, guint32> > TransMap;
    typedef std::map<SPPath*, std::pair<Geom::Matrix, guint32> > PathMap;
    GSList const *ilist = sel->itemList();
    TransMap items;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    for (GSList *i = const_cast<GSList*>(ilist); i; i = i->next) {
        SPObject *obj = static_cast<SPObject*>(i->data);
        if (SP_IS_ITEM(obj)) {
            items.insert(std::make_pair(SP_ITEM(obj),
                std::make_pair(Geom::identity(),
                prefs->getColor("/tools/nodes/outline_color", 0xff0000ff))));
            if (nt->edit_clipping_paths && SP_ITEM(i->data)->clip_ref) {
                store_clip_mask_items(SP_ITEM(i->data),
                    SP_OBJECT(SP_ITEM(i->data)->clip_ref->getObject()), items,
                    nt->desktop->dt2doc(),
                    prefs->getColor("/tools/nodes/clipping_path_color", 0x00ff00ff));
            }
            if (nt->edit_masks && SP_ITEM(i->data)->mask_ref) {
                store_clip_mask_items(SP_ITEM(i->data),
                    SP_OBJECT(SP_ITEM(i->data)->mask_ref->getObject()), items,
                    nt->desktop->dt2doc(),
                    prefs->getColor("/tools/nodes/mask_color", 0x0000ffff));
            }
        }
    }

    // ugly hack: set the first editable non-path item for knotholder
    // maybe use multiple ShapeEditors for now, to allow editing many shapes at once?
    bool something_set = false;
    for (TransMap::iterator i = items.begin(); i != items.end(); ++i) {
        SPItem *obj = i->first;
        if (SP_IS_SHAPE(obj) && !SP_IS_PATH(obj)) {
            nt->shape_editor->set_item(obj, SH_KNOTHOLDER);
            something_set = true;
            break;
        }
    }
    if (!something_set) {
        nt->shape_editor->unset_item(SH_KNOTHOLDER);
    }
    
    PathMap p;
    for (TransMap::iterator i = items.begin(); i != items.end(); ++i) {
        if (SP_IS_PATH(i->first)) {
            p.insert(std::make_pair(SP_PATH(i->first),
                std::make_pair(i->second.first, i->second.second)));
        }
    }

    nt->_multipath->setItems(p);
    ink_node_tool_update_tip(nt, NULL);
    nt->desktop->updateNow();
}

gint ink_node_tool_root_handler(SPEventContext *event_context, GdkEvent *event)
{
    /* things to handle here:
     * 1. selection of items
     * 2. passing events to manipulators
     * 3. some keybindings
     */
    using namespace Inkscape::UI; // pull in event helpers
    
    SPDesktop *desktop = event_context->desktop;
    Inkscape::Selection *selection = desktop->selection;
    InkNodeTool *nt = static_cast<InkNodeTool*>(event_context);
    static Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    
    if (nt->_multipath->event(event)) return true;
    if (nt->_selector->event(event)) return true;
    if (nt->_selected_nodes->event(event)) return true;

    switch (event->type)
    {
    case GDK_MOTION_NOTIFY:
        // create outline
        if (prefs->getBool("/tools/nodes/pathflash_enabled")) {
            if (prefs->getBool("/tools/nodes/pathflash_unselected") && !nt->_multipath->empty())
                break;

            SPItem *over_item = sp_event_context_find_item (desktop, event_point(event->button),
                FALSE, TRUE);
            if (over_item == nt->flashed_item) break;
            if (nt->flash_tempitem) {
                desktop->remove_temporary_canvasitem(nt->flash_tempitem);
                nt->flash_tempitem = NULL;
                nt->flashed_item = NULL;
            }
            if (!SP_IS_PATH(over_item)) break; // for now, handle only paths

            nt->flashed_item = over_item;
            SPCurve *c = sp_path_get_curve_for_edit(SP_PATH(over_item));
            c->transform(sp_item_i2d_affine(over_item));
            SPCanvasItem *flash = sp_canvas_bpath_new(sp_desktop_tempgroup(desktop), c);
            sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(flash),
                prefs->getInt("/tools/nodes/highlight_color", 0xff0000ff), 1.0,
                SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
            sp_canvas_bpath_set_fill(SP_CANVAS_BPATH(flash), 0, SP_WIND_RULE_NONZERO);
            nt->flash_tempitem = desktop->add_temporary_canvasitem(flash,
                prefs->getInt("/tools/nodes/pathflash_timeout", 500));
            c->unref();
        }
        return true;
    case GDK_KEY_PRESS:
        switch (get_group0_keyval(&event->key))
        {
        case GDK_Escape: // deselect everything
            if (nt->_selected_nodes->empty()) {
                selection->clear();
            } else {
                nt->_selected_nodes->clear();
            }
            ink_node_tool_update_tip(nt, event);
            return TRUE;
        case GDK_a:
            if (held_control(event->key)) {
                if (held_alt(event->key)) {
                    nt->_multipath->selectAll();
                } else {
                    // select all nodes in subpaths that have something selected
                    // if nothing is selected, select everything
                    nt->_multipath->selectSubpaths();
                }
                ink_node_tool_update_tip(nt, event);
                return TRUE;
            }
            break;
        default:
            break;
        }
        ink_node_tool_update_tip(nt, event);
        break;
    case GDK_KEY_RELEASE:
        ink_node_tool_update_tip(nt, event);
        break;
    default: break;
    }
    
    SPEventContextClass *parent_class = (SPEventContextClass *) g_type_class_peek(g_type_parent(INK_TYPE_NODE_TOOL));
    if (parent_class->root_handler)
        return parent_class->root_handler(event_context, event);
    return FALSE;
}

void ink_node_tool_update_tip(InkNodeTool *nt, GdkEvent *event)
{
    using namespace Inkscape::UI;
    if (event && (event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE)) {
        unsigned new_state = state_after_event(event);
        if (new_state == event->key.state) return;
        if (state_held_shift(new_state)) {
            nt->_node_message_context->set(Inkscape::NORMAL_MESSAGE,
                C_("Node tool tip", "<b>Shift:</b> drag to add nodes to the selection, "
                "click to toggle object selection"));
            return;
        }
    }
    unsigned sz = nt->_selected_nodes->size();
    if (sz != 0) {
        char *dyntip = g_strdup_printf(C_("Node tool tip",
            "Selected <b>%d nodes</b>. Drag to select nodes, click to select a single object "
            "or unselect all objects"), sz);
        nt->_node_message_context->set(Inkscape::NORMAL_MESSAGE, dyntip);
        g_free(dyntip);
    } else if (nt->_multipath->empty()) {
        nt->_node_message_context->set(Inkscape::NORMAL_MESSAGE,
            C_("Node tool tip", "Drag or click to select objects to edit"));
    } else {
        nt->_node_message_context->set(Inkscape::NORMAL_MESSAGE,
            C_("Node tool tip", "Drag to select nodes, click to select an object "
            "or clear the selection"));
    }
}

gint ink_node_tool_item_handler(SPEventContext *event_context, SPItem *item, GdkEvent *event)
{
    SPEventContextClass *parent_class =
        (SPEventContextClass *) g_type_class_peek(g_type_parent(INK_TYPE_NODE_TOOL));
    if (parent_class->item_handler)
        return parent_class->item_handler(event_context, item, event);
    return FALSE;
}

void ink_node_tool_select_area(InkNodeTool *nt, Geom::Rect const &sel, GdkEventButton *event)
{
    using namespace Inkscape::UI;
    if (nt->_multipath->empty()) {
        // if multipath is empty, select rubberbanded items rather than nodes
        Inkscape::Selection *selection = nt->desktop->selection;
        GSList *items = sp_document_items_in_box(
            sp_desktop_document(nt->desktop), nt->desktop->dkey, sel);
        selection->setList(items);
        g_slist_free(items);
    } else {
        nt->_multipath->selectArea(sel, !held_shift(*event));
    }
}
void ink_node_tool_select_point(InkNodeTool *nt, Geom::Point const &sel, GdkEventButton *event)
{
    using namespace Inkscape::UI; // pull in event helpers
    if (!event) return;
    if (event->button != 1) return;

    Inkscape::Selection *selection = nt->desktop->selection;

    SPItem *item_clicked = sp_event_context_find_item (nt->desktop, event_point(*event),
                    (event->state & GDK_MOD1_MASK) && !(event->state & GDK_CONTROL_MASK), TRUE);

    if (item_clicked == NULL) { // nothing under cursor
        // if no Shift, deselect
        if (!(event->state & GDK_SHIFT_MASK)) {
            selection->clear();
        }
        return;
    }
    if (held_shift(*event)) {
        selection->toggle(item_clicked);
    } else {
        selection->set(item_clicked);
    }
    nt->desktop->updateNow();
}

void ink_node_tool_mouseover_changed(InkNodeTool *nt, Inkscape::UI::ControlPoint *p)
{
    using Inkscape::UI::CurveDragPoint;
    CurveDragPoint *cdp = dynamic_cast<CurveDragPoint*>(p);
    if (cdp && !nt->cursor_drag) {
        nt->cursor_shape = cursor_node_d_xpm;
        nt->hot_x = 1;
        nt->hot_y = 1;
        sp_event_context_update_cursor(nt);
        nt->cursor_drag = true;
    } else if (!cdp && nt->cursor_drag) {
        nt->cursor_shape = cursor_node_xpm;
        nt->hot_x = 1;
        nt->hot_y = 1;
        sp_event_context_update_cursor(nt);
        nt->cursor_drag = false;
    }
}

} // anonymous namespace

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
