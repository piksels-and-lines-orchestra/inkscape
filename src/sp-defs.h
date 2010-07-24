#ifndef SEEN_SP_DEFS_H
#define SEEN_SP_DEFS_H

/*
 * SVG <defs> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000-2002 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"

#define SP_TYPE_DEFS            (SPDefs::sp_defs_get_type())
#define SP_DEFS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), SP_TYPE_DEFS, SPDefs))
#define SP_DEFS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), SP_TYPE_DEFS, SPDefsClass))
#define SP_IS_DEFS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), SP_TYPE_DEFS))
#define SP_IS_DEFS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), SP_TYPE_DEFS))

class SPDefs : public SPObject {
	public:
		static GType sp_defs_get_type(void);
	
	private:
		static void sp_defs_init(SPDefs *defs);
		static void sp_defs_release(SPObject *object);
		static void sp_defs_update(SPObject *object, SPCtx *ctx, guint flags);
		static void sp_defs_modified(SPObject *object, guint flags);
		static Inkscape::XML::Node *sp_defs_write(SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);

	friend class SPDefsClass;	
};

class SPDefsClass {
	public:
		SPObjectClass parent_class;

	private:
		static void sp_defs_class_init(SPDefsClass *dc);
		static SPObjectClass *static_parent_class;

	friend class SPDefs;	
};

//GType sp_defs_get_type(void);


#endif /* !SEEN_SP_DEFS_H */

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
