/*

    File: file_gp5.c

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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_gp5)
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
static void register_header_check_gp5(file_stat_t *file_stat);

const file_hint_t file_hint_gp5= {
  .extension="gp5",
  .description="Guitar Pro 4 & 5",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=1,
  .register_header_check=&register_header_check_gp5
};

/*@
  @ requires file_recovery->file_check == &file_check_gp5;
  @ requires valid_file_check_param(file_recovery);
  @ ensures  valid_file_check_result(file_recovery);
  @ assigns *file_recovery->handle, errno, file_recovery->file_size;
  @ assigns Frama_C_entropy_source;
  @*/
static void file_check_gp5(file_recovery_t *file_recovery)
{
  const unsigned char gp5_footer[13]= {
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00
  };
  file_search_footer(file_recovery, gp5_footer, sizeof(gp5_footer), 0);
}

/*@
  @ requires buffer_size >= 23;
  @ requires separation: \separated(&file_hint_gp5, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_gp5(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  if(buffer[22]!='.')
    return 0;
  if(buffer[21]=='4')
  {
    reset_file_recovery(file_recovery_new);
    file_recovery_new->extension="gp4";
    return 1;
  }
  if(buffer[21]!='5')
    return 0;
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension=file_hint_gp5.extension;
  file_recovery_new->file_check=&file_check_gp5;
  return 1;
}

static void register_header_check_gp5(file_stat_t *file_stat)
{
  static const unsigned char gp5_header[21]=  {
    0x18, 'F' , 'I' , 'C' , 'H' , 'I' , 'E' , 'R' ,
    ' ' , 'G' , 'U' , 'I' , 'T' , 'A' , 'R' , ' ' ,
    'P' , 'R' , 'O' , ' ' , 'v'
  };
  register_header_check(0, gp5_header, sizeof(gp5_header), &header_check_gp5, file_stat);
}
#endif
