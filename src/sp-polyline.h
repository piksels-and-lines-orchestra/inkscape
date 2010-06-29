#ifndef SP_POLYLINE_H
#define SP_POLYLINE_H

#include "sp-shape.h"



#define SP_TYPE_POLYLINE            (SPPolyLine::sp_polyline_get_type ())
#define SP_POLYLINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_POLYLINE, SPPolyLine))
#define SP_POLYLINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_POLYLINE, SPPolyLineClass))
#define SP_IS_POLYLINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_POLYLINE))
#define SP_IS_POLYLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_POLYLINE))

class SPPolyLine;
class SPPolyLineClass;

class SPPolyLine : public SPShape {
	public:
		static GType sp_polyline_get_type (void);
	private:
		static void sp_polyline_init (SPPolyLine *polyline);

		static void sp_polyline_build (SPObject * object, SPDocument * document, Inkscape::XML::Node * repr);
		static void sp_polyline_set (SPObject *object, unsigned int key, const gchar *value);
		static Inkscape::XML::Node *sp_polyline_write (SPObject *object, Inkscape::XML::Document *doc, Inkscape::XML::Node *repr, guint flags);

		static gchar * sp_polyline_description (SPItem * item);

	friend class SPPolyLineClass;	

};

class SPPolyLineClass {
	public:
		SPShapeClass parent_class;
	private:
		static SPShapeClass *static_parent_class;
		static void sp_polyline_class_init (SPPolyLineClass *klass);

	friend class SPPolyLine;	
};




#endif
