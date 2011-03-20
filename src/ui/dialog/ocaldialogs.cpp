/** @file
 * @brief Open Clip Art Library integration dialogs - implementation
 */
/* Authors:
 *   Bruno Dilly
 *   Other dudes from The Inkscape Organization
 *   Andrew Higginson
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

#include "path-prefix.h"
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

SearchEntry::SearchEntry() : Gtk::Entry()
{
    signal_changed().connect(sigc::mem_fun(*this, &SearchEntry::_on_changed));
    signal_icon_press().connect(sigc::mem_fun(*this, &SearchEntry::_on_icon_pressed));

    set_icon_from_stock(Gtk::Stock::FIND, Gtk::ENTRY_ICON_PRIMARY);
    gtk_entry_set_icon_from_stock(gobj(), GTK_ENTRY_ICON_SECONDARY, NULL);
}

void SearchEntry::_on_icon_pressed(Gtk::EntryIconPosition icon_position, const GdkEventButton* event)
{
    if (icon_position == Gtk::ENTRY_ICON_SECONDARY) {
        grab_focus();
        set_text("");
    } else if (icon_position == Gtk::ENTRY_ICON_PRIMARY) {
        select_region(0, -1);
        grab_focus();
    }
}

void SearchEntry::_on_changed()
{
    if (get_text().empty()) {
        gtk_entry_set_icon_from_stock(gobj(), GTK_ENTRY_ICON_SECONDARY, NULL);
    } else {
        set_icon_from_stock(Gtk::Stock::CLEAR, Gtk::ENTRY_ICON_SECONDARY);
    }
}

LogoDrawingArea::LogoDrawingArea() : Gtk::DrawingArea()
{
    // Try to load the OCAL logo, but if the file is not found, degrade gracefully
    try {
        std::string logo_path = Glib::build_filename(INKSCAPE_PIXMAPDIR, "OCAL.png");
        logo_mask = Cairo::ImageSurface::create_from_png(logo_path);
        draw_logo = true;
    } catch( Cairo::logic_error ) {
        logo_mask = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 1,1);
        draw_logo = false;
    }
    signal_expose_event().connect(sigc::mem_fun(*this, &LogoDrawingArea::_on_expose_event));
    signal_realize().connect(sigc::mem_fun(*this, &LogoDrawingArea::_on_realize));
}

void LogoDrawingArea::_on_realize()
{
    Gdk::Color color = get_style()->get_mid(get_state());
    layout = this->create_pango_layout("");
    Glib::ustring markup = Glib::ustring::compose("<span color=\"%1\" size=\"large\">%2</span>",
        color.to_string(), _("Powered by"));
        
    layout->set_markup(markup);
}

bool LogoDrawingArea::_on_expose_event(GdkEventExpose* event)
{
    Cairo::RefPtr<Cairo::Context> cr = get_window()->create_cairo_context();

    // Draw background and shadow
    int width = get_allocation().get_width();
    int height = get_allocation().get_height();
    Gdk::Color background_fill = get_style()->get_base(get_state());

    cr->rectangle(0, 0, width, height);
    Gdk::Cairo::set_source_color(cr, background_fill);
    cr->fill();

    get_style()->paint_shadow(get_window(), get_state(), Gtk::SHADOW_IN,
        Gdk::Rectangle(0, 0, width, height),
        *this, Glib::ustring::ustring("viewport"), 0, 0, width, height);

    if (draw_logo) {
        // Draw logo, we mask [read fill] it with the mid colour from the
        // user's GTK theme
        Gdk::Color logo_fill = get_style()->get_mid(get_state());
        int x_logo = width - 12 - 127;
        int y_logo = height - 12 - 44;

        Gdk::Cairo::set_source_color(cr, logo_fill);
        cr->mask(logo_mask, x_logo, y_logo);

        // Draw text
        Pango::Rectangle extents = layout->get_pixel_logical_extents();
        int text_width = extents.get_width();
        int text_height = extents.get_height();

        int x_text = x_logo - text_width - 12;
        int y_text = height - text_height - 12;
            
        get_style()->paint_layout(get_window(), get_state(), true,
            Gdk::Rectangle(0, 0, width, height), *this, "", x_text, y_text, layout);
    }
    
    return false;
}

SearchResultList::SearchResultList(guint columns_count, SVGPreview& filesPreview,
    Gtk::Label& description, Gtk::Button& okButton) : ListViewText(columns_count)
{
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
void ImportDialog::on_list_results_cursor_changed()
{
    std::vector<Gtk::TreeModel::Path> pathlist;
    pathlist = list_results->get_selection()->get_selected_rows();
    std::vector<int> posArray(1);
    posArray = pathlist[0].get_indices();

    // FIXME: this would be better as a per-user OCAL cache of files
    // instead of filling /tmp with downloads.

    // Get Remote File URL
    Glib::ustring url = list_results->get_text(posArray[0], RESULTS_COLUMN_URL);
    file_remote = Gio::File::create_for_uri(url.c_str());

    // Create local file
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
    myFilename.append(list_results->get_text(posArray[0], RESULTS_COLUMN_FILENAME));
    // rename based on original image's name, retaining extension
    if (rename(tmpname.c_str(),myFilename.c_str())<0) {
        unlink(tmpname.c_str());
        g_warning("Error creating destination file '%s': %s", myFilename.c_str(), strerror(errno));
    }
    file_local = Gio::File::create_for_path(myFilename.c_str());

    
    //If we are not UTF8
    if (!Glib::get_charset()) {
        url = Glib::filename_to_utf8(url);
    }

    file_remote->copy_async(file_local, sigc::mem_fun(*this, &ImportDialog::on_file_copied),
        Gio::FILE_COPY_OVERWRITE);
}

/*
 * Callback for row activated
 */
void ImportDialog::on_file_copied(const Glib::RefPtr<Gio::AsyncResult>& result)
{
    bool success = file_remote->copy_finish(result);

    if (success) {
        preview_files->showImage(myFilename);
        //update_label_no_search_results(list_results->get_text(posArray[0], RESULTS_COLUMN_TITLE));
    } else {
        myFilename = "";
    }
}

/*
 * Callback for row activated
 */
void ImportDialog::on_list_results_row_activated(const Gtk::TreeModel::Path& path,
    Gtk::TreeViewColumn* column)
{
    on_list_results_cursor_changed();
    button_import->activate();
}

/**
 * Prints the names of the all the xml elements
 * that are siblings or children of a given xml node
 */
void SearchResultList::populate_from_xml(xmlNode * a_node)
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
                    row_num = append_text("");
                    xmlChar *xml_title = xmlNodeGetContent(cur_node);
                    char* title = (char*) xml_title;
                    
                    set_text(row_num, RESULTS_COLUMN_TITLE, title);
                    xmlFree(title);
                }
                else if (!strcmp((const char*)cur_node->name, "pubDate"))
                {
                    xmlChar *xml_date = xmlNodeGetContent(cur_node);
                    char* date = (char*) xml_date;
                    
                    set_text(row_num, RESULTS_COLUMN_DATE, date);
                    xmlFree(xml_date);
                }
                else if (!strcmp((const char*)cur_node->name, "creator"))
                {
                    xmlChar *xml_creator = xmlNodeGetContent(cur_node);
                    char* creator = (char*) xml_creator;
                    
                    set_text(row_num, RESULTS_COLUMN_CREATOR, creator);
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

                    set_text(row_num, RESULTS_COLUMN_DESCRIPTION, stripped_description);
                    xmlFree(xml_description);
                }
                else if (!strcmp((const char*)cur_node->name, "enclosure"))
                {
                    xmlChar *xml_url = xmlGetProp(cur_node, (xmlChar*) "url");
                    char* url = (char*) xml_url;
                    char* filename = g_path_get_basename(url);

                    set_text(row_num, RESULTS_COLUMN_URL, url);
                    set_text(row_num, RESULTS_COLUMN_FILENAME, filename);
                    xmlFree(xml_url);
                }
                else if (!strcmp((const char*)cur_node->name, "thumbnail"))
                {
                    xmlChar *xml_thumbnail_url = xmlGetProp(cur_node, (xmlChar*) "url");
                    char* thumbnail_url = (char*) xml_thumbnail_url;

                    set_text(row_num, RESULTS_COLUMN_THUMBNAIL_URL, thumbnail_url);
                    xmlFree(xml_thumbnail_url);
                }
            }
        populate_from_xml(cur_node->children);
    }
}

/**
 * Callback for user input into entry_search
 */
void ImportDialog::on_entry_search_activated()
{
    notebook_content->set_current_page(NOTEBOOK_PAGE_LOGO);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    Glib::ustring search_keywords = entry_search->get_text();
    
    // Create the URI to the OCAL RSS feed
    xml_uri = Glib::ustring::compose("http://%1/media/feed/rss/%2",
        prefs->getString("/options/ocalurl/str"), search_keywords);
    // If we are not UTF8
    if (!Glib::get_charset()) {
        xml_uri = Glib::filename_to_utf8(xml_uri);
    }

    // Open the rss feed
    xml_file = Gio::File::create_for_uri(xml_uri);
    xml_file->load_contents_async(sigc::mem_fun(*this, &ImportDialog::on_xml_file_read));
}

void ImportDialog::on_xml_file_read(const Glib::RefPtr<Gio::AsyncResult>& result)
{
    char* data;
    gsize length;
    
    bool sucess = xml_file->load_contents_finish(result, data, length);
    if (!sucess) {
        sp_ui_error_dialog(_("Failed to receive the Open Clip Art Library RSS feed. Verify if the server name is correct in Configuration->Import/Export (e.g.: openclipart.org)"));
        return;
    }

    // Create the resulting xml document tree
    // Initialize libxml and test mistakes between compiled and shared library used
    LIBXML_TEST_VERSION
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    doc = xmlReadMemory(data, (int) length, xml_uri.c_str(), NULL,
            XML_PARSE_RECOVER + XML_PARSE_NOWARNING + XML_PARSE_NOERROR);
        
    if (doc == NULL) {
        sp_ui_error_dialog(_("Server supplied malformed Clip Art feed"));
        g_warning("Failed to parse %s\n", xml_uri.c_str());
        return;
    }

    // get the root element node
    root_element = xmlDocGetRootElement(doc);

    // clear the list_results
    list_results->clear_items();

    list_results->populate_from_xml(root_element);

    if (list_results->size() == 0) {
        notebook_content->set_current_page(NOTEBOOK_PAGE_NOT_FOUND);
        update_label_no_search_results();
    } else {
        // Populate the MARKUP column with the title & description of the clipart
        for (guint i = 0; i <= list_results->size() - 1; i++) {
            Glib::ustring title = list_results->get_text(i, RESULTS_COLUMN_TITLE);
            Glib::ustring description = list_results->get_text(i, RESULTS_COLUMN_DESCRIPTION);
            char* markup = g_markup_printf_escaped("<b>%s</b>\n<span size=\"small\">%s</span>",
                title.c_str(), description.c_str());
            list_results->set_text(i, RESULTS_COLUMN_MARKUP, markup);
        }
        notebook_content->set_current_page(NOTEBOOK_PAGE_RESULTS);
    }

    // free the document
    xmlFreeDoc(doc);
    // free the global variables that may have been allocated by the parser
    xmlCleanupParser();
}


void ImportDialog::update_label_no_search_results()
{
    const char* keywords = entry_search->get_text().c_str();
    Gdk::Color grey = entry_search->get_style()->get_text_aa(entry_search->get_state());
    
    char* markup = g_markup_printf_escaped(
        "<span size=\"large\">%s<b>%s</b>%s</span>\n<span color=\"%s\">%s</span>",
        _("No clipart named "), keywords, _(" was found."), grey.to_string().c_str(),
        _("Please make sure all keywords are spelled correctly, or try again with different keywords."));
    
    label_not_found->set_markup(markup);
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
    entry_search = new SearchEntry();
    button_search = new Gtk::Button(_("Search"));
    Gtk::HButtonBox* hbuttonbox_search = new Gtk::HButtonBox();
    preview_files = new SVGPreview();
    /// Add the buttons in the bottom of the dialog
    add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    button_import = add_button(_("Import"), Gtk::RESPONSE_OK);
    list_results = new SearchResultList(RESULTS_COLUMN_LENGTH,
            *preview_files, *label_description, *button_import);
    drawingarea_logo = new LogoDrawingArea();
    notebook_content = new Gtk::Notebook();
    
    // Packing
    hbuttonbox_search->pack_start(*button_search, false, false);
    hbox_tags.pack_start(*entry_search, true, true);
    hbox_tags.pack_start(*hbuttonbox_search, false, false);
    hbox_files.pack_start(*notebook_content, true, true);
    hbox_files.pack_start(*preview_files, true, true);
    vbox->pack_start(hbox_tags, false, false);
    vbox->pack_start(hbox_files, true, true);

    notebook_content->insert_page(*drawingarea_logo, NOTEBOOK_PAGE_LOGO);
    notebook_content->insert_page(scrolledwindow_list, NOTEBOOK_PAGE_RESULTS);
    notebook_content->insert_page(*label_not_found, NOTEBOOK_PAGE_NOT_FOUND);

    // Properties
    set_border_width(12);
    set_default_size(480, 320);
    vbox->set_spacing(12);
    entry_search->set_max_length(255);
    hbox_tags.set_spacing(6);
    preview_files->showNoPreview();
    set_default(*button_import);
    notebook_content->set_current_page(NOTEBOOK_PAGE_LOGO);
    /// Add the listview inside a ScrolledWindow
    scrolledwindow_list.add(*list_results);
    scrolledwindow_list.set_shadow_type(Gtk::SHADOW_IN);
    /// Only show the scrollbars when they are necessary
    scrolledwindow_list.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scrolledwindow_list.set_size_request(310, 230);
    drawingarea_logo->set_size_request(310, 230);
    hbox_files.set_spacing(12);
    notebook_content->set_show_tabs(false);
    notebook_content->set_show_border(false);
    
    // Signals
    entry_search->signal_activate().connect(
            sigc::mem_fun(*this, &ImportDialog::on_entry_search_activated));
    button_search->signal_clicked().connect(
            sigc::mem_fun(*this, &ImportDialog::on_entry_search_activated));
    list_results->signal_cursor_changed().connect(
            sigc::mem_fun(*this, &ImportDialog::on_list_results_cursor_changed));
    list_results->signal_row_activated().connect(
            sigc::mem_fun(*this, &ImportDialog::on_list_results_row_activated));

    show_all_children();
    entry_search->grab_focus();
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
bool ImportDialog::show()
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
    return myFilename;
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
