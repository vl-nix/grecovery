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

#define RECOVERY_TYPE_OBJECT recovery_get_type ()

G_DECLARE_FINAL_TYPE ( Recovery, recovery, RECOVERY, OBJECT, GObject )

Recovery * recovery_new ( void );

void recovery_formats_win ( Recovery * );

void recovery_disk_changed ( uint , Recovery * );
void recovery_set_list_disk ( GtkComboBoxText *, Recovery * );

void recovery_part_changed ( uint , uint , Recovery * );
void recovery_set_list_part ( GtkTreeView *, Recovery * );

void recovery_stop ( Recovery * );
void recovery_search ( uint8_t , uint8_t , const char *, GtkWindow *, Recovery * );
void recovery_search_update ( GtkLabel *, GtkProgressBar *, Recovery * );

gboolean recovery_done ( Recovery * );
gboolean recovery_add_disk ( const char *, Recovery * );

