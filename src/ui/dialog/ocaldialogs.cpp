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
namespace OCAL
{

//########################################################################
//# F I L E    E X P O R T   T O   O C A L
//########################################################################

/**
 * Callback for fileNameEntry widget
 */
/*
void ExportDialog::fileNameEntryChangedCallback()
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
ExportDialog::ExportDialog(Gtk::Window &parentWindow,
            FileDialogType fileTypes,
            const Glib::ustring &title) :
    FileDialogBase(title, parentWindow)
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
             sigc::mem_fun(*this, &ExportDialog::fileNameEntryChangedCallback) );
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
ExportDialog::~ExportDialog()
{
}
*/
/**
 * Show this dialog modally.  Return true if user hits [OK]
 */
/*
bool
ExportDialog::show()
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
ExportDialog::get_filename()
{
    myFilename = fileNameEntry->get_text();
    if (!Glib::get_charset()) //If we are not utf8
        myFilename = Glib::filename_to_utf8(myFilename);

    return myFilename;
}


void
ExportDialog::change_title(const Glib::ustring& title)
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
ExportPasswordDialog::ExportPasswordDialog(Gtk::Window &parentWindow,
                             const Glib::ustring &title) : FileDialogBase(title, parentWindow)
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
ExportPasswordDialog::~ExportPasswordDialog()
{
}
*/
/**
 * Show this dialog modally.  Return true if user hits [OK]
 */
/*
bool
ExportPasswordDialog::show()
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
ExportPasswordDialog::getUsername()
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
ExportPasswordDialog::getPassword()
{
    myPassword = passwordEntry->get_text();
    return myPassword;
}

void
ExportPasswordDialog::change_title(const Glib::ustring& title)
{
    this->set_title(title);
}
*/

//#########################################################################
//### F I L E   I M P O R T   F R O M   O C A L
//#########################################################################

SearchResultList::SearchResultList(guint columns_count, SVGPreview& filesPreview,
    Gtk::Label& description, Gtk::Button& okButton) : ListViewText(columns_count)
{
    myPreview = &filesPreview;
    myLabel = &description;
    myButton = &okButton;

    set_headers_visible(false);
    set_column_title(RESULTS_COLUMN_MARKUP, _("Clipart found"));

    Gtk::CellRenderer* cr_markup = get_column_cell_renderer(RESULTS_COLUMN_MARKUP);
    cr_markup->set_property("ellipsize", Pango::ELLIPSIZE_END);
    get_column(RESULTS_COLUMN_MARKUP)->clear_attributes(*cr_markup);
    get_column(RESULTS_COLUMN_MARKUP)->add_attribute(*cr_markup,
        "markup", RESULTS_COLUMN_MARKUP);

    get_column(RESULTS_COLUMN_TITLE)->set_visible(false);
    get_column(RESULTS_COLUMN_DESCRIPTION)->set_visible(false);
    get_column(RESULTS_COLUMN_CREATOR)->set_visible(false);
    get_column(RESULTS_COLUMN_DATE)->set_visible(false);
    get_column(RESULTS_COLUMN_FILENAME)->set_visible(false);
    get_column(RESULTS_COLUMN_URL)->set_visible(false);
    get_column(RESULTS_COLUMN_THUMBNAIL_URL)->set_visible(false);
}

/*
 * Callback for cursor chage
 */
void SearchResultList::on_cursor_changed()
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
    myFilename.append(get_text(posArray[0], RESULTS_COLUMN_FILENAME));
    // rename based on original image's name, retaining extension
    if (rename(tmpname.c_str(),myFilename.c_str())<0) {
        unlink(tmpname.c_str());
        g_warning("Error creating destination file '%s': %s", myFilename.c_str(), strerror(errno));
        goto failquit;
    }

    //get file url
    fileUrl = get_text(posArray[0], RESULTS_COLUMN_URL); //http url

    //Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    //Glib::ustring fileUrl = "dav://"; //dav url
    //fileUrl.append(prefs->getString("/options/ocalurl/str"));
    //fileUrl.append("/dav.php/");
    //fileUrl.append(row[results_columns.CREATOR]); //author dir
    //fileUrl.append("/");
    //fileUrl.append(row[results_columns.FILENAME]); //filename

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
    myLabel->set_text(get_text(posArray[0], RESULTS_COLUMN_TITLE));
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
void SearchResultList::on_row_activated(const Gtk::TreeModel::Path& /*path*/, Gtk::TreeViewColumn* /*column*/)
{
    this->on_cursor_changed();
    myButton->activate();
}


/*
 * Returns the selected filename
 */
Glib::ustring SearchResultList::get_filename()
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
void ImportDialog::on_entry_search_changed()
{
    if (!entry_search)
        return;

    label_not_found->hide();
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
    GnomeVFSHandle *from_handle = NULL;
    GnomeVFSResult result;

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

    // clear the list_results
    list_results->clear_items();
    list_results->set_sensitive(false);

    // print all xml the element names
    print_xml_element_names(root_element);

    if (list_results->size() == 0) {
        label_not_found->show();
        list_results->set_sensitive(false);
    } else {
        for (guint i = 0; i <= list_results->size() - 1; i++) {
            Glib::ustring title = list_results->get_text(i, RESULTS_COLUMN_TITLE);
            Glib::ustring description = list_results->get_text(i, RESULTS_COLUMN_DESCRIPTION);
            char* markup = g_markup_printf_escaped("<b>%s</b>\n%s",
                title.c_str(), description.c_str());
            list_results->set_text(i, RESULTS_COLUMN_MARKUP, markup);
        }
        list_results->set_sensitive(true);
    }

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
void ImportDialog::print_xml_element_names(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;
    guint row_num = 0;
    
    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        // Get items information
        if (strcmp((const char*)cur_node->name, "rss")) // Avoid the root
            if (cur_node->type == XML_ELEMENT_NODE && !strcmp((const char*)cur_node->parent->name, "item"))
            {
                if (!strcmp((const char*)cur_node->name, "title"))
                {
                    row_num = list_results->append_text("");
                    xmlChar *xml_title = xmlNodeGetContent(cur_node);
                    char* title = (char*) xml_title;
                    
                    list_results->set_text(row_num, RESULTS_COLUMN_TITLE, title);
                    xmlFree(title);
                }
#ifdef WITH_GNOME_VFS
                else if (!strcmp((const char*)cur_node->name, "pubDate"))
                {
                    xmlChar *xml_date = xmlNodeGetContent(cur_node);
                    char* date = (char*) xml_date;
                    
                    list_results->set_text(row_num, RESULTS_COLUMN_DATE, date);
                    xmlFree(xml_date);
                }
                else if (!strcmp((const char*)cur_node->name, "creator"))
                {
                    xmlChar *xml_creator = xmlNodeGetContent(cur_node);
                    char* creator = (char*) xml_creator;
                    
                    list_results->set_text(row_num, RESULTS_COLUMN_CREATOR, creator);
                    xmlFree(xml_creator);
                }
                else if (!strcmp((const char*)cur_node->name, "description"))
                {
                    xmlChar *xml_description = xmlNodeGetContent(cur_node);
                    //char* final_description;
                    char* stripped_description = g_strstrip((char*) xml_description);

                    if (!strcmp(stripped_description, "")) {
                        stripped_description = _("No description");
                    }

                    //GRegex* regex = g_regex_new(g_regex_escape_string(stripped_description, -1));
                    //final_description = g_regex_replace_literal(regex, "\n", -1, 0, " ");

                    list_results->set_text(row_num, RESULTS_COLUMN_DESCRIPTION, stripped_description);
                    xmlFree(xml_description);
                }
                else if (!strcmp((const char*)cur_node->name, "enclosure"))
                {
                    xmlChar *xml_url = xmlGetProp(cur_node, (xmlChar*) "url");
                    char* url = (char*) xml_url;
                    char* filename = gnome_vfs_uri_extract_short_path_name(gnome_vfs_uri_new(url));

                    list_results->set_text(row_num, RESULTS_COLUMN_URL, url);
                    list_results->set_text(row_num, RESULTS_COLUMN_FILENAME, filename);
                    xmlFree(xml_url);
                }
                else if (!strcmp((const char*)cur_node->name, "thumbnail"))
                {
                    xmlChar *xml_thumbnail_url = xmlGetProp(cur_node, (xmlChar*) "url");
                    char* thumbnail_url = (char*) xml_thumbnail_url;

                    list_results->set_text(row_num, RESULTS_COLUMN_THUMBNAIL_URL, thumbnail_url);
                    xmlFree(xml_thumbnail_url);
                }
#endif
            }
        print_xml_element_names(cur_node->children);
    }
}

/**
 * Constructor.  Not called directly.  Use the factory.
 */
ImportDialog::ImportDialog(Gtk::Window& parent_window,
                                                   const Glib::ustring &/*dir*/,
                                                   FileDialogType file_types,
                                                   const Glib::ustring &title) :
    FileDialogBase(title, parent_window)
{
    // Initalize to Autodetect
    extension = NULL;
    // No filename to start out with
    Glib::ustring search_keywords = "";

    dialogType = file_types;

    // Creation
    Gtk::VBox *vbox = get_vbox();
    label_not_found = new Gtk::Label();
    label_description = new Gtk::Label();
    entry_search = new Gtk::Entry();
    button_search = new Gtk::Button(_("Search"));
    Gtk::HButtonBox* hbuttonbox_search = new Gtk::HButtonBox();
    preview_files = new SVGPreview();
    /// Add the buttons in the bottom of the dialog
    add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    button_import = add_button(_("Import"), Gtk::RESPONSE_OK);
    list_results = new SearchResultList(RESULTS_COLUMN_LENGTH,
        *preview_files, *label_description, *button_import);

    // Properties
    set_border_width(12);
    set_default_size(480, 320);
    vbox->set_spacing(12);
    entry_search->set_text(search_keywords);
    entry_search->set_max_length(255);
    hbox_tags.set_spacing(6);
    preview_files->showNoPreview();
    set_default(*button_import);
    list_results->set_sensitive(false);
    /// Add the listview inside a ScrolledWindow
    scrolledwindow_list.add(*list_results);
    scrolledwindow_list.set_shadow_type(Gtk::SHADOW_IN);
    /// Only show the scrollbars when they are necessary
    scrolledwindow_list.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scrolledwindow_list.set_size_request(310, 230);

    hbox_files.set_spacing(12);
    label_not_found->hide();
    
    // Packing
    hbuttonbox_search->pack_start(*button_search, false, false);
    hbox_tags.pack_start(*entry_search, true, true);
    hbox_tags.pack_start(*hbuttonbox_search, false, false);
    hbox_files.pack_start(scrolledwindow_list, true, true);
    hbox_files.pack_start(*preview_files, true, true);
    vbox->pack_start(hbox_tags, false, false);
    vbox->pack_start(hbox_files, true, true);
    
    // Signals
    entry_search = NULL;
    Gtk::Container *cont = get_toplevel();
    std::vector<Gtk::Entry *> entries;
    findEntryWidgets(cont, entries);
    
    if (entries.size() >=1 ) {
    /// Catch when user hits [return] on the text field
        entry_search = entries[0];
        entry_search->signal_activate().connect(
              sigc::mem_fun(*this, &ImportDialog::on_entry_search_changed));
    }

    button_search->signal_clicked().connect(
            sigc::mem_fun(*this, &ImportDialog::on_entry_search_changed));

    show_all_children();
}

/**
 * Destructor
 */
ImportDialog::~ImportDialog()
{

}

/**
 * Show this dialog modally.  Return true if user hits [OK]
 */
bool
ImportDialog::show()
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
ImportDialog::get_selection_type()
{
    return extension;
}


/**
 * Get the file name chosen by the user.   Valid after an [OK]
 */
Glib::ustring
ImportDialog::get_filename (void)
{
    return list_results->get_filename();
}


} //namespace OCAL
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
