#ifndef __SP_OBJECTGROUP_H__
#define __SP_OBJECTGROUP_H__

/*
 * Abstract base class for non-item groups
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"

#define SP_TYPE_OBJECTGROUP (SPObjectGroup::sp_objectgroup_get_type ())
#define SP_OBJECTGROUP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_OBJECTGROUP, SPObjectGroup))
#define SP_OBJECTGROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_OBJECTGROUP, SPObjectGroupClass))
#define SP_IS_OBJECTGROUP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_OBJECTGROUP))
#define SP_IS_OBJECTGROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_OBJECTGROUP))

class SPObjectGroup : public SPObject {
	public:
		static GType sp_objectgroup_get_type (void);

	private:
		static void sp_objectgroup_init (SPObjectGroup *objectgroup);

		static void sp_objectgroup_child_added (SPObject * object, Inkscape::XML::Node * child, Inkscape::XML::Node * ref);
		static void sp_objectgroup_remove_child (SPObject * object, Inkscape::XML::Node * child);
		static void sp_objectgroup_order_changed (SPObject * object, Inkscape::XML::Node * child, Inkscape::XML::Node * old_ref, Inkscape::XML::Node * new_ref);
		static Inkscape::XML::Node *sp_objectgroup_write (SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);

	friend class SPObjectGroupClass;	
};

class SPObjectGroupClass {
	public:
		SPObjectClass parent_class;

	private:
		static void sp_objectgroup_class_init (SPObjectGroupClass *klass);
		static SPObjectClass *static_parent_class;

	friend class SPObjectGroup;	
};

//GType sp_objectgroup_get_type (void);

#endif
