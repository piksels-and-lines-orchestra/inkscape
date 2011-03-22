/** @file
 * @brief Open Clip Art Library integration dialogs
 */
/* Authors:
 *   Bruno Dilly <bruno.dilly@gmail.com>
 *   Inkscape Guys
 *   Andrew Higginson
 *
 * Copyright (C) 2007 Bruno Dilly <bruno.dilly@gmail.com>
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
 
#ifndef __OCAL_DIALOG_H__
#define __OCAL_DIALOG_H__

#include <glibmm.h>
#include <vector>
#include <gtkmm.h>
#include "filedialogimpl-gtkmm.h"

//General includes
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <set>
#include <libxml/parser.h>
#include <libxml/tree.h>


//Gtk includes
#include <glibmm/i18n.h>
#include <glib/gstdio.h>

//Temporary ugly hack
//Remove this after the get_filter() calls in
//show() on both classes are fixed
#include <gtk/gtkfilechooser.h>

//Another hack
#include <gtk/gtkentry.h>
#include <gtk/gtkexpander.h>
#ifdef WITH_GNOME_VFS
#include <libgnomevfs/gnome-vfs-init.h>  // gnome_vfs_initialized
#include<libgnomevfs/gnome-vfs.h>
#endif

//Inkscape includes
#include <extension/input.h>
#include <extension/output.h>
#include <extension/db.h>
#include "inkscape.h"
#include "svg-view-widget.h"
#include "gc-core.h"

//For export dialog
#include "ui/widget/scalar-unit.h"

#include <dialogs/dialog-events.h>

namespace Inkscape
{
namespace UI 
{   
namespace Dialog
{   
namespace OCAL
{
/*#########################################################################
### F I L E     D I A L O G    O C A L    B A S E    C L A S S
#########################################################################*/

/**
 * This class is the base implementation for export to OCAL.
 */
class FileDialogBase : public Gtk::Window
{
public:

    /**
     * Constructor
     */
    FileDialogBase(const Glib::ustring &title, Gtk::Window& parent) : Gtk::Window(Gtk::WINDOW_TOPLEVEL)
    {
        set_title(title);

        //set_modal(TRUE);
        //sp_transientize((GtkWidget*) gobj());
    }

    /*
     * Destructor
     */
    virtual ~FileDialogBase()
    {}

protected:
    void cleanup( bool showConfirmed );

    /**
     * What type of 'open' are we? (open, import, place, etc)
     */
    FileDialogType dialogType;
};




//########################################################################
//# F I L E    E X P O R T   T O   O C A L
//########################################################################


/**
 * Our implementation of the ExportDialog interface.
 */
/*
class ExportDialog : public FileDialogBase
{

public:
*/
    /**
     * Constructor
     * @param fileTypes one of FileDialogTypes
     * @param title the title of the dialog
     * @param key a list of file types from which the user can select
     */
/*
    ExportDialog(Gtk::Window& parentWindow, 
                             FileDialogType fileTypes,
                 const Glib::ustring &title);
*/
    /**
     * Destructor.
     * Perform any necessary cleanups.
     */
/*
    ~ExportDialog();
*/
    /**
     * Show an SaveAs file selector.
     * @return the selected path if user selected one, else NULL
     */
/*
    bool show();

    Glib::ustring getFilename();

    Glib::ustring myFilename;
*/
    /**
     * Change the window title.
     */
/*
    void change_title(const Glib::ustring& title);
    
private:
*/
    /**
     * Fix to allow the user to type the file name
     */
/*
    Gtk::Entry *fileNameEntry;
*/
    /**
     *  Data mirror of the combo box
     */
/*
    std::vector<FileType> fileTypes;

    // Child widgets
    Gtk::HBox childBox;
    Gtk::VBox checksBox;
    Gtk::HBox fileBox;
*/
    /**
     * The extension to use to write this file
     */
/*
    Inkscape::Extension::Extension *extension;
*/
    /**
     * Callback for user input into fileNameEntry
     */
/*
    void fileNameEntryChangedCallback();
*/
    /**
     * List of known file extensions.
     */
/*
    std::set<Glib::ustring> knownExtensions;

}; //ExportDialog
*/

//########################################################################
//# F I L E    E X P O R T   T O   O C A L   P A S S W O R D
//########################################################################


/**
 * Our implementation of the ExportPasswordDialog interface.
 */
/*
class ExportPasswordDialog : public FileDialogBase
{

public:
*/
    /**
     * Constructor
     * @param title the title of the dialog
     */
/*
    ExportPasswordDialog(Gtk::Window& parentWindow, 
                                const Glib::ustring &title);
*/
    /**
     * Destructor.
     * Perform any necessary cleanups.
     */
/*
    ~ExportPasswordDialog();
*/

    /**
     * Show 2 entry to input username and password.
     */
/*
    bool show();

    Glib::ustring getUsername();
    Glib::ustring getPassword();
*/
    /**
     * Change the window title.
     */
/*
    void change_title(const Glib::ustring& title);

    Glib::ustring myUsername;
    Glib::ustring myPassword;

private:
*/
    /**
     * Fix to allow the user to type the file name
     */
/*
    Gtk::Entry *usernameEntry;
    Gtk::Entry *passwordEntry;
    
    // Child widgets
    Gtk::VBox entriesBox;
    Gtk::HBox userBox;
    Gtk::HBox passBox;
    
}; //ExportPasswordDialog
*/


//#########################################################################
//### F I L E   I M P O R T   F R O M   O C A L
//#########################################################################


/**
 * A Widget that contains an status icon and a message
 */
class StatusWidget : public Gtk::HBox
{
public:
    StatusWidget();

    void clear();
    void set_error(Glib::ustring text);
    void start_process(Glib::ustring text);
    void end_process();

    Gtk::Spinner* spinner;
    Gtk::Image* image;
    Gtk::Label* label;
};

/**
 * A Gtk::Entry with search & clear icons
 */
class SearchEntry : public Gtk::Entry
{
public:
    SearchEntry();
    
private:
    void _on_icon_pressed(Gtk::EntryIconPosition icon_position, const GdkEventButton* event);
    void _on_changed();
};

/**
 * A box which paints an overlay of the OCAL logo
 */
class LogoArea : public Gtk::EventBox
{
public:
    LogoArea();
private:
    bool _on_expose_event(GdkEventExpose* event);
    void _on_realize();
    bool draw_logo;
    Cairo::RefPtr<Cairo::ImageSurface> logo_mask;
    Glib::RefPtr<Pango::Layout> layout;
};

/**
 * A box filled with the Base colour from the user's GTK theme, and a border
 */
class BaseBox : public Gtk::EventBox
{
public:
    BaseBox();
private:
    bool _on_expose_event(GdkEventExpose* event);
};

enum {
    RESULTS_COLUMN_MARKUP,
    RESULTS_COLUMN_TITLE,
    RESULTS_COLUMN_DESCRIPTION,
    RESULTS_COLUMN_CREATOR,
    RESULTS_COLUMN_DATE,
    RESULTS_COLUMN_FILENAME,
    RESULTS_COLUMN_URL,
    RESULTS_COLUMN_THUMBNAIL_URL,
    RESULTS_COLUMN_LENGTH,
};

enum {
    NOTEBOOK_PAGE_LOGO,
    NOTEBOOK_PAGE_RESULTS,
    NOTEBOOK_PAGE_NOT_FOUND,
};

/**
 * The TreeView which holds the search results
 */
class SearchResultList : public Gtk::ListViewText
{
public:
    SearchResultList(guint columns_count, SVGPreview& filesPreview,
        Gtk::Label& description, Gtk::Button& okButton);
    void populate_from_xml(xmlNode* a_node);
};

/**
 * The Import Dialog
 */
class ImportDialog : public FileDialogBase
{
public:
    /**
     * Constructor
     * @param path the directory where to start searching
     * @param fileTypes one of FileDialogTypes
     * @param title the title of the dialog
     */
    ImportDialog(Gtk::Window& parent_window, const Glib::ustring &dir,
                 FileDialogType file_types, const Glib::ustring &title);

    /**
     * Destructor.
     * Perform any necessary cleanups.
     */
    ~ImportDialog();

    /**
     * Show an OpenFile file selector.
     * @return the selected path if user selected one, else NULL
     */
    bool show();

    /**
     * Return the 'key' (filetype) of the selection, if any
     * @return a pointer to a string if successful (which must
     * be later freed with g_free(), else NULL.
     */
    Inkscape::Extension::Extension *get_selection_type();
    
    typedef sigc::signal<void, Glib::ustring> type_signal_response;
    type_signal_response signal_response();
    
protected:
  type_signal_response m_signal_response;

private:
    Glib::ustring filename_image;
    Glib::ustring filename_thumbnail;
    SearchEntry *entry_search;
    LogoArea *drawingarea_logo;
    SearchResultList *list_results;
    SVGPreview *preview_files;
    Gtk::Label *label_not_found;
    Gtk::Label *label_description;
    Gtk::Button *button_search;
    Gtk::Button *button_import;
    Gtk::Button *button_cancel;
    StatusWidget *widget_status;

    // Child widgets
    Gtk::Notebook *notebook_content;
    Gtk::HBox hbox_tags;
    Gtk::HBox hbox_files;
    Gtk::ScrolledWindow scrolledwindow_list;
    Glib::RefPtr<Gtk::TreeSelection> selection;

    // XML
    Glib::RefPtr<Gio::File> xml_file;
    Glib::RefPtr<Gio::FileInputStream> xml_stream;
    Glib::ustring xml_uri;
    char xml_buffer[8192];

    // File
    Glib::RefPtr<Gio::File> file_local;
    Glib::RefPtr<Gio::File> file_remote;
    Glib::RefPtr<Gio::File> file_thumbnail_local;
    Glib::RefPtr<Gio::File> file_thumbnail_remote;

    void update_label_no_search_results();
    void update_preview(int row);
    void on_list_results_cursor_changed();
    void download_thumbnail_image(int row);
    void on_thumbnail_image_downloaded(const Glib::RefPtr<Gio::AsyncResult>& result);
    void download_image(int row);
    void on_image_downloaded(const Glib::RefPtr<Gio::AsyncResult>& result);
    void on_list_results_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
    void on_button_import_clicked();
    void on_button_search_clicked();
    void on_entry_search_activated();
    void on_xml_file_read(const Glib::RefPtr<Gio::AsyncResult>& result);


    /**
     * The extension to use to write this file
     */
    Inkscape::Extension::Extension *extension;

}; //ImportDialog


} //namespace OCAL
} //namespace Dialog
} //namespace UI
} //namespace Inkscape


#endif /* __OCAL_DIALOG_H__ */

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
