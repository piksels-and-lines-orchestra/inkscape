class SPDocumentUndo
{
	public:
		static void set_undo_sensitive(SPDocument *doc, bool sensitive);
		static bool get_undo_sensitive(SPDocument const *document);
		static void clear_undo(SPDocument *document);
		static void clear_redo(SPDocument *document);
		static void done(SPDocument *document, unsigned int event_type, Glib::ustring event_description);
		static void maybe_done(SPDocument *document, const gchar *keyconst, unsigned int event_type, Glib::ustring event_description);
		static void reset_key(Inkscape::Application *inkscape, SPDesktop *desktop, GtkObject *base);
		static void cancel(SPDocument *document);
		static gboolean undo(SPDocument *document);
		static gboolean redo(SPDocument *document);
};
