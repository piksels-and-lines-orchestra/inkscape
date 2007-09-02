/**
 * \brief A dockable dialog implementation.
 *
 * Author:
 *   Gustav Broberg <broberg@kth.se>
 *
 * Copyright (C) 2007 Authors
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "dock-behavior.h"
#include "inkscape.h"
#include "desktop.h"
#include "interface.h"
#include "widgets/icon.h"
#include "ui/widget/dock.h"
#include "verbs.h"
#include "dialog.h"
#include "prefs-utils.h"
#include "dialogs/dialog-events.h"

#include <gtkmm/invisible.h>
#include <gtkmm/label.h>
#include <gtkmm/stock.h>

#include <gtk/gtk.h>

namespace Inkscape {
namespace UI {
namespace Dialog {
namespace Behavior {


DockBehavior::DockBehavior(Dialog& dialog) :
    Behavior(dialog),
    _dock_item(*SP_ACTIVE_DESKTOP->getDock(),
               Inkscape::Verb::get(dialog._verb_num)->get_id(), dialog._title.c_str(),
               (Inkscape::Verb::get(dialog._verb_num)->get_image() ? 
                Inkscape::Verb::get(dialog._verb_num)->get_image() : ""),
               static_cast<Widget::DockItem::State>(
                   prefs_get_int_attribute (_dialog._prefs_path, "state", 
                                            UI::Widget::DockItem::DOCKED_STATE)))
{
    // Connect signals
    _signal_hide_connection = signal_hide().connect(sigc::mem_fun(*this, &Inkscape::UI::Dialog::Behavior::DockBehavior::_onHide));
    signal_response().connect(sigc::mem_fun(_dialog, &Inkscape::UI::Dialog::Dialog::on_response));
    _dock_item.signal_state_changed().connect(sigc::mem_fun(*this, &Inkscape::UI::Dialog::Behavior::DockBehavior::_onStateChanged));

    if (_dock_item.getState() == Widget::DockItem::FLOATING_STATE) {
        if (Gtk::Window *floating_win = _dock_item.getWindow())
            sp_transientize(GTK_WIDGET(floating_win->gobj()));
    }
}

DockBehavior::~DockBehavior()
{
}


Behavior *
DockBehavior::create(Dialog& dialog)
{
    return new DockBehavior(dialog);
}


DockBehavior::operator Gtk::Widget&()
{
    return _dock_item.getWidget();
}

GtkWidget *
DockBehavior::gobj()
{
    return _dock_item.gobj();
}

Gtk::VBox *
DockBehavior::get_vbox()
{
    return _dock_item.get_vbox();
}

void
DockBehavior::present() 
{
    bool was_attached = _dock_item.isAttached();

    _dock_item.present();

    if (!was_attached)
        _dialog.read_geometry();
}

void
DockBehavior::hide()
{
    _signal_hide_connection.block();
    _dock_item.hide();
    _signal_hide_connection.unblock();
}

void
DockBehavior::show() 
{ 
    _dock_item.show();
}

void 
DockBehavior::show_all_children()
{
    get_vbox()->show_all_children();
}

void 
DockBehavior::get_position(int& x, int& y)
{ 
    _dock_item.get_position(x, y);
}

void 
DockBehavior::get_size(int& width, int& height)
{ 
    _dock_item.get_size(width, height);
}

void
DockBehavior::resize(int width, int height) 
{
    _dock_item.resize(width, height);
}

void
DockBehavior::move(int x, int y)
{
    _dock_item.move(x, y);
}

void
DockBehavior::set_position(Gtk::WindowPosition position)
{
    _dock_item.set_position(position);
}

void
DockBehavior::set_size_request(int width, int height)
{
    _dock_item.set_size_request(width, height);
}

void 
DockBehavior::size_request(Gtk::Requisition& requisition)
{ 
    _dock_item.size_request(requisition);
}

void
DockBehavior::set_title(Glib::ustring title)
{
    _dock_item.set_title(title);
}

void
DockBehavior::set_response_sensitive(int response_id, bool setting)
{
    if (_response_map[response_id])
        _response_map[response_id]->set_sensitive(setting);
}

void
DockBehavior::set_sensitive(bool sensitive)
{
    get_vbox()->set_sensitive();
}

Gtk::Button * 
DockBehavior::add_button(const Glib::ustring& button_text, int response_id)
{
    Gtk::Button *button = new Gtk::Button(button_text);
    _addButton(button, response_id);
    return button;
}

Gtk::Button *
DockBehavior::add_button(const Gtk::StockID& stock_id, int response_id)
{
    Gtk::Button *button = new Gtk::Button(stock_id);
    _addButton(button, response_id);
    return button;
}

void
DockBehavior::_addButton(Gtk::Button *button, int response_id)
{
    _dock_item.addButton(button, response_id);

    if (response_id != 0) {

        /* Pass the signal_clicked signals onto a our own signal handler that can re-emit them as
         * signal_response signals
         */
        button->signal_clicked().connect( 
            sigc::bind<int>(sigc::mem_fun(*this, 
                            &Inkscape::UI::Dialog::Behavior::DockBehavior::_onResponse),
                            response_id));

        _response_map[response_id] = button;
    }
}

void
DockBehavior::set_default_response(int response_id)
{
    ResponseMap::iterator widget_found;
    widget_found = _response_map.find(response_id);

    if (widget_found != _response_map.end()) {
        widget_found->second->activate();
        widget_found->second->property_can_default() = true;
        widget_found->second->grab_default();
    }
}


void
DockBehavior::_onHide()
{
    _dialog.save_geometry();
    prefs_set_int_attribute (_dialog._prefs_path, "state", _dock_item.getPrevState());
}

void
DockBehavior::_onStateChanged(Widget::DockItem::State prev_state, 
                              Widget::DockItem::State new_state)
{
    prefs_set_int_attribute (_dialog._prefs_path, "state", new_state);

    if (new_state == Widget::DockItem::FLOATING_STATE) {
        if (Gtk::Window *floating_win = _dock_item.getWindow())
            sp_transientize(GTK_WIDGET(floating_win->gobj()));
    }
}

void
DockBehavior::_onResponse(int response_id)
{
    g_signal_emit_by_name (_dock_item.gobj(), "signal_response", response_id);
}

void
DockBehavior::onHideF12()
{
    _dialog.save_geometry();
    hide();
}

void
DockBehavior::onShowF12()
{
    present();
}

void
DockBehavior::onShutdown()
{
    prefs_set_int_attribute (_dialog._prefs_path, "state", _dock_item.getPrevState());
}

void
DockBehavior::onDesktopActivated(SPDesktop *desktop)
{
    gint transient_policy = prefs_get_int_attribute_limited ( "options.transientpolicy", "value", 1, 0, 2);

#ifdef WIN32 // FIXME: Temporary Win32 special code to enable transient dialogs
    if (prefs_get_int_attribute ( "options.dialogsontopwin32", "value", 0))
        transient_policy = 2;
    else    
        return;
#endif        

    if (!transient_policy) 
        return;

    Gtk::Window *floating_win = _dock_item.getWindow();

    if (floating_win) {

        if (_dialog.retransientize_suppress) {
            /* if retransientizing of this dialog is still forbidden after
             * previous call warning turned off because it was confusingly fired
             * when loading many files from command line
             */

            // g_warning("Retranzientize aborted! You're switching windows too fast!");
            return;
        }

        if (GtkWindow *dialog_win = floating_win->gobj()) {

            _dialog.retransientize_suppress = true; // disallow other attempts to retranzientize this dialog
            
            desktop->setWindowTransient (dialog_win);

            /*
             * This enables "aggressive" transientization,
             * i.e. dialogs always emerging on top when you switch documents. Note
             * however that this breaks "click to raise" policy of a window
             * manager because the switched-to document will be raised at once
             * (so that its transients also could raise)
             */
            if (transient_policy == 2 && ! _dialog._hiddenF12 && !_dialog._user_hidden) {
                // without this, a transient window not always emerges on top
                gtk_window_present (dialog_win);
            }
        }

        // we're done, allow next retransientizing not sooner than after 120 msec
        gtk_timeout_add (120, (GtkFunction) sp_retransientize_again, (gpointer) floating_win);
    }
}


/* Signal wrappers */

Glib::SignalProxy0<void> 
DockBehavior::signal_show() { return _dock_item.signal_show(); }

Glib::SignalProxy0<void> 
DockBehavior::signal_hide() { return _dock_item.signal_hide(); }

Glib::SignalProxy1<void, int> 
DockBehavior::signal_response() { return _dock_item.signal_response(); }

Glib::SignalProxy1<bool, GdkEventAny *> 
DockBehavior::signal_delete_event() { return _dock_item.signal_delete_event(); }

Glib::SignalProxy0<void>
DockBehavior::signal_drag_begin() { return _dock_item.signal_drag_begin(); }

Glib::SignalProxy1<void, bool>
DockBehavior::signal_drag_end() { return _dock_item.signal_drag_end(); }


} // namespace Behavior
} // namespace Dialog
} // namespace UI
} // namespace Inkscape

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
