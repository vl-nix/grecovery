/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#pragma once

#include <gtk/gtk.h>

enum res_n
{
	SG_SAVE,
	SG_RESET,
	SG_RESTORE,
	SG_ALL
};

#define FORMATS_TYPE_WIN formats_win_get_type ()

G_DECLARE_FINAL_TYPE ( FormatsWin, formats_win, FORMATS, WIN, GtkWindow )

FormatsWin * formats_win_new ( void );

void formats_win_treeview_append ( uint , gboolean , const char *, FormatsWin * );
