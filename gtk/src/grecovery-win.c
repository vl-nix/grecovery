/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#include "grecovery-win.h"
#include "recovery.h"

enum cols_n
{
	COL_NUM,
	COL_FLG,
	COL_FTP,
	COL_SYS,
	COL_FSZ,
	COL_LBL,
	NUM_COLS
};

struct _GRecoveryWin
{
	GtkWindow parent_instance;

	GtkEntry *entry_save;
	GtkTreeView *treeview;
	GtkTreeView *treeview_file;
	GtkTreeSelection *selection;

	GtkComboBoxText *combo_disk;
	GtkComboBoxText *combo_type;
	GtkComboBoxText *combo_whole;

	GtkSwitch *gswitch;
	GtkButton *button_rec;
	GtkTreeViewColumn *column_sector;
	GtkTreeViewColumn *column_toggled;

	GtkLabel *prg_label;
	GtkProgressBar *prg_bar;

	Recovery *recovery;

	uint src_update;
	ulong disk_signal_id;
	ulong part_signal_id;
};

G_DEFINE_TYPE ( GRecoveryWin, grecovery_win, GTK_TYPE_WINDOW )

static void grecovery_win_about ( GtkWindow *window )
{
	GtkAboutDialog *dialog = (GtkAboutDialog *)gtk_about_dialog_new ();
	gtk_window_set_transient_for ( GTK_WINDOW ( dialog ), window );

	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_resource ( "/gres/recovery.png", NULL );

	gtk_about_dialog_set_logo ( dialog, pixbuf );
	gtk_window_set_icon ( GTK_WINDOW ( dialog ), pixbuf );

	if ( pixbuf ) g_object_unref ( pixbuf );

	// Gui authors
	const char *authors[] = { "Stepan Perun", " ", NULL };

	// PhotoRec authors
	const char *photorec[]  = { "Christophe Grenier", " ", NULL };

	// All sources authors
	const char *authors_all[]  = { "Gary S. Brown", "Hans Reiser", "Ingo Molnar", "Gadi Oxman", "Klaus Halfmann", "Anton Altaparmakov", "Lode Leroy", "Richard Russon", "Tomasz Kojm", "Peter Turczak", "Simson Garfinkel", "Nick Schrader", "James Holodnak", "Dmitry Brant", "Free Software Foundation Inc.", "International Business Machines Corp.", " ", NULL };

	gtk_about_dialog_add_credit_section ( dialog, "PhotoRec", photorec );
	gtk_about_dialog_add_credit_section ( dialog, "All sources", authors_all );

	gtk_about_dialog_set_program_name ( dialog, "GRecovery" );
	gtk_about_dialog_set_version ( dialog, "21.8" );
	gtk_about_dialog_set_license_type ( dialog, GTK_LICENSE_GPL_2_0 );
	gtk_about_dialog_set_authors ( dialog, authors );
	gtk_about_dialog_set_website ( dialog,   "https://github.com/vl-nix/grecovery" );
	gtk_about_dialog_set_copyright ( dialog, "Copyright 2021 GRecovery" );
	gtk_about_dialog_set_comments  ( dialog, "Data Recovery Utility\nBased in PhotoRec" );

	gtk_dialog_run ( GTK_DIALOG (dialog) );
	gtk_widget_destroy ( GTK_WIDGET (dialog) );
}

static void grecovery_win_message_dialog ( const char *f_error, const char *file_or_info, GtkMessageType mesg_type, GtkWindow *window )
{
	GtkMessageDialog *dialog = ( GtkMessageDialog *)gtk_message_dialog_new (
					window,    GTK_DIALOG_MODAL,
					mesg_type, GTK_BUTTONS_CLOSE,
					"%s\n%s",  f_error, file_or_info );

	gtk_dialog_run     ( GTK_DIALOG ( dialog ) );
	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}

static void grecovery_win_stop ( GRecoveryWin *win )
{
	win->src_update = 0;

	gtk_widget_set_sensitive ( GTK_WIDGET ( win->combo_disk  ), TRUE );
	gtk_widget_set_sensitive ( GTK_WIDGET ( win->treeview    ), TRUE );
	gtk_widget_set_sensitive ( GTK_WIDGET ( win->combo_type  ), TRUE );
	gtk_widget_set_sensitive ( GTK_WIDGET ( win->combo_whole ), TRUE );

	recovery_search_update ( win->prg_label, win->prg_bar, win->recovery );
}

static gboolean grecovery_win_timeout_update ( GRecoveryWin *win )
{
	recovery_search_update ( win->prg_label, win->prg_bar, win->recovery );

	if ( recovery_done ( win->recovery ) ) { grecovery_win_stop ( win ); return FALSE; }

	return TRUE;
}

static void grecovery_win_search ( GRecoveryWin *win )
{
	const char *dir = gtk_entry_get_text ( win->entry_save );

	uint8_t type  = (uint8_t)gtk_combo_box_get_active ( GTK_COMBO_BOX ( win->combo_type ) );
	uint8_t whole = (uint8_t)gtk_combo_box_get_active ( GTK_COMBO_BOX ( win->combo_whole ) );

	if ( dir && !g_file_test ( dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR ) )
		{ grecovery_win_message_dialog ( "Dir not found.", dir, GTK_MESSAGE_WARNING, GTK_WINDOW ( win ) ); return; }

	if ( gtk_widget_get_sensitive ( GTK_WIDGET ( win->combo_disk ) ) )
	{
		gtk_label_set_text ( win->prg_label, " " );
		gtk_progress_bar_set_fraction ( win->prg_bar, 0 );
		gtk_list_store_clear ( GTK_LIST_STORE ( gtk_tree_view_get_model ( win->treeview_file ) ) );
	}

	gtk_widget_set_sensitive ( GTK_WIDGET ( win->combo_disk  ), FALSE );
	gtk_widget_set_sensitive ( GTK_WIDGET ( win->treeview    ), FALSE );
	gtk_widget_set_sensitive ( GTK_WIDGET ( win->combo_type  ), FALSE );
	gtk_widget_set_sensitive ( GTK_WIDGET ( win->combo_whole ), FALSE );

	gboolean search_only = gtk_switch_get_state ( win->gswitch );

	win->src_update = g_timeout_add ( 250, (GSourceFunc)grecovery_win_timeout_update, win );

	recovery_search ( type, whole, dir, search_only, GTK_WINDOW ( win ), win->recovery );
}

static char * grecovery_win_file_open ( const char *path, const char *accept, const char *icon, uint8_t num, GtkWindow *window )
{
	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
		" ", window, num, "gtk-cancel", GTK_RESPONSE_CANCEL, accept, GTK_RESPONSE_ACCEPT, NULL );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), icon );

	gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ), path );

	gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER ( dialog ), FALSE );

	char *filename = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return filename;
}

static void grecovery_win_signal_file_open ( GtkEntry *entry, GtkEntryIconPosition icon_pos, G_GNUC_UNUSED GdkEventButton *event, GRecoveryWin *win )
{
	if ( icon_pos == GTK_ENTRY_ICON_SECONDARY )
	{
		g_autofree char *file = grecovery_win_file_open ( g_get_home_dir (), "gtk-open", "document-open", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_WINDOW ( win ) );

		if ( file ) gtk_entry_set_text ( win->entry_save, file );
	}
}

static void grecovery_win_disk_set ( GRecoveryWin *win )
{
	g_signal_handler_block ( win->combo_disk, win->disk_signal_id );
	g_signal_handler_block ( win->selection,  win->part_signal_id );

	recovery_set_list_disk ( win->combo_disk, win->recovery );
	recovery_set_list_part ( win->treeview,   win->recovery );

	g_signal_handler_unblock ( win->combo_disk, win->disk_signal_id );
	g_signal_handler_unblock ( win->selection,  win->part_signal_id );
}

static void grecovery_win_signal_combo_disk_changed ( GtkComboBoxText *combo, GRecoveryWin *win )
{
	uint act = (uint)gtk_combo_box_get_active ( GTK_COMBO_BOX ( combo ) );
	g_autofree char *text = gtk_combo_box_text_get_active_text ( combo );

	if ( text && g_str_has_prefix ( "Add disk", text ) )
	{
		g_autofree char *file = grecovery_win_file_open ( g_get_home_dir (), "gtk-open", "document-open", GTK_FILE_CHOOSER_ACTION_OPEN, GTK_WINDOW ( win ) );

		if ( !file ) return;
		if ( !recovery_add_disk ( file, win->recovery ) ) return;

		grecovery_win_disk_set ( win );
	}
	else
	{
		recovery_disk_changed ( act, win->recovery );
		recovery_set_list_part ( win->treeview, win->recovery );
	}
}

static void grecovery_win_signal_tree_part_changed ( GtkTreeSelection *selection, GRecoveryWin *win )
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	if ( gtk_tree_selection_get_selected ( selection, &model, &iter ) )
	{
		uint order = 0;
		gtk_tree_model_get ( model, &iter, COL_NUM, &order, -1 );

		g_autofree char *n_str = gtk_tree_model_get_string_from_iter ( model, &iter );

		recovery_part_changed ( ( uint )atoi ( n_str ), order, win->recovery );
	}
}

static GtkScrolledWindow * grecovery_win_create_treeview_scroll ( GRecoveryWin *win )
{
	GtkScrolledWindow *scroll = (GtkScrolledWindow *)gtk_scrolled_window_new ( NULL, NULL );
	gtk_scrolled_window_set_policy ( scroll, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	gtk_widget_set_visible ( GTK_WIDGET ( scroll ), TRUE );

	GtkListStore *store = gtk_list_store_new ( NUM_COLS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );

	win->treeview = (GtkTreeView *)gtk_tree_view_new_with_model ( GTK_TREE_MODEL ( store ) );
	gtk_widget_set_visible ( GTK_WIDGET ( win->treeview ), TRUE );

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	struct Column { const char *name; const char *type; uint8_t num; } column_n[] =
	{
		{ "O",       "text",   COL_NUM },
		{ "F",       "text",   COL_FLG },
		{ "Type",    "text",   COL_FTP },
		{ "System",  "text",   COL_SYS },
		{ "Size",    "text",   COL_FSZ },
		{ "Label",   "text",   COL_LBL }
	};

	uint8_t c = 0; for ( c = 0; c < NUM_COLS; c++ )
	{
		renderer = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes ( column_n[c].name, renderer, column_n[c].type, column_n[c].num, NULL );
		gtk_tree_view_append_column ( win->treeview, column );
	}

	gtk_container_add ( GTK_CONTAINER ( scroll ), GTK_WIDGET ( win->treeview ) );
	g_object_unref ( G_OBJECT (store) );

	win->selection = (GtkTreeSelection *)gtk_tree_view_get_selection ( win->treeview );
	win->part_signal_id = g_signal_connect ( win->selection, "changed", G_CALLBACK ( grecovery_win_signal_tree_part_changed ), win );

	return scroll;
}

static void grecovery_win_treeview_file_append ( uint ind, gboolean set, const char *file, uint64_t sz, uint64_t sector, GRecoveryWin *win )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( win->treeview_file );

	char path_str[24];
	sprintf ( path_str, "%u", ind );

	if ( gtk_tree_model_get_iter_from_string ( model, &iter, path_str ) )
	{
		gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter, 0, ind + 1, 1, set, 2, file, 3, sz, 4, sector, -1 );
	}
	else
	{
		gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter );
		gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter, 0, ind + 1, 1, set, 2, file, 3, sz, 4, sector, -1 );
	}
}

static void grecovery_win_click_sector ( G_GNUC_UNUSED GtkButton *button, GRecoveryWin *win )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( win->treeview_file );

	gboolean valid = FALSE;
	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid; valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		gboolean toggle_item;
		gtk_tree_model_get ( model, &iter, 1, &toggle_item, -1 );

		if ( toggle_item )
		{
			uint ind = 0;
			char *file = NULL;
			uint64_t sz = 0, sector = 0;

			gtk_tree_model_get ( model, &iter, 0, &ind, 2, &file, 3, &sz, 4, &sector, -1 );

			recovery_file_sector ( ind-1, file, sz, sector, win->recovery );

			free ( file );
		}
	}
}

static void grecovery_win_toggled ( G_GNUC_UNUSED GtkCellRendererToggle *toggle, char *path_str, GRecoveryWin *win )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( win->treeview_file );

	GtkTreePath *path = gtk_tree_path_new_from_string ( path_str );
	gtk_tree_model_get_iter ( model, &iter, path );

	gboolean toggle_item;
	gtk_tree_model_get ( model, &iter, 1, &toggle_item, -1 );
	gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter, 1, !toggle_item, -1 );

	gtk_tree_path_free ( path );
}

static GtkBox * grecovery_win_create_treeview_file_scroll ( GRecoveryWin *win )
{
	GtkBox *v_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_widget_set_visible ( GTK_WIDGET ( v_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	GtkScrolledWindow *scroll = (GtkScrolledWindow *)gtk_scrolled_window_new ( NULL, NULL );
	gtk_scrolled_window_set_min_content_height ( scroll, 150 );
	gtk_scrolled_window_set_policy ( scroll, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	gtk_widget_set_visible ( GTK_WIDGET ( scroll ), TRUE );

	GtkListStore *store = gtk_list_store_new ( 5, G_TYPE_UINT, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64 );

	win->treeview_file = (GtkTreeView *)gtk_tree_view_new_with_model ( GTK_TREE_MODEL ( store ) );
	gtk_widget_set_visible ( GTK_WIDGET ( win->treeview_file ), TRUE );

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	struct Column { const char *name; const char *type; uint8_t num; } column_n[] =
	{
		{ "Num",     "text",   0 },
		{ "Set",   "active",   1 },
		{ "File",    "text",   2 },
		{ "Size",    "text",   3 },
		{ "Sector",  "text",   4 }
	};

	uint8_t c = 0; for ( c = 0; c < 5; c++ )
	{
		if ( c == 1 )
			renderer = gtk_cell_renderer_toggle_new ();
		else
			renderer = gtk_cell_renderer_text_new ();

		if ( c == 1 ) g_signal_connect ( renderer, "toggled", G_CALLBACK ( grecovery_win_toggled ), win );

		column = gtk_tree_view_column_new_with_attributes ( column_n[c].name, renderer, column_n[c].type, column_n[c].num, NULL );
		gtk_tree_view_append_column ( win->treeview_file, column );

		if ( c == 1 ) { win->column_toggled = column; gtk_tree_view_column_set_visible ( column, FALSE ); }
		if ( c == 4 ) { win->column_sector  = column; gtk_tree_view_column_set_visible ( column, FALSE ); }
	}

	gtk_container_add ( GTK_CONTAINER ( scroll ), GTK_WIDGET ( win->treeview_file ) );
	g_object_unref ( G_OBJECT (store) );

	win->button_rec = (GtkButton *)gtk_button_new_with_label ( "Recovery" );
	gtk_widget_set_visible ( GTK_WIDGET ( win->button_rec ), FALSE );
	g_signal_connect ( win->button_rec, "clicked", G_CALLBACK ( grecovery_win_click_sector ), win );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( win->button_rec ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( v_box, GTK_WIDGET ( scroll ), FALSE, FALSE, 0 );
	gtk_box_pack_end ( v_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );

	return v_box;
}

static void grecovery_win_srch ( G_GNUC_UNUSED GtkButton *button, GRecoveryWin *win )
{
	grecovery_win_search ( win );
}

static void grecovery_win_info ( G_GNUC_UNUSED GtkButton *button, GRecoveryWin *win )
{
	grecovery_win_about ( GTK_WINDOW ( win ) );
}

static void grecovery_win_frmt ( G_GNUC_UNUSED GtkButton *button, GRecoveryWin *win )
{
	recovery_formats_win ( win->recovery );
}

static void grecovery_win_quit ( G_GNUC_UNUSED GtkButton *button, GRecoveryWin *win )
{
	gtk_widget_destroy ( GTK_WIDGET ( win ) );
}

static void grecovery_win_create_butttons ( GtkBox *h_box, GRecoveryWin *win )
{
	// const char *labels[] = { "🛈", "🗐", "🔎", "⏻" };
	const char *label_prg[] = { "/gres/info.png", "/gres/image.png", "/gres/search.png", "/gres/close.png" };
	const void *funcs[] = { grecovery_win_info, grecovery_win_frmt, grecovery_win_srch, grecovery_win_quit };

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( label_prg ); c++ )
	{
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_resource ( label_prg[c], NULL );
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

static GtkComboBoxText * grecovery_win_create_combo ( const char *text_a, const char *text_b, GRecoveryWin *win )
{
	GtkComboBoxText *combo = (GtkComboBoxText *) gtk_combo_box_text_new ();

	gtk_combo_box_text_append_text ( combo, text_a );
	gtk_combo_box_text_append_text ( combo, text_b ); 
	gtk_combo_box_set_active ( GTK_COMBO_BOX ( combo ), 0 );

	gtk_widget_set_visible ( GTK_WIDGET ( combo ), TRUE );

	return combo;
}

static void grecovery_win_switch_active ( GObject *gobject, G_GNUC_UNUSED GParamSpec *pspec, GRecoveryWin *win )
{
	gboolean state = gtk_switch_get_state ( GTK_SWITCH ( gobject ) );

	gtk_tree_view_column_set_visible ( win->column_toggled, state );
	gtk_tree_view_column_set_visible ( win->column_sector,  state );
}

static void grecovery_win_expander_expanded ( GtkExpander *expander, G_GNUC_UNUSED GParamSpec *pspec, GRecoveryWin *win )
{
	gtk_widget_set_visible ( GTK_WIDGET ( win->gswitch ), !gtk_expander_get_expanded ( expander ) );
	gtk_widget_set_visible ( GTK_WIDGET ( win->button_rec ), gtk_switch_get_state ( win->gswitch ) );
}

static void grecovery_win_handler_set_file ( Recovery *recovery, uint8_t num, const char *file, uint64_t sz, uint64_t sector, GRecoveryWin *win )
{
	g_autofree char *name = g_path_get_basename ( file );

	grecovery_win_treeview_file_append ( num, FALSE, name, sz, sector, win );
}

static void grecovery_win_create ( GRecoveryWin *win )
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_resource ( "/gres/recovery.png", NULL );

	GtkWindow *window = GTK_WINDOW ( win );
	gtk_window_set_title ( window, "GRecovery" );
	gtk_window_set_default_size ( window, 400, 500 );
	gtk_window_set_icon ( window, pixbuf );

	if ( pixbuf ) g_object_unref ( pixbuf );

	GtkBox *v_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_box_set_spacing ( v_box, 5 );
	gtk_box_set_spacing ( h_box, 5 );

	gtk_widget_set_margin_top    ( GTK_WIDGET ( v_box ), 10 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( v_box ), 10 );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( v_box ), 10 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( v_box ), 10 );

	gtk_widget_set_visible ( GTK_WIDGET ( v_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	win->combo_disk = grecovery_win_create_combo ( "None", "Add disk", win );
	win->disk_signal_id = g_signal_connect ( win->combo_disk, "changed", G_CALLBACK ( grecovery_win_signal_combo_disk_changed ), win );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( win->combo_disk ), FALSE, FALSE, 0 );

	gtk_box_pack_start ( v_box, GTK_WIDGET ( grecovery_win_create_treeview_scroll ( win ) ), TRUE, TRUE, 0 );

	win->gswitch = (GtkSwitch *)gtk_switch_new ();
	gtk_widget_set_sensitive ( GTK_WIDGET ( win->gswitch  ), FALSE );

	gtk_widget_set_visible ( GTK_WIDGET ( win->gswitch ), TRUE );
	gtk_widget_set_tooltip_text ( GTK_WIDGET ( win->gswitch ), "Only Search" );
	g_signal_connect ( win->gswitch, "notify::active", G_CALLBACK ( grecovery_win_switch_active ), win );

	GtkExpander *expander = (GtkExpander *)gtk_expander_new ( "Details:" );
	g_signal_connect ( expander, "notify::expanded", G_CALLBACK ( grecovery_win_expander_expanded ), win );

	gtk_widget_set_visible ( GTK_WIDGET ( expander ), TRUE );
	gtk_container_add ( GTK_CONTAINER ( expander ), GTK_WIDGET ( grecovery_win_create_treeview_file_scroll ( win ) ) );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( expander ), TRUE, TRUE, 0 );
	gtk_box_pack_end   ( h_box, GTK_WIDGET ( win->gswitch ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );

	win->prg_label = (GtkLabel *)gtk_label_new ( " " );
	gtk_widget_set_halign ( GTK_WIDGET ( win->prg_label ), GTK_ALIGN_START );

	win->prg_bar   = (GtkProgressBar *)gtk_progress_bar_new ();

	gtk_widget_set_visible ( GTK_WIDGET ( win->prg_label ), TRUE );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( win->prg_label ), FALSE, FALSE, 0 );

	gtk_widget_set_visible ( GTK_WIDGET ( win->prg_bar ), TRUE );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( win->prg_bar ), FALSE, FALSE, 0 );

	win->combo_type = grecovery_win_create_combo ( "FAT / NTFS ...", "Ext2 - Ext4", win );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( win->combo_type ), FALSE, FALSE, 0 );

	win->combo_whole = grecovery_win_create_combo ( "Whole", "Free", win );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( win->combo_whole ), FALSE, FALSE, 0 );

	win->entry_save = (GtkEntry *)gtk_entry_new ();
	gtk_entry_set_text ( win->entry_save, "Recovery ..." );
	g_object_set ( win->entry_save, "editable", FALSE, NULL );
	gtk_entry_set_icon_from_icon_name ( win->entry_save, GTK_ENTRY_ICON_SECONDARY, "folder" );
	g_signal_connect ( win->entry_save, "icon-press", G_CALLBACK ( grecovery_win_signal_file_open ), win );

	gtk_widget_set_visible ( GTK_WIDGET ( win->entry_save ), TRUE );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( win->entry_save ), FALSE, FALSE, 0 );

	h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );
	gtk_box_set_spacing ( h_box, 5 );

	grecovery_win_create_butttons ( h_box, win );
	gtk_box_pack_end ( v_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );

	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( v_box ) );

	gtk_window_present ( GTK_WINDOW ( win ) );
}

static void grecovery_win_init ( GRecoveryWin *win )
{
	win->src_update = 0;

	win->recovery = recovery_new ();
	g_signal_connect ( win->recovery, "recovery-set-file", G_CALLBACK ( grecovery_win_handler_set_file ), win );

	grecovery_win_create ( win );
	grecovery_win_disk_set ( win );
}

static void grecovery_win_finalize ( GObject *object )
{
	GRecoveryWin *win = GRECOVERY_WIN ( object );

	if ( win->src_update ) g_source_remove ( win->src_update );

	recovery_stop ( win->recovery );

	g_object_unref ( win->recovery );

	G_OBJECT_CLASS ( grecovery_win_parent_class )->finalize ( object );
}

static void grecovery_win_class_init ( GRecoveryWinClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = grecovery_win_finalize;
}

GRecoveryWin * grecovery_win_new ( GRecoveryApp *app )
{
	return g_object_new ( GRECOVERY_TYPE_WIN, "application", app, NULL );
}
