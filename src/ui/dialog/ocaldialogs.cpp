/** @file
 * @brief Open Clip Art Library integration dialogs - implementation
 */
/* Authors:
 *   Bruno Dilly
 *   Other dudes from The Inkscape Organization
 *
 * Copyright (C) 2007 Bruno Dilly <bruno.dilly@gmail.com>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>  // rename()
#include <unistd.h> // close()
#include <errno.h>  // errno
#include <string.h> // strerror()

#include "ocaldialogs.h"
#include "filedialogimpl-gtkmm.h"
#include "interface.h"
#include "gc-core.h"
#include <dialogs/dialog-events.h>
#include "io/sys.h"
#include "preferences.h"

namespace Inkscape
{
namespace UI
{
namespace Dialog
{

//########################################################################
//# F I L E    E X P O R T   T O   O C A L
//########################################################################

/**
 * Callback for fileNameEntry widget
 */
/*
void FileExportToOCALDialog::fileNameEntryChangedCallback()
{
    if (!fileNameEntry)
        return;

    Glib::ustring fileName = fileNameEntry->get_text();
    if (!Glib::get_charset()) //If we are not utf8
        fileName = Glib::filename_to_utf8(fileName);

    myFilename = fileName;
    response(Gtk::RESPONSE_OK);
}
*/
/**
 * Constructor
 */
/*
FileExportToOCALDialog::FileExportToOCALDialog(Gtk::Window &parentWindow,
            FileDialogType fileTypes,
            const Glib::ustring &title) :
    FileDialogOCALBase(title, parentWindow)
{
*/
     /*
     * Start Taking the vertical Box and putting a Label
     * and a Entry to take the filename
     * Later put the extension selection and checkbox (?)
     */
    /* Initalize to Autodetect */
/*
    extension = NULL;
*/
    /* No filename to start out with */
/*
    myFilename = "";
*/
    /* Set our dialog type (save, export, etc...)*/
/*
    dialogType = fileTypes;
    Gtk::VBox *vbox = get_vbox();

    Gtk::Label *fileLabel = new Gtk::Label(_("File"));

    fileNameEntry = new Gtk::Entry();
    fileNameEntry->set_text(myFilename);
    fileNameEntry->set_max_length(252); // I am giving the extension approach.
    fileBox.pack_start(*fileLabel);
    fileBox.pack_start(*fileNameEntry, Gtk::PACK_EXPAND_WIDGET, 3);
    vbox->pack_start(fileBox);

    //Let's do some customization
    fileNameEntry = NULL;
    Gtk::Container *cont = get_toplevel();
    std::vector<Gtk::Entry *> entries;
    findEntryWidgets(cont, entries);
    if (entries.size() >=1 )
        {
        //Catch when user hits [return] on the text field
        fileNameEntry = entries[0];
        fileNameEntry->signal_activate().connect(
             sigc::mem_fun(*this, &FileExportToOCALDialog::fileNameEntryChangedCallback) );
        }

    add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    set_default(*add_button(Gtk::Stock::SAVE,   Gtk::RESPONSE_OK));

    show_all_children();
}
*/
/**
 * Destructor
 */
/*
FileExportToOCALDialog::~FileExportToOCALDialog()
{
}
*/
/**
 * Show this dialog modally.  Return true if user hits [OK]
 */
/*
bool
FileExportToOCALDialog::show()
{
    set_modal (TRUE);                      //Window
    sp_transientize((GtkWidget *)gobj());  //Make transient
    gint b = run();                        //Dialog
    hide();

    if (b == Gtk::RESPONSE_OK)
    {
        return TRUE;
        }
    else
        {
        return FALSE;
        }
}
*/
/**
 * Get the file name chosen by the user.   Valid after an [OK]
 */
/*
Glib::ustring
FileExportToOCALDialog::get_filename()
{
    myFilename = fileNameEntry->get_text();
    if (!Glib::get_charset()) //If we are not utf8
        myFilename = Glib::filename_to_utf8(myFilename);

    return myFilename;
}


void
FileExportToOCALDialog::change_title(const Glib::ustring& title)
{
    this->set_title(title);
}
*/

//########################################################################
//# F I L E    E X P O R T   T O   O C A L   P A S S W O R D
//########################################################################


/**
 * Constructor
 */
/*
FileExportToOCALPasswordDialog::FileExportToOCALPasswordDialog(Gtk::Window &parentWindow,
                             const Glib::ustring &title) : FileDialogOCALBase(title, parentWindow)
{
*/
    /*
     * Start Taking the vertical Box and putting 2 Labels
     * and 2 Entries to take the username and password
     */
    /* No username and password to start out with */
/*
    myUsername = "";
    myPassword = "";

    Gtk::VBox *vbox = get_vbox();

    Gtk::Label *userLabel = new Gtk::Label(_("Username:"));
    Gtk::Label *passLabel = new Gtk::Label(_("Password:"));

    usernameEntry = new Gtk::Entry();
    usernameEntry->set_text(myUsername);
    usernameEntry->set_max_length(255);

    passwordEntry = new Gtk::Entry();
    passwordEntry->set_text(myPassword);
    passwordEntry->set_max_length(255);
    passwordEntry->set_invisible_char('*');
    passwordEntry->set_visibility(false);
    passwordEntry->set_activates_default(true);

    userBox.pack_start(*userLabel);
    userBox.pack_start(*usernameEntry, Gtk::PACK_EXPAND_WIDGET, 3);
    vbox->pack_start(userBox);

    passBox.pack_start(*passLabel);
    passBox.pack_start(*passwordEntry, Gtk::PACK_EXPAND_WIDGET, 3);
    vbox->pack_start(passBox);

    add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    set_default(*add_button(Gtk::Stock::OK,   Gtk::RESPONSE_OK));

    show_all_children();
}
*/

/**
 * Destructor
 */
/*
FileExportToOCALPasswordDialog::~FileExportToOCALPasswordDialog()
{
}
*/
/**
 * Show this dialog modally.  Return true if user hits [OK]
 */
/*
bool
FileExportToOCALPasswordDialog::show()
{
    set_modal (TRUE);                      //Window
    sp_transientize((GtkWidget *)gobj());  //Make transient
    gint b = run();                        //Dialog
    hide();

    if (b == Gtk::RESPONSE_OK)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
*/
/**
 * Get the username.   Valid after an [OK]
 */
/*
Glib::ustring
FileExportToOCALPasswordDialog::getUsername()
{
    myUsername = usernameEntry->get_text();
    return myUsername;
}
*/
/**
 * Get the password.   Valid after an [OK]
 */
/*
Glib::ustring
FileExportToOCALPasswordDialog::getPassword()
{
    myPassword = passwordEntry->get_text();
    return myPassword;
}

void
FileExportToOCALPasswordDialog::change_title(const Glib::ustring& title)
{
    this->set_title(title);
}
*/

//#########################################################################
//### F I L E   I M P O R T   F R O M   O C A L
//#########################################################################

/*
 * Calalback for cursor chage
 */
void FileListViewText::on_cursor_changed()
{
    std::vector<Gtk::TreeModel::Path> pathlist;
    pathlist = this->get_selection()->get_selected_rows();
    std::vector<int> posArray(1);
    posArray = pathlist[0].get_indices();

#ifdef WITH_GNOME_VFS
    gnome_vfs_init();
    GnomeVFSHandle    *from_handle = NULL;
    GnomeVFSHandle    *to_handle = NULL;
    GnomeVFSFileSize  bytes_read;
    GnomeVFSFileSize  bytes_written;
    GnomeVFSResult    result;
    guint8 buffer[8192];
    Glib::ustring fileUrl;

    // FIXME: this would be better as a per-user OCAL cache of files
    // instead of filling /tmp with downloads.
    //
    // create file path
    const std::string tmptemplate = "ocal-";
    std::string tmpname;
    int fd = Inkscape::IO::file_open_tmp(tmpname, tmptemplate);
    if (fd<0) {
        g_warning("Error creating temp file");
        return;
    }
    close(fd);
    // make sure we don't collide with other users on the same machine
    myFilename = tmpname;
    myFilename.append("-");
    myFilename.append(get_text(posArray[0], 2));
    // rename based on original image's name, retaining extension
    if (rename(tmpname.c_str(),myFilename.c_str())<0) {
        unlink(tmpname.c_str());
        g_warning("Error creating destination file '%s': %s", myFilename.c_str(), strerror(errno));
        goto failquit;
    }

    //get file url
    fileUrl = get_text(posArray[0], 1); //http url

    //Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    //Glib::ustring fileUrl = "dav://"; //dav url
    //fileUrl.append(prefs->getString("/options/ocalurl/str"));
    //fileUrl.append("/dav.php/");
    //fileUrl.append(get_text(posArray[0], 3)); //author dir
    //fileUrl.append("/");
    //fileUrl.append(get_text(posArray[0], 2)); //filename

    if (!Glib::get_charset()) //If we are not utf8
        fileUrl = Glib::filename_to_utf8(fileUrl);

    {
        // open the temp file to receive
        result = gnome_vfs_open (&to_handle, myFilename.c_str(), GNOME_VFS_OPEN_WRITE);
        if (result == GNOME_VFS_ERROR_NOT_FOUND){
            result = gnome_vfs_create (&to_handle, myFilename.c_str(), GNOME_VFS_OPEN_WRITE, FALSE, GNOME_VFS_PERM_USER_ALL);
        }
        if (result != GNOME_VFS_OK) {
            g_warning("Error creating temp file '%s': %s", myFilename.c_str(), gnome_vfs_result_to_string(result));
            goto fail;
        }
        result = gnome_vfs_open (&from_handle, fileUrl.c_str(), GNOME_VFS_OPEN_READ);
        if (result != GNOME_VFS_OK) {
            g_warning("Could not find the file in Open Clip Art Library.");
            goto fail;
        }
        // copy the file
        while (1) {
            result = gnome_vfs_read (from_handle, buffer, 8192, &bytes_read);
            if ((result == GNOME_VFS_ERROR_EOF) &&(!bytes_read)){
                result = gnome_vfs_close (from_handle);
                result = gnome_vfs_close (to_handle);
                break;
            }
            if (result != GNOME_VFS_OK) {
                g_warning("%s", gnome_vfs_result_to_string(result));
                goto fail;
            }
            result = gnome_vfs_write (to_handle, buffer, bytes_read, &bytes_written);
            if (result != GNOME_VFS_OK) {
                g_warning("%s", gnome_vfs_result_to_string(result));
                goto fail;
            }
            if (bytes_read != bytes_written){
                g_warning("Bytes read not equal to bytes written");
                goto fail;
            }
        }
    }
    myPreview->showImage(myFilename);
    myLabel->set_text(get_text(posArray[0], 4));
#endif
    return;
fail:
    unlink(myFilename.c_str());
failquit:
    myFilename = "";
}


/*
 * Callback for row activated
 */
void FileListViewText::on_row_activated(const Gtk::TreeModel::Path& /*path*/, Gtk::TreeViewColumn* /*column*/)
{
    this->on_cursor_changed();
    myButton->activate();
}


/*
 * Returns the selected filename
 */
Glib::ustring FileListViewText::get_filename()
{
    return myFilename;
}


#ifdef WITH_GNOME_VFS
/**
 * Read callback for xmlReadIO(), used below
 */
static int vfs_read_callback (GnomeVFSHandle *handle, char* buf, int nb)
{
    GnomeVFSFileSize ndone;
    GnomeVFSResult    result;

    result = gnome_vfs_read (handle, buf, nb, &ndone);

    if (result == GNOME_VFS_OK) {
        return (int)ndone;
    } else {
        if (result != GNOME_VFS_ERROR_EOF) {
            sp_ui_error_dialog(_("Error while reading the Open Clip Art RSS feed"));
            g_warning("%s\n", gnome_vfs_result_to_string(result));
        }
        return -1;
    }
}
#endif


/**
 * Callback for user input into entry_search
 */
void FileImportFromOCALDialog::on_entry_search_changed()
{
    if (!entry_search)
        return;

    label_not_found->hide();
    label_description->set_text("");
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    Glib::ustring search_keywords = entry_search->get_text();
    // create the ocal uri to get rss feed
    Glib::ustring uri = "http://";
    uri.append(prefs->getString("/options/ocalurl/str"));
    uri.append("/media/feed/rss/");
    uri.append(search_keywords);
    if (!Glib::get_charset()) //If we are not utf8
        uri = Glib::filename_to_utf8(uri);

#ifdef WITH_GNOME_VFS

    // open the rss feed
    gnome_vfs_init();
    GnomeVFSHandle    *from_handle = NULL;
    GnomeVFSResult    result;

    result = gnome_vfs_open (&from_handle, uri.c_str(), GNOME_VFS_OPEN_READ);
    if (result != GNOME_VFS_OK) {
        sp_ui_error_dialog(_("Failed to receive the Open Clip Art Library RSS feed. Verify if the server name is correct in Configuration->Import/Export (e.g.: openclipart.org)"));
        return;
    }

    // create the resulting xml document tree
    // this initialize the library and test mistakes between compiled and shared library used
    LIBXML_TEST_VERSION
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    doc = xmlReadIO ((xmlInputReadCallback) vfs_read_callback,
        (xmlInputCloseCallback) gnome_vfs_close, from_handle, uri.c_str(), NULL,
        XML_PARSE_RECOVER + XML_PARSE_NOWARNING + XML_PARSE_NOERROR);
    if (doc == NULL) {
        sp_ui_error_dialog(_("Server supplied malformed Clip Art feed"));
        g_warning("Failed to parse %s\n", uri.c_str());
        return;
    }

    // get the root element node
    root_element = xmlDocGetRootElement(doc);

    // clear the list_files
    list_files->clear_items();
    list_files->set_sensitive(false);

    // print all xml the element names
    print_xml_element_names(root_element);

    if (list_files->size() == 0)
    {
        label_not_found->show();
        list_files->set_sensitive(false);
    }
    else
        list_files->set_sensitive(true);

    // free the document
    xmlFreeDoc(doc);
    // free the global variables that may have been allocated by the parser
    xmlCleanupParser();
    return;
#endif
}


/**
 * Prints the names of the all the xml elements
 * that are siblings or children of a given xml node
 */
void FileImportFromOCALDialog::print_xml_element_names(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;
    guint row_num = 0;
    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        // get itens information
        if (strcmp((const char*)cur_node->name, "rss")) //avoid the root
            if (cur_node->type == XML_ELEMENT_NODE && !strcmp((const char*)cur_node->parent->name, "item"))
            {
                if (!strcmp((const char*)cur_node->name, "title"))
                {
                    xmlChar *title = xmlNodeGetContent(cur_node);
                    row_num = list_files->append_text((const char*)title);
                    xmlFree(title);
                }
#ifdef WITH_GNOME_VFS
                else if (!strcmp((const char*)cur_node->name, "enclosure"))
                {
                    xmlChar *urlattribute = xmlGetProp(cur_node, (xmlChar*)"url");
                    list_files->set_text(row_num, 1, (const char*)urlattribute);
                    gchar *tmp_file;
                    tmp_file = gnome_vfs_uri_extract_short_path_name(gnome_vfs_uri_new((const char*)urlattribute));
                    list_files->set_text(row_num, 2, (const char*)tmp_file);
                    xmlFree(urlattribute);
                }
                else if (!strcmp((const char*)cur_node->name, "creator"))
                {
                    list_files->set_text(row_num, 3, (const char*)xmlNodeGetContent(cur_node));
                }
                else if (!strcmp((const char*)cur_node->name, "description"))
                {
                    list_files->set_text(row_num, 4, (const char*)xmlNodeGetContent(cur_node));
                }
#endif
            }
        print_xml_element_names(cur_node->children);
    }
}

/**
 * Constructor.  Not called directly.  Use the factory.
 */
FileImportFromOCALDialog::FileImportFromOCALDialog(Gtk::Window& parent_window,
                                                   const Glib::ustring &/*dir*/,
                                                   FileDialogType file_types,
                                                   const Glib::ustring &title) :
    FileDialogOCALBase(title, parent_window)
{
    // Initalize to Autodetect
    extension = NULL;
    // No filename to start out with
    Glib::ustring search_keywords = "";

    dialogType = file_types;

    // Creation
    Gtk::VBox *vbox = get_vbox();
    label_not_found = new Gtk::Label(_("No files matched your search"));
    label_description = new Gtk::Label();
    entry_search = new Gtk::Entry();
    button_search = new Gtk::Button(_("Search"));
    Gtk::HButtonBox* hbuttonbox_search = new Gtk::HButtonBox();
    preview_files = new SVGPreview();
    /// Add the buttons in the bottom of the dialog
    add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    button_ok = add_button(Gtk::Stock::OPEN,   Gtk::RESPONSE_OK);
    list_files = new FileListViewText(5, *preview_files, *label_description, *button_ok);

    // Properties
    set_border_width(12);
    set_default_size(480, 350);
    vbox->set_spacing(12);
    label_description->set_max_width_chars(260);
    label_description->set_size_request(500, -1);
    label_description->set_single_line_mode(false);
    label_description->set_line_wrap(true);
    entry_search->set_text(search_keywords);
    entry_search->set_max_length(255);
    hbox_tags.set_spacing(6);
    preview_files->showNoPreview();
    set_default(*button_ok);
    list_files->set_sensitive(false);
    /// Add the listview inside a ScrolledWindow
    scrolledwindow_list.add(*list_files);
    scrolledwindow_list.set_shadow_type(Gtk::SHADOW_IN);
    /// Only show the scrollbars when they are necessary
    scrolledwindow_list.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS);
    list_files->set_column_title(0, _("Files found"));
    scrolledwindow_list.set_size_request(400, 180);

    list_files->set_headers_visible(false);
    list_files->get_column(1)->set_visible(false); // file url
    list_files->get_column(2)->set_visible(false); // tmp file path
    list_files->get_column(3)->set_visible(false); // author dir
    list_files->get_column(4)->set_visible(false); // file description

    hbox_files.set_spacing(12);
    label_not_found->hide();
    
    // Packing
    hbox_message.pack_start(*label_not_found, false, false);
    hbox_description.pack_start(*label_description, false, false);
    hbuttonbox_search->pack_start(*button_search, false, false);
    hbox_tags.pack_start(*entry_search, true, true);
    hbox_tags.pack_start(*hbuttonbox_search, false, false);
    hbox_files.pack_start(scrolledwindow_list, true, true);
    hbox_files.pack_start(*preview_files, true, true);
    vbox->pack_start(hbox_tags, false, false);
    vbox->pack_start(hbox_message, false, false);
    vbox->pack_start(hbox_files, true, true);
    vbox->pack_start(hbox_description, false, false);
    
    // Signals
    entry_search = NULL;
    Gtk::Container *cont = get_toplevel();
    std::vector<Gtk::Entry *> entries;
    findEntryWidgets(cont, entries);
    
    if (entries.size() >=1 ) {
    /// Catch when user hits [return] on the text field
        entry_search = entries[0];
        entry_search->signal_activate().connect(
              sigc::mem_fun(*this, &FileImportFromOCALDialog::on_entry_search_changed));
    }

    button_search->signal_clicked().connect(
            sigc::mem_fun(*this, &FileImportFromOCALDialog::on_entry_search_changed));

    show_all_children();
}

/**
 * Destructor
 */
FileImportFromOCALDialog::~FileImportFromOCALDialog()
{

}

/**
 * Show this dialog modally.  Return true if user hits [OK]
 */
bool
FileImportFromOCALDialog::show()
{
    set_modal (TRUE);                      //Window
    sp_transientize((GtkWidget *)gobj());  //Make transient
    gint b = run();                        //Dialog
    hide();

    if (b == Gtk::RESPONSE_OK)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/**
 * Get the file extension type that was selected by the user. Valid after an [OK]
 */
Inkscape::Extension::Extension *
FileImportFromOCALDialog::get_selection_type()
{
    return extension;
}


/**
 * Get the file name chosen by the user.   Valid after an [OK]
 */
Glib::ustring
FileImportFromOCALDialog::get_filename (void)
{
    return list_files->get_filename();
}


} //namespace Dialog
} //namespace UI
} //namespace Inkscape



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
