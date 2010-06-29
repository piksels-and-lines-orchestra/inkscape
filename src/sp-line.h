#ifndef __SP_LINE_H__
#define __SP_LINE_H__

/*
 * SVG <line> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "svg/svg-length.h"
#include "sp-shape.h"



#define SP_TYPE_LINE            (SPLine::sp_line_get_type ())
#define SP_LINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_LINE, SPLine))
#define SP_LINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_LINE, SPLineClass))
#define SP_IS_LINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_LINE))
#define SP_IS_LINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_LINE))

class SPLine;
class SPLineClass;

//static void sp_line_class_init (SPLineClass *klass);

class SPLine : public SPShape {
	public:
		SVGLength x1;
		SVGLength y1;
		SVGLength x2;
		SVGLength y2;
		static GType sp_line_get_type (void);
	private:
		static void sp_line_init (SPLine *line);

		static void sp_line_build (SPObject * object, SPDocument * document, Inkscape::XML::Node * repr);
		static void sp_line_set (SPObject *object, unsigned int key, const gchar *value);
		static Inkscape::XML::Node *sp_line_write (SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);

		static gchar *sp_line_description (SPItem * item);
		static Geom::Matrix sp_line_set_transform(SPItem *item, Geom::Matrix const &xform);

		static void sp_line_update (SPObject *object, SPCtx *ctx, guint flags);
		static void sp_line_set_shape (SPShape *shape);
		static void sp_line_convert_to_guides(SPItem *item);

		friend class SPLineClass;
};

class SPLineClass {
	public:
		SPShapeClass parent_class;
	private:
		static SPShapeClass *static_parent_class;
		static void sp_line_class_init (SPLineClass *klass);
	
	friend class SPLine;
};

//GType sp_line_get_type (void);



#endif
