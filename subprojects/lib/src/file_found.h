/*

    File: file_found.h

    Copyright (C) 2009 Christophe GRENIER <grenier@cgsecurity.org>
  
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
#ifndef _FILE_FOUND_H
#define _FILE_FOUND_H
#ifdef __cplusplus
extern "C" {
#endif

/*@
  @ requires \valid(current_search_space);
  @ requires \valid(file_stat);
  @ requires \separated(current_search_space, file_stat);
  @*/
alloc_data_t *file_found(alloc_data_t *current_search_space, const uint64_t offset, file_stat_t *file_stat);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif
#endif
