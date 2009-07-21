#include "document-interface.h"
#include <string.h>

#include "verbs.h"
#include "helper/action.h" //sp_action_perform

#include "inkscape.h" //inkscape_find_desktop_by_dkey, activate desktops

#include "desktop-handles.h" //sp_desktop_document()
#include "xml/repr.h" //sp_repr_document_new

#include "sp-object.h"

#include "document.h" // sp_document_repr_doc

#include "desktop-style.h" //sp_desktop_get_style

#include "selection.h" //selection struct
#include "selection-chemistry.h"// lots of selection functions

#include "sp-ellipse.h"

#include "layer-fns.h" //LPOS_BELOW

#include "style.h" //style_write

#include "file.h" //IO

#include "extension/system.h" //IO

#include "extension/output.h" //IO

#include "print.h" //IO

#include "live_effects/parameter/text.h" //text
#include "display/canvas-text.h" //text

#include "2geom/svg-path-parser.h" //get_node_coordinates

/****************************************************************************
     HELPER / SHORTCUT FUNCTIONS
****************************************************************************/

const gchar* intToCString(int i)
{
    std::stringstream ss;
    ss << i;
    return ss.str().c_str();
}

SPObject *
get_object_by_name (SPDesktop *desk, gchar *name)
{
    return sp_desktop_document(desk)->getObjectById(name);
}

const gchar *
get_name_from_object (SPObject * obj)
{
    return obj->repr->attribute("id");
}

void
desktop_ensure_active (SPDesktop* desk) {
    if (desk != SP_ACTIVE_DESKTOP)
        inkscape_activate_desktop (desk);
    return;
}

Inkscape::XML::Node *
document_retrive_node (SPDocument *doc, gchar *name) {
    return (doc->getObjectById(name))->repr;
}

gdouble
selection_get_center_x (Inkscape::Selection *sel){
    NRRect *box = g_new(NRRect, 1);;
    box = sel->boundsInDocument(box);
    return box->x0 + ((box->x1 - box->x0)/2);
}

gdouble
selection_get_center_y (Inkscape::Selection *sel){
    NRRect *box = g_new(NRRect, 1);;
    box = sel->boundsInDocument(box);
    return box->y0 + ((box->y1 - box->y0)/2);
}
//move_to etc
const GSList *
selection_swap(SPDesktop *desk, gchar *name)
{
    Inkscape::Selection *sel = sp_desktop_selection(desk);
    const GSList *oldsel = g_slist_copy((GSList *)sel->list());
    
    sel->set(get_object_by_name(desk, name));
    return oldsel;
}

void
selection_restore(SPDesktop *desk, const GSList * oldsel)
{
    Inkscape::Selection *sel = sp_desktop_selection(desk);
    sel->setList(oldsel);
}

Inkscape::XML::Node *
dbus_create_node (SPDesktop *desk, gboolean isrect)
{
    SPDocument * doc = sp_desktop_document (desk);
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(doc);
    gchar *type;
    if (isrect)
        type = (gchar *)"svg:rect";
    else
        type = (gchar *)"svg:path";
    return xml_doc->createElement(type);
}

gchar *
finish_create_shape (DocumentInterface *object, GError **error, Inkscape::XML::Node *newNode, gchar *desc)
{

    SPCSSAttr *style = sp_desktop_get_style(object->desk, TRUE);
    
    if (style) {
        newNode->setAttribute("style", sp_repr_css_write_string(style), TRUE);
    }
    else {
        newNode->setAttribute("style", "fill:#0000ff;fill-opacity:1;stroke:#c900b9;stroke-width:0;stroke-miterlimit:0;stroke-opacity:1;stroke-dasharray:none", TRUE);
    }

    object->desk->currentLayer()->appendChildRepr(newNode);
    object->desk->currentLayer()->updateRepr();

    if (object->updates)
        sp_document_done(sp_desktop_document(object->desk), 0, (gchar *)desc);
    else
        document_interface_pause_updates(object, error);

    return strdup(newNode->attribute("id"));
}

gboolean
dbus_call_verb (DocumentInterface *object, int verbid, GError **error)
{    
    SPDesktop *desk2 = object->desk;

    if ( desk2 ) {
        Inkscape::Verb *verb = Inkscape::Verb::get( verbid );
        if ( verb ) {
            SPAction *action = verb->get_action(desk2);
            if ( action ) {
                sp_action_perform( action, NULL );
                if (object->updates) {
                    sp_document_done(sp_desktop_document(desk2), verb->get_code(), g_strdup(verb->get_tip()));
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

/****************************************************************************
     DOCUMENT INTERFACE CLASS STUFF
****************************************************************************/

G_DEFINE_TYPE(DocumentInterface, document_interface, G_TYPE_OBJECT)

static void
document_interface_finalize (GObject *object)
{
        G_OBJECT_CLASS (document_interface_parent_class)->finalize (object);
}


static void
document_interface_class_init (DocumentInterfaceClass *klass)
{
        GObjectClass *object_class;
        object_class = G_OBJECT_CLASS (klass);
        object_class->finalize = document_interface_finalize;
}

static void
document_interface_init (DocumentInterface *object)
{
	object->desk = NULL;
}


DocumentInterface *
document_interface_new (void)
{
        return (DocumentInterface*)g_object_new (TYPE_DOCUMENT_INTERFACE, NULL);
}

/****************************************************************************
     MISC FUNCTIONS
****************************************************************************/

gboolean
document_interface_delete_all (DocumentInterface *object, GError **error)
{
    sp_edit_clear_all (object->desk);
    return TRUE;
}

void
document_interface_call_verb (DocumentInterface *object, gchar *verbid, GError **error)
{
    SPDesktop *desk2 = object->desk;
    desktop_ensure_active (object->desk);
    if ( desk2 ) {
        Inkscape::Verb *verb = Inkscape::Verb::getbyid( verbid );
        if ( verb ) {
            SPAction *action = verb->get_action(desk2);
            if ( action ) {
                sp_action_perform( action, NULL );
                if (object->updates) {
                    sp_document_done(sp_desktop_document(desk2), verb->get_code(), g_strdup(verb->get_tip()));
                }
            }
        }
    }
}


/****************************************************************************
     CREATION FUNCTIONS
****************************************************************************/

gchar* 
document_interface_rectangle (DocumentInterface *object, int x, int y, 
                              int width, int height, GError **error)
{


    Inkscape::XML::Node *newNode = dbus_create_node(object->desk, TRUE);
    sp_repr_set_int(newNode, "x", x);  //could also use newNode->setAttribute()
    sp_repr_set_int(newNode, "y", y);
    sp_repr_set_int(newNode, "width", width);
    sp_repr_set_int(newNode, "height", height);
    return finish_create_shape (object, error, newNode, (gchar *)"create rectangle");
}

gchar*
document_interface_ellipse_center (DocumentInterface *object, int cx, int cy, 
                                   int rx, int ry, GError **error)
{
    Inkscape::XML::Node *newNode = dbus_create_node(object->desk, FALSE);
    newNode->setAttribute("sodipodi:type", "arc");
    sp_repr_set_int(newNode, "sodipodi:cx", cx);
    sp_repr_set_int(newNode, "sodipodi:cy", cy);
    sp_repr_set_int(newNode, "sodipodi:rx", rx);
    sp_repr_set_int(newNode, "sodipodi:ry", ry);
    return finish_create_shape (object, error, newNode, (gchar *)"create circle");
}

gchar* 
document_interface_polygon (DocumentInterface *object, int cx, int cy, 
                            int radius, int rotation, int sides, 
                            GError **error)
{
    gdouble rot = ((rotation / 180.0) * 3.14159265) - ( 3.14159265 / 2.0);
    Inkscape::XML::Node *newNode = dbus_create_node(object->desk, FALSE);
    newNode->setAttribute("inkscape:flatsided", "true");
    newNode->setAttribute("sodipodi:type", "star");
    sp_repr_set_int(newNode, "sodipodi:cx", cx);
    sp_repr_set_int(newNode, "sodipodi:cy", cy);
    sp_repr_set_int(newNode, "sodipodi:r1", radius);
    sp_repr_set_int(newNode, "sodipodi:r2", radius);
    sp_repr_set_int(newNode, "sodipodi:sides", sides);
    sp_repr_set_int(newNode, "inkscape:randomized", 0);
    sp_repr_set_svg_double(newNode, "sodipodi:arg1", rot);
    sp_repr_set_svg_double(newNode, "sodipodi:arg2", rot);
    sp_repr_set_svg_double(newNode, "inkscape:rounded", 0);

    return finish_create_shape (object, error, newNode, (gchar *)"create polygon");
}

gchar* 
document_interface_star (DocumentInterface *object, int cx, int cy, 
                         int r1, int r2, int sides, gdouble rounded,
                         gdouble arg1, gdouble arg2, GError **error)
{
    Inkscape::XML::Node *newNode = dbus_create_node(object->desk, FALSE);
    newNode->setAttribute("inkscape:flatsided", "false");
    newNode->setAttribute("sodipodi:type", "star");
    sp_repr_set_int(newNode, "sodipodi:cx", cx);
    sp_repr_set_int(newNode, "sodipodi:cy", cy);
    sp_repr_set_int(newNode, "sodipodi:r1", r1);
    sp_repr_set_int(newNode, "sodipodi:r2", r2);
    sp_repr_set_int(newNode, "sodipodi:sides", sides);
    sp_repr_set_int(newNode, "inkscape:randomized", 0);
    sp_repr_set_svg_double(newNode, "sodipodi:arg1", arg1);
    sp_repr_set_svg_double(newNode, "sodipodi:arg2", arg2);
    sp_repr_set_svg_double(newNode, "inkscape:rounded", rounded);

    return finish_create_shape (object, error, newNode, (gchar *)"create star");
}

gchar* 
document_interface_ellipse (DocumentInterface *object, int x, int y, 
                            int width, int height, GError **error)
{
    int rx = width/2;
    int ry = height/2;
    return document_interface_ellipse_center (object, x+rx, y+ry, rx, ry, error);
}

gchar* 
document_interface_line (DocumentInterface *object, int x, int y, 
                              int x2, int y2, GError **error)
{
    Inkscape::XML::Node *newNode = dbus_create_node(object->desk, FALSE);
    std::stringstream out;
    printf("X2: %d\nY2 %d\n", x2, y2);
	out << "m " << x << "," << y << " " << x2 << "," << y2;
    printf ("PATH: %s\n", out.str().c_str());
    newNode->setAttribute("d", out.str().c_str());
    return finish_create_shape (object, error, newNode, (gchar *)"create line");
}

gchar* 
document_interface_spiral (DocumentInterface *object, int cx, int cy, 
                           int r, int revolutions, GError **error)
{
    Inkscape::XML::Node *newNode = dbus_create_node(object->desk, FALSE);
    newNode->setAttribute("sodipodi:type", "spiral");
    sp_repr_set_int(newNode, "sodipodi:cx", cx);
    sp_repr_set_int(newNode, "sodipodi:cy", cy);
    sp_repr_set_int(newNode, "sodipodi:radius", r);
    sp_repr_set_int(newNode, "sodipodi:revolution", revolutions);
    sp_repr_set_int(newNode, "sodipodi:t0", 0);
    sp_repr_set_int(newNode, "sodipodi:argument", 0);
    sp_repr_set_int(newNode, "sodipodi:expansion", 1);
    gchar * retval = finish_create_shape (object, error, newNode, (gchar *)"create spiral");
    newNode->setAttribute("style", "fill:none");
    return retval;
}

gboolean
document_interface_text (DocumentInterface *object, int x, int y, gchar *text, GError **error)
{
    //FIXME: Not selectable.

    SPDesktop *desktop = object->desk;
    SPCanvasText * canvas_text = (SPCanvasText *) sp_canvastext_new(sp_desktop_tempgroup(desktop), desktop, Geom::Point(0,0), "");
    sp_canvastext_set_text (canvas_text, text);
    sp_canvastext_set_coords (canvas_text, x, y);

    return TRUE;
}

gchar* 
document_interface_node (DocumentInterface *object, gchar *type, GError **error)
{
    SPDocument * doc = sp_desktop_document (object->desk);
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(doc);

    Inkscape::XML::Node *newNode =  xml_doc->createElement(type);

    object->desk->currentLayer()->appendChildRepr(newNode);
    object->desk->currentLayer()->updateRepr();

    if (object->updates)
        sp_document_done(sp_desktop_document(object->desk), 0, (gchar *)"created empty node");
    else
        document_interface_pause_updates(object, error);

    return strdup(newNode->attribute("id"));
}

/****************************************************************************
     ENVIORNMENT FUNCTIONS
****************************************************************************/
gdouble
document_interface_document_get_width (DocumentInterface *object)
{
    return sp_document_width(sp_desktop_document(object->desk));
}

gdouble
document_interface_document_get_height (DocumentInterface *object)
{
    return sp_document_height(sp_desktop_document(object->desk));
}

gchar *
document_interface_document_get_css (DocumentInterface *object, GError **error)
{
    SPCSSAttr *current = (object->desk)->current;
    return sp_repr_css_write_string(current);
}

gboolean 
document_interface_document_merge_css (DocumentInterface *object,
                                       gchar *stylestring, GError **error)
{
    SPCSSAttr * style = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string (style, stylestring);
    sp_desktop_set_style (object->desk, style);
    return TRUE;
}

gboolean 
document_interface_document_set_css (DocumentInterface *object,
                                     gchar *stylestring, GError **error)
{
    SPCSSAttr * style = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string (style, stylestring);
    //Memory leak?
    object->desk->current = style;
    return TRUE;
}

gboolean 
document_interface_document_resize_to_fit_selection (DocumentInterface *object,
                                                     GError **error)
{
    dbus_call_verb (object, SP_VERB_FIT_CANVAS_TO_SELECTION, error);
    return TRUE;
}

/****************************************************************************
     OBJECT FUNCTIONS
****************************************************************************/

gboolean
document_interface_set_attribute (DocumentInterface *object, char *shape, 
                                  char *attribute, char *newval, GError **error)
{
    Inkscape::XML::Node *newNode = get_object_by_name(object->desk, shape)->repr;

    /* ALTERNATIVE (is this faster?)
    Inkscape::XML::Node *newnode = sp_repr_lookup_name((doc->root)->repr, name);
    */
    if (newNode)
    {
        newNode->setAttribute(attribute, newval, TRUE);
        return TRUE;
    }
    return FALSE;
}

void 
document_interface_set_int_attribute (DocumentInterface *object, 
                                      char *shape, char *attribute, 
                                      int newval, GError **error)
{
    Inkscape::XML::Node *newNode = get_object_by_name (object->desk, shape)->repr;
    if (newNode)
        sp_repr_set_int (newNode, attribute, newval);
}


void 
document_interface_set_double_attribute (DocumentInterface *object, 
                                         char *shape, char *attribute, 
                                         double newval, GError **error)
{
    Inkscape::XML::Node *newNode = get_object_by_name (object->desk, shape)->repr;
    if (newNode)
        sp_repr_set_svg_double (newNode, attribute, newval);
}

gchar *
document_interface_get_attribute (DocumentInterface *object, char *shape, 
                                  char *attribute, GError **error)
{
    Inkscape::XML::Node *newNode = get_object_by_name(object->desk, shape)->repr;

    if (newNode)
        return g_strdup(newNode->attribute(attribute));
    return FALSE;
}

gboolean
document_interface_move (DocumentInterface *object, gchar *name, gdouble x, 
                         gdouble y, GError **error)
{
    const GSList *oldsel = selection_swap(object->desk, name);
    sp_selection_move (object->desk, x, 0 - y);
    selection_restore(object->desk, oldsel);
    return TRUE;
}

gboolean
document_interface_move_to (DocumentInterface *object, gchar *name, gdouble x, 
                         gdouble y, GError **error)
{
    const GSList *oldsel = selection_swap(object->desk, name);
    Inkscape::Selection * sel = sp_desktop_selection(object->desk);
    sp_selection_move (object->desk, x - selection_get_center_x(sel),
                                     0 - (y - selection_get_center_y(sel)));
    selection_restore(object->desk, oldsel);
    return TRUE;
}

gboolean
document_interface_object_to_path (DocumentInterface *object, 
                                   char *shape, GError **error)
{
    const GSList *oldsel = selection_swap(object->desk, shape);
    dbus_call_verb (object, SP_VERB_OBJECT_TO_CURVE, error);
    selection_restore(object->desk, oldsel);
    return TRUE;
}

gchar *
document_interface_get_path (DocumentInterface *object, char *pathname, GError **error)
{
    Inkscape::XML::Node *node = document_retrive_node (sp_desktop_document (object->desk), pathname);
    if (node == NULL || node->attribute("d") == NULL) {
        g_set_error(error, DBUS_GERROR, DBUS_GERROR_REMOTE_EXCEPTION, "Object is not a path or does not exist.");
        return FALSE;
    }
    return strdup(node->attribute("d"));
}

gboolean 
document_interface_transform (DocumentInterface *object, gchar *shape,
                              gchar *transformstr, GError **error)
{
    //FIXME: This should merge transformations.
    gchar trans[] = "transform";
    document_interface_set_attribute (object, shape, trans, transformstr, error);
    return TRUE;
}

gchar *
document_interface_get_css (DocumentInterface *object, gchar *shape,
                            GError **error)
{
    gchar style[] = "style";
    return document_interface_get_attribute (object, shape, style, error);
}

gboolean 
document_interface_modify_css (DocumentInterface *object, gchar *shape,
                               gchar *cssattrb, gchar *newval, GError **error)
{
    gchar style[] = "style";
    Inkscape::XML::Node *node = get_object_by_name(object->desk, shape)->repr;
    SPCSSAttr * oldstyle = sp_repr_css_attr (node, style);
    sp_repr_css_set_property(oldstyle, cssattrb, newval);
    node->setAttribute (style, sp_repr_css_write_string (oldstyle), TRUE);
    return TRUE;
}

gboolean 
document_interface_merge_css (DocumentInterface *object, gchar *shape,
                               gchar *stylestring, GError **error)
{
    SPCSSAttr * newstyle = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string (newstyle, stylestring);

    gchar style[] = "style";
    Inkscape::XML::Node *node = get_object_by_name(object->desk, shape)->repr;
    SPCSSAttr * oldstyle = sp_repr_css_attr (node, style);

    sp_repr_css_merge(oldstyle, newstyle);
    node->setAttribute (style, sp_repr_css_write_string (oldstyle), TRUE);
    return TRUE;
}

gboolean 
document_interface_move_to_layer (DocumentInterface *object, gchar *shape, 
                              gchar *layerstr, GError **error)
{
    const GSList *oldsel = selection_swap(object->desk, shape);
    document_interface_selection_move_to_layer(object, layerstr, error);
    selection_restore(object->desk, oldsel);
    return TRUE;
}

GArray *
document_interface_get_node_coordinates (DocumentInterface *object, gchar *shape)
{
    //FIXME: Needs lot's of work.

    Inkscape::XML::Node *shapenode = document_retrive_node (sp_desktop_document (object->desk), shape);
    if (shapenode == NULL || shapenode->attribute("d") == NULL) {
        return FALSE;
    }
    const char * path = strdup(shapenode->attribute("d"));
    printf("PATH: %s\n", path);
    
    Geom::parse_svg_path (path);
    return NULL;
}


/****************************************************************************
     FILE I/O FUNCTIONS
****************************************************************************/

gboolean 
document_interface_save (DocumentInterface *object, GError **error)
{
    SPDocument * doc = sp_desktop_document(object->desk);
    printf("1:  %s\n2:  %s\n3:  %s\n", doc->uri, doc->base, doc->name);
    if (doc->uri)
        return document_interface_save_as (object, doc->uri, error);
    return FALSE;
}

gboolean 
document_interface_load (DocumentInterface *object, 
                        gchar *filename, GError **error)
{
    desktop_ensure_active (object->desk);
    const Glib::ustring file(filename);
    sp_file_open(file, NULL, TRUE, TRUE);
    if (object->updates)
        sp_document_done(sp_desktop_document(object->desk), SP_VERB_FILE_OPEN, "Opened File");
    return TRUE;
}

gboolean 
document_interface_save_as (DocumentInterface *object, 
                           gchar *filename, GError **error)
{
    SPDocument * doc = sp_desktop_document(object->desk);
    #ifdef WITH_GNOME_VFS
    const Glib::ustring file(filename);
    return file_save_remote(doc, file, NULL, TRUE, TRUE);
    #endif
    if (!doc || strlen(filename)<1) //Safety check
        return false;

    try {
        Inkscape::Extension::save(NULL, doc, filename,
                 false, false, true);
    } catch (...) {
        //SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Document not saved."));
        return false;
    }

    //SP_ACTIVE_DESKTOP->event_log->rememberFileSave();
    //SP_ACTIVE_DESKTOP->messageStack()->flash(Inkscape::NORMAL_MESSAGE, "Document saved.");
    return true;
}
/*
gboolean 
document_interface_print_to_file (DocumentInterface *object, GError **error)
{
    SPDocument * doc = sp_desktop_document(object->desk);
    sp_print_document_to_file (doc, g_strdup("/home/soren/test.pdf"));
                               
    return TRUE;
}
*/
/****************************************************************************
     PROGRAM CONTROL FUNCTIONS
****************************************************************************/

gboolean
document_interface_close (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_FILE_CLOSE_VIEW, error);
}

gboolean
document_interface_exit (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_FILE_QUIT, error);
}

gboolean
document_interface_undo (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_EDIT_UNDO, error);
}

gboolean
document_interface_redo (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_EDIT_REDO, error);
}



/****************************************************************************
     UPDATE FUNCTIONS FIXME: test update system again.
****************************************************************************/

void
document_interface_pause_updates (DocumentInterface *object, GError **error)
{
    object->updates = FALSE;
    sp_desktop_document(object->desk)->root->uflags = FALSE;
    sp_desktop_document(object->desk)->root->mflags = FALSE;
}

void
document_interface_resume_updates (DocumentInterface *object, GError **error)
{
    object->updates = TRUE;
    sp_desktop_document(object->desk)->root->uflags = TRUE;
    sp_desktop_document(object->desk)->root->mflags = TRUE;
    //sp_desktop_document(object->desk)->_updateDocument();
    //FIXME: use better verb than rect.
    sp_document_done(sp_desktop_document(object->desk), SP_VERB_CONTEXT_RECT, "Multiple actions");
}

void
document_interface_update (DocumentInterface *object, GError **error)
{
    sp_desktop_document(object->desk)->root->uflags = TRUE;
    sp_desktop_document(object->desk)->root->mflags = TRUE;
    sp_desktop_document(object->desk)->_updateDocument();
    sp_desktop_document(object->desk)->root->uflags = FALSE;
    sp_desktop_document(object->desk)->root->mflags = FALSE;
    //sp_document_done(sp_desktop_document(object->desk), SP_VERB_CONTEXT_RECT, "Multiple actions");
}

/****************************************************************************
     SELECTION FUNCTIONS FIXME: use call_verb where appropriate (once update system is tested.)
****************************************************************************/

gboolean
document_interface_selection_get (DocumentInterface *object, char ***out, GError **error)
{
    Inkscape::Selection * sel = sp_desktop_selection(object->desk);
    GSList const *oldsel = sel->list();

    int size = g_slist_length((GSList *) oldsel);

    *out = g_new0 (char *, size + 1);

    int i = 0;
    for (GSList const *iter = oldsel; iter != NULL; iter = iter->next) {
        (*out)[i] = g_strdup(SP_OBJECT(iter->data)->repr->attribute("id"));
        i++;
    }
    (*out)[i] = NULL;

    return TRUE;
}

gboolean
document_interface_selection_add (DocumentInterface *object, char *name, GError **error)
{
    if (name == NULL) 
        return FALSE;
    Inkscape::Selection *selection = sp_desktop_selection(object->desk);

    selection->add(get_object_by_name(object->desk, name));
    return TRUE;
}

gboolean
document_interface_selection_add_list (DocumentInterface *object, 
                                       char **names, GError **error)
{
    int i;
    for (i=0;names[i] != NULL;i++) {
        document_interface_selection_add(object, names[i], error);       
    }
    return TRUE;
}

gboolean
document_interface_selection_set (DocumentInterface *object, char *name, GError **error)
{
    SPDocument * doc = sp_desktop_document (object->desk);
    Inkscape::Selection *selection = sp_desktop_selection(object->desk);
    selection->set(doc->getObjectById(name));
    return TRUE;
}

gboolean
document_interface_selection_set_list (DocumentInterface *object, 
                                       gchar **names, GError **error)
{
    sp_desktop_selection(object->desk)->clear();
    int i;
    for (i=0;names[i] != NULL;i++) {
        document_interface_selection_add(object, names[i], error);       
    }
    return TRUE;
}

gboolean
document_interface_selection_rotate (DocumentInterface *object, int angle, GError **error)
{
    Inkscape::Selection *selection = sp_desktop_selection(object->desk);
    sp_selection_rotate(selection, angle);
    return TRUE;
}

gboolean
document_interface_selection_delete (DocumentInterface *object, GError **error)
{
    sp_selection_delete (object->desk);
    return TRUE;
}

gboolean
document_interface_selection_clear (DocumentInterface *object, GError **error)
{
    sp_desktop_selection(object->desk)->clear();
    return TRUE;
}

gboolean
document_interface_select_all (DocumentInterface *object, GError **error)
{
    sp_edit_select_all (object->desk);
    return TRUE;
}

gboolean
document_interface_select_all_in_all_layers(DocumentInterface *object, 
                                            GError **error)
{
    sp_edit_select_all_in_all_layers (object->desk);
    return TRUE;
}

gboolean
document_interface_selection_box (DocumentInterface *object, int x, int y,
                                  int x2, int y2, gboolean replace, 
                                  GError **error)
{
    //FIXME: implement.
    return FALSE;
}

gboolean
document_interface_selection_invert (DocumentInterface *object, GError **error)
{
    sp_edit_invert (object->desk);
    return TRUE;
}

gboolean
document_interface_selection_group (DocumentInterface *object, GError **error)
{
    sp_selection_group (object->desk);
    return TRUE;
}
gboolean
document_interface_selection_ungroup (DocumentInterface *object, GError **error)
{
    sp_selection_ungroup (object->desk);
    return TRUE;
}
 
gboolean
document_interface_selection_cut (DocumentInterface *object, GError **error)
{
    sp_selection_cut (object->desk);
    return TRUE;
}
gboolean
document_interface_selection_copy (DocumentInterface *object, GError **error)
{
    desktop_ensure_active (object->desk);
    sp_selection_copy ();
    return TRUE;
}
gboolean
document_interface_selection_paste (DocumentInterface *object, GError **error)
{
    desktop_ensure_active (object->desk);
    sp_selection_paste (object->desk, TRUE);
    return TRUE;
}

gboolean
document_interface_selection_scale (DocumentInterface *object, gdouble grow, GError **error)
{
    Inkscape::Selection *selection = sp_desktop_selection(object->desk);
    sp_selection_scale (selection, grow);
    return TRUE;
}

gboolean
document_interface_selection_move (DocumentInterface *object, gdouble x, gdouble y, GError **error)
{
    sp_selection_move (object->desk, x, 0 - y); //switching coordinate systems.
    return TRUE;
}

gboolean
document_interface_selection_move_to (DocumentInterface *object, gdouble x, gdouble y, GError **error)
{
    Inkscape::Selection * sel = sp_desktop_selection(object->desk);

    Geom::OptRect sel_bbox = sel->bounds();
    if (sel_bbox) {
        Geom::Point m( x - selection_get_center_x(sel) , 0 - (y - selection_get_center_y(sel)) );
        sp_selection_move_relative(sel, m, true);
    }
    return TRUE;
}

//FIXME: does not paste in new layer.
gboolean 
document_interface_selection_move_to_layer (DocumentInterface *object,
                                            gchar *layerstr, GError **error)
{
    SPDesktop * dt = object->desk;

    Inkscape::Selection *selection = sp_desktop_selection(dt);

    // check if something is selected
    if (selection->isEmpty())
        return FALSE;

    SPObject *next = get_object_by_name(object->desk, layerstr);

    if (next && (strcmp("layer", (next->repr)->attribute("inkscape:groupmode")) == 0)) {

        sp_selection_cut(dt);

        dt->setCurrentLayer(next);

        sp_selection_paste(dt, TRUE);
        }
    return TRUE;
}

GArray *
document_interface_selection_get_center (DocumentInterface *object)
{
    Inkscape::Selection * sel = sp_desktop_selection(object->desk);

    if (sel) 
    {
        gdouble x = selection_get_center_x(sel);
        gdouble y = selection_get_center_y(sel);
        GArray * intArr = g_array_new (TRUE, TRUE, sizeof(double));

        g_array_append_val (intArr, x);
        g_array_append_val (intArr, y);
        return intArr;
    }

    return NULL;
}

gboolean 
document_interface_selection_to_path (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_OBJECT_TO_CURVE, error);    
}


gchar *
document_interface_selection_combine (DocumentInterface *object, gchar *cmd,
                                      GError **error)
{
    if (strcmp(cmd, "union") == 0)
        dbus_call_verb (object, SP_VERB_SELECTION_UNION, error);
    else if (strcmp(cmd, "intersection") == 0)
        dbus_call_verb (object, SP_VERB_SELECTION_INTERSECT, error);
    else if (strcmp(cmd, "difference") == 0)
        dbus_call_verb (object, SP_VERB_SELECTION_DIFF, error);
    else if (strcmp(cmd, "exclusion") == 0)
        dbus_call_verb (object, SP_VERB_SELECTION_SYMDIFF, error);
    else
        return NULL;

    if (sp_desktop_selection(object->desk)->singleRepr() != NULL)
        return g_strdup((sp_desktop_selection(object->desk)->singleRepr())->attribute("id"));
    return NULL;
}

gboolean
document_interface_selection_divide (DocumentInterface *object, char ***out, GError **error)
{
    dbus_call_verb (object, SP_VERB_SELECTION_CUT, error);

    return document_interface_selection_get (object, out, error);
}

gboolean
document_interface_selection_change_level (DocumentInterface *object, gchar *cmd,
                                      GError **error)
{
    if (strcmp(cmd, "raise") == 0)
        return dbus_call_verb (object, SP_VERB_SELECTION_RAISE, error);
    if (strcmp(cmd, "lower") == 0)
        return dbus_call_verb (object, SP_VERB_SELECTION_LOWER, error);
    if ((strcmp(cmd, "to_top") == 0) || (strcmp(cmd, "to_front") == 0))
        return dbus_call_verb (object, SP_VERB_SELECTION_TO_FRONT, error);
    if ((strcmp(cmd, "to_bottom") == 0) || (strcmp(cmd, "to_back") == 0))
        return dbus_call_verb (object, SP_VERB_SELECTION_TO_BACK, error);
    return TRUE;
}

/****************************************************************************
     LAYER FUNCTIONS
****************************************************************************/

gchar *
document_interface_layer_new (DocumentInterface *object, GError **error)
{
    SPDesktop * dt = object->desk;
    SPObject *new_layer = Inkscape::create_layer(dt->currentRoot(), dt->currentLayer(), Inkscape::LPOS_BELOW);
    dt->setCurrentLayer(new_layer);
    return g_strdup(get_name_from_object (new_layer));
}

gboolean 
document_interface_layer_set (DocumentInterface *object,
                              gchar *layerstr, GError **error)
{
    object->desk->setCurrentLayer (get_object_by_name (object->desk, layerstr));
    return TRUE;
}

gchar **
document_interface_layer_get_all (DocumentInterface *object)
{
    //FIXME: implement.
    return NULL;
}

gboolean 
document_interface_layer_change_level (DocumentInterface *object,
                                       gchar *cmd, GError **error)
{
    if (strcmp(cmd, "raise") == 0)
        return dbus_call_verb (object, SP_VERB_LAYER_RAISE, error);
    if (strcmp(cmd, "lower") == 0)
        return dbus_call_verb (object, SP_VERB_LAYER_LOWER, error);
    if ((strcmp(cmd, "to_top") == 0) || (strcmp(cmd, "to_front") == 0))
        return dbus_call_verb (object, SP_VERB_LAYER_TO_TOP, error);
    if ((strcmp(cmd, "to_bottom") == 0) || (strcmp(cmd, "to_back") == 0))
        return dbus_call_verb (object, SP_VERB_LAYER_TO_BOTTOM, error);
    return TRUE;
}

gboolean 
document_interface_layer_next (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_LAYER_NEXT, error);
}

gboolean 
document_interface_layer_previous (DocumentInterface *object, GError **error)
{
    return dbus_call_verb (object, SP_VERB_LAYER_PREV, error);
}






