/**
 * @file
 * Widget that listens and modifies repr attributes.
 */
/* Authors:
 *  Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 authors
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Licensed under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_DIALOGS_SP_ATTRIBUTE_WIDGET_H
#define SEEN_DIALOGS_SP_ATTRIBUTE_WIDGET_H

#include <gtkmm/entry.h>
#include <glib.h>
#include <stddef.h>
#include <sigc++/connection.h>

#define SP_TYPE_ATTRIBUTE_TABLE (sp_attribute_table_get_type ())
#define SP_ATTRIBUTE_TABLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_ATTRIBUTE_TABLE, SPAttributeTable))
#define SP_ATTRIBUTE_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_ATTRIBUTE_TABLE, SPAttributeTableClass))
#define SP_IS_ATTRIBUTE_TABLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_ATTRIBUTE_TABLE))
#define SP_IS_ATTRIBUTE_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_ATTRIBUTE_TABLE))

namespace Inkscape {
namespace XML {
class Node;
}
}

struct SPAttributeTable;
struct SPAttributeTableClass;

class SPObject;

#include <gtk/gtk.h>

class SPAttributeWidget : Gtk::Entry {
//NOTE: SPAttributeWidget does not seem to be used nowhere in Inkscape, conversion to c++ not tested
public:
    SPAttributeWidget ();
    ~SPAttributeWidget ();
    void set_object(SPObject *object, const gchar *attribute);
    void set_repr(Inkscape::XML::Node *repr, const gchar *attribute);
    Glib::ustring get_attribute(void) {return _attribute;};
    void set_blocked(guint b) {blocked = b;};
    
    union {
        SPObject *object;
        Inkscape::XML::Node *repr;
    } src;

protected:
    void on_changed (void);
    
private:
    guint blocked;
    guint hasobj;
    Glib::ustring _attribute;
    sigc::connection modified_connection;
    sigc::connection release_connection;
};


/* SPAttributeTable */

struct SPAttributeTable {
    GtkVBox vbox;
    guint blocked : 1;
    guint hasobj : 1;
    GtkWidget *table;
    union {
        SPObject *object;
        Inkscape::XML::Node *repr;
    } src;
    gint num_attr;
    gchar **attributes;
    GtkWidget **entries;

    sigc::connection modified_connection;
    sigc::connection release_connection;
};

struct SPAttributeTableClass {
    GtkEntryClass entry_class;
};

GType sp_attribute_table_get_type (void);

GtkWidget *sp_attribute_table_new ( SPObject *object, gint num_attr, 
                                    const gchar **labels, 
                                    const gchar **attributes );
GtkWidget *sp_attribute_table_new_repr ( Inkscape::XML::Node *repr, gint num_attr, 
                                         const gchar **labels, 
                                         const gchar **attributes );
void sp_attribute_table_set_object ( SPAttributeTable *spw, 
                                     SPObject *object, gint num_attr, 
                                     const gchar **labels, 
                                     const gchar **attrs );
void sp_attribute_table_set_repr ( SPAttributeTable *spw, 
                                   Inkscape::XML::Node *repr, gint num_attr, 
                                   const gchar **labels, 
                                   const gchar **attrs );

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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
