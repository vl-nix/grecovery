/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#include "formats-win.h"

enum cols_n
{
	COL_NUM,
	COL_SET,
	COL_FTP,
	COL_ALL
};

struct _FormatsWin
{
	GtkWindow parent_instance;

	GtkTreeView *treeview;
};

G_DEFINE_TYPE ( FormatsWin, formats_win, GTK_TYPE_WINDOW )

void formats_win_treeview_append ( uint ind, gboolean set, const char *descr, FormatsWin *win )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( win->treeview );

	gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter );
	gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter,
				COL_NUM, ind,
				COL_SET, set,
				COL_FTP, descr,
				-1 );
}

static void formats_win_toggled ( G_GNUC_UNUSED GtkCellRendererToggle *toggle, char *path_str, FormatsWin *win )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( win->treeview );

	GtkTreePath *path = gtk_tree_path_new_from_string ( path_str );
	gtk_tree_model_get_iter ( model, &iter, path );

	gboolean toggle_item;
	gtk_tree_model_get ( model, &iter, COL_SET, &toggle_item, -1 );
	gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter, COL_SET, !toggle_item, -1 );

	gtk_tree_path_free ( path );
}

static GtkScrolledWindow * formats_win_create_treeview_scroll ( FormatsWin *win )
{
	GtkScrolledWindow *scroll = (GtkScrolledWindow *)gtk_scrolled_window_new ( NULL, NULL );
	gtk_scrolled_window_set_policy ( scroll, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	gtk_widget_set_visible ( GTK_WIDGET ( scroll ), TRUE );

	GtkListStore *store = gtk_list_store_new ( COL_ALL, G_TYPE_UINT, G_TYPE_BOOLEAN, G_TYPE_STRING );

	win->treeview = (GtkTreeView *)gtk_tree_view_new_with_model ( GTK_TREE_MODEL ( store ) );
	gtk_widget_set_visible ( GTK_WIDGET ( win->treeview ), TRUE );

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	struct Column { const char *name; const char *type; uint8_t num; } column_n[] =
	{
		{ "Num",     "text",   COL_NUM },
		{ "Set",   "active",   COL_SET },
		{ "Type",    "text",   COL_FTP }
	};

	uint8_t c = 0; for ( c = 0; c < COL_ALL; c++ )
	{
		if ( c == COL_SET )
			renderer = gtk_cell_renderer_toggle_new ();
		else
			renderer = gtk_cell_renderer_text_new ();

		if ( c == COL_SET ) g_signal_connect ( renderer, "toggled", G_CALLBACK ( formats_win_toggled ), win );

		column = gtk_tree_view_column_new_with_attributes ( column_n[c].name, renderer, column_n[c].type, column_n[c].num, NULL );
		gtk_tree_view_append_column ( win->treeview, column );
	}

	gtk_container_add ( GTK_CONTAINER ( scroll ), GTK_WIDGET ( win->treeview ) );
	g_object_unref ( G_OBJECT (store) );

	return scroll;
}

static void formats_win_save ( G_GNUC_UNUSED GtkButton *button, FormatsWin *win )
{
	g_signal_emit_by_name ( win, "formats-set-data", SG_SAVE, G_OBJECT ( win->treeview ) );

	gtk_widget_destroy ( GTK_WIDGET ( win ) );
}

static void formats_win_reset ( G_GNUC_UNUSED GtkButton *button, FormatsWin *win )
{
	g_signal_emit_by_name ( win, "formats-set-data", SG_RESET, G_OBJECT ( win->treeview ) );
}

static void formats_win_restore ( G_GNUC_UNUSED GtkButton *button, FormatsWin *win )
{
	g_signal_emit_by_name ( win, "formats-set-data", SG_RESTORE, G_OBJECT ( win->treeview ) );
}

static void formats_win_create_buttons ( GtkBox *h_box, FormatsWin *win )
{
	// const char *labels[] = { "Reset", "Restore", "Ok" };
	const char *labels[] = { "/gres/clear.png", "/gres/reload.png", "/gres/apply.png" };
	const void *funcs[]  = { formats_win_reset, formats_win_restore, formats_win_save };

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( labels ); c++ )
	{
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_resource ( labels[c], NULL );
		GdkPixbuf *pixbuf_scale = gdk_pixbuf_scale_simple ( pixbuf, 16, 16, GDK_INTERP_BILINEAR );
		GtkImage *image = (GtkImage *)gtk_image_new_from_pixbuf ( pixbuf_scale );

		GtkButton *button = (GtkButton *)gtk_button_new (); // gtk_button_new_with_label ( labels[c] );
		gtk_button_set_image ( button, GTK_WIDGET ( image ) );

		if ( pixbuf ) g_object_unref ( pixbuf );
		if ( pixbuf ) g_object_unref ( pixbuf_scale );

		g_signal_connect ( button, "clicked", G_CALLBACK ( funcs[c] ), win );

		gtk_widget_set_visible (  GTK_WIDGET ( button ), TRUE );
		gtk_box_pack_start ( h_box, GTK_WIDGET ( button  ), TRUE, TRUE, 0 );
	}
}

static void formats_win_create ( FormatsWin *win )
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_resource ( "/gres/recovery.png", NULL );

	GtkWindow *window = GTK_WINDOW ( win );
	// gtk_window_set_title ( window, "Formats" );
	gtk_window_set_default_size ( window, 500, 300 );
	gtk_window_set_icon ( window, pixbuf );

	if ( pixbuf ) g_object_unref ( pixbuf );

	GtkBox *v_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_box_set_spacing ( v_box, 10 );
	gtk_box_set_spacing ( h_box, 10 );

	gtk_widget_set_visible ( GTK_WIDGET ( v_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	gtk_box_pack_start ( v_box, GTK_WIDGET ( formats_win_create_treeview_scroll ( win ) ), TRUE, TRUE, 0 );

	formats_win_create_buttons ( h_box, win );

	gtk_box_pack_start ( v_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );

	gtk_container_set_border_width ( GTK_CONTAINER ( v_box ), 10 );
	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( v_box ) );

	gtk_window_present ( GTK_WINDOW ( win ) );
}

static void formats_win_init ( FormatsWin *win )
{
	formats_win_create ( win );
}

static void formats_win_finalize ( GObject *object )
{
	G_OBJECT_CLASS ( formats_win_parent_class )->finalize ( object );
}

static void formats_win_class_init ( FormatsWinClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = formats_win_finalize;

	g_signal_new ( "formats-set-data", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_OBJECT );
}

FormatsWin * formats_win_new ( void )
{
	FormatsWin *win = g_object_new ( FORMATS_TYPE_WIN, NULL );

	return win;
}
