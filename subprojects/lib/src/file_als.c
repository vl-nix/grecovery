/*

    File: file_als.c

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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_als)
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
static void register_header_check_als(file_stat_t *file_stat);

const file_hint_t file_hint_als= {
  .extension="als",
  .description="Ableton Live Sets",
  .max_filesize=100*1024*1024,
  .recover=1,
  .enable_by_default=1,
  .register_header_check=&register_header_check_als
};

/*@
  @ requires file_recovery->file_check == &file_check_als;
  @ requires \separated(file_recovery, file_recovery->handle, file_recovery->extension, &errno, &Frama_C_entropy_source);
  @ requires valid_file_check_param(file_recovery);
  @ ensures  valid_file_check_result(file_recovery);
  @ assigns *file_recovery->handle, errno, file_recovery->file_size;
  @ assigns Frama_C_entropy_source;
  @*/
static void file_check_als(file_recovery_t *file_recovery)
{
  static const unsigned char als_footer[0x16]= {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x00, 0x00, 0x80, 0x01
  };
  file_search_footer(file_recovery, als_footer, sizeof(als_footer), 7);
}

/*@
  @ requires buffer_size >= 11+13;
  @ requires separation: \separated(&file_hint_als, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_als(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  static const unsigned char als_header2[13]= {
    0x0c, 'L', 'i', 'v', 'e', 'D',  'o',  'c',
    'u', 'm', 'e', 'n', 't'
  };
  if(memcmp(buffer+11,als_header2,sizeof(als_header2))!=0)
    return 0;
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension=file_hint_als.extension;
  file_recovery_new->file_check=&file_check_als;
  return 1;
}

/* Header
 * 0000  ab 1e 56 78 03 XX 00 00  00 00 XX 0c 4c 69 76 65  |  Vx        Live|
 * 0010  44 6f 63 75 6d 65 6e 74  XX 00 00 00 00 XX XX XX  |Document        |
 */
static void register_header_check_als(file_stat_t *file_stat)
{
  static const unsigned char als_header[5]= {
    0xab, 0x1e,  'V',  'x', 0x03
  };
  register_header_check(0, als_header,sizeof(als_header), &header_check_als, file_stat);
}
#endif
