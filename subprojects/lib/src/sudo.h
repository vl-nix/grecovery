/*

    File: sudo.h

    Copyright (C) 2007 Christophe GRENIER <grenier@cgsecurity.org>

    This software is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write the Free Software Foundation, Inc., 51
    Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */
#ifdef SUDO_BIN
#ifndef _SUDO_H
#define _SUDO_H
#ifdef __cplusplus
extern "C" {
#endif

void run_sudo(const int argc, char **argv, const int create_log);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif
#endif
#endif
