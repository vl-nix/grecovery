/*
* Copyright 2021 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#pragma once

#include "grecovery-app.h"

#define GRECOVERY_TYPE_WIN grecovery_win_get_type ()

G_DECLARE_FINAL_TYPE ( GRecoveryWin, grecovery_win, GRECOVERY, WIN, GtkWindow )

GRecoveryWin * grecovery_win_new ( GRecoveryApp * );

