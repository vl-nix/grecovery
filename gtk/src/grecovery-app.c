/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#include "grecovery-app.h"
#include "grecovery-win.h"

struct _GRecoveryApp
{
	GtkApplication  parent_instance;
};

G_DEFINE_TYPE ( GRecoveryApp, grecovery_app, GTK_TYPE_APPLICATION )

static void grecovery_app_activate ( GApplication *app )
{
	grecovery_win_new ( GRECOVERY_APP ( app ) );
}

static void grecovery_app_init ( G_GNUC_UNUSED GRecoveryApp *app )
{

}

static void grecovery_app_finalize ( GObject *object )
{
	G_OBJECT_CLASS ( grecovery_app_parent_class )->finalize ( object );
}

static void grecovery_app_class_init ( GRecoveryAppClass *class )
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	G_APPLICATION_CLASS (class)->activate = grecovery_app_activate;

	object_class->finalize = grecovery_app_finalize;
}

GRecoveryApp * grecovery_app_new ( void )
{
	return g_object_new ( GRECOVERY_TYPE_APP, "application-id", "org.gtk.grecovery", "flags", G_APPLICATION_NON_UNIQUE, NULL );
}

