/*

    File: misc.h

    Copyright (C) 2008 Christophe GRENIER <grenier@cgsecurity.org>
  
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
#ifndef _MISC_H
#define _MISC_H
#ifdef __cplusplus
extern "C" {
#endif

/*@ ensures valid_read_string(\result); */
const char *get_os(void);

/*@ ensures valid_read_string(\result); */
const char *get_compiler(void);

#ifdef RECORD_COMPILATION_DATE
/*@ ensures valid_read_string(\result); */
const char *get_compilation_date(void);
#endif

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif
#endif
