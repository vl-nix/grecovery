/*

    File: file_lso.c

    Copyright (C) 2010 Christophe GRENIER <grenier@cgsecurity.org>
  
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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_lso)
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdio.h>
#include "types.h"
#include "filegen.h"

/*@ requires valid_register_header_check(file_stat); */
static void register_header_check_lso(file_stat_t *file_stat);

const file_hint_t file_hint_lso= {
  .extension="lso",
  .description="Logic Platinum File",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=1,
  .register_header_check=&register_header_check_lso
};

/*@
  @ requires file_recovery->file_check == &file_check_lso;
  @ requires valid_file_check_param(file_recovery);
  @ ensures  valid_file_check_result(file_recovery);
  @ assigns *file_recovery->handle, errno, file_recovery->file_size;
  @ assigns Frama_C_entropy_source;
  @*/
static void file_check_lso(file_recovery_t *file_recovery)
{
  const unsigned char lso_footer[6]= {0xFF, 0xFF, 0xFF, 0x7F, 0x7F, 0x7F};
  file_search_footer(file_recovery, lso_footer, sizeof(lso_footer), 0x46);
}

/*@
  @ requires separation: \separated(&file_hint_lso, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_lso(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension=file_hint_lso.extension;
  file_recovery_new->file_check=&file_check_lso;
  return 1;
}

static void register_header_check_lso(file_stat_t *file_stat)
{
  static const unsigned char lso_header[14]=  {
    0x13, 'G' , 0xc0, 0xab, 0x17, 0x05, 0x15, 0x00,
    0x03, 0x00, 0x24, 0x00, 0x24, 0x00
  };
  register_header_check(0, lso_header, sizeof(lso_header), &header_check_lso, file_stat);
}
#endif
