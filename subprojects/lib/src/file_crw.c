/*

    File: file_crw.c

    Copyright (C) 1998-2005,2007-2008 Christophe GRENIER <grenier@cgsecurity.org>
  
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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_crw)
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdio.h>
#include "types.h"
#include "filegen.h"
#include "log.h"

/*@ requires valid_register_header_check(file_stat); */
static void register_header_check_crw(file_stat_t *file_stat);

const file_hint_t file_hint_crw= {
  .extension="crw",
  .description="Canon Raw picture",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=1,
  .register_header_check=&register_header_check_crw
};

/*@
  @ requires valid_file_check_param(file_recovery);
  @ ensures  valid_file_check_result(file_recovery);
  @ assigns  *file_recovery->handle, errno, file_recovery->file_size;
  @ assigns  Frama_C_entropy_source;
  @*/
static void file_check_crw(file_recovery_t *file_recovery)
{
  const unsigned char crw_footer[2]= { 0x0A, 0x30};
  file_search_footer(file_recovery, crw_footer, sizeof(crw_footer), 12);
}

/*@
  @ requires buffer_size >= 6+8;
  @ requires separation: \separated(&file_hint_crw, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_crw(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  if(((buffer[0]==0x49 && buffer[1]==0x49)||(buffer[0]==0x4D && buffer[1]==0x4D))
      && memcmp(buffer+6,"HEAPCCDR",8)==0)
  {
    reset_file_recovery(file_recovery_new);
    file_recovery_new->extension=file_hint_crw.extension;
    file_recovery_new->file_check=&file_check_crw;
    return 1;
  }
  return 0;
}

static void register_header_check_crw(file_stat_t *file_stat)
{
  static const unsigned char crw_header_be[2]= {'I','I'};
  static const unsigned char crw_header_le[2]= {'M','M'};
  register_header_check(0, crw_header_be, sizeof(crw_header_be), &header_check_crw, file_stat);
#ifndef __FRAMAC__
  register_header_check(0, crw_header_le, sizeof(crw_header_le), &header_check_crw, file_stat);
#endif
}
#endif
