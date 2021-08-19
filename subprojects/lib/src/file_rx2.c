/*

    File: file_rx2.c

    Copyright (C) 2012 Christophe GRENIER <grenier@cgsecurity.org>
  
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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_rx2)
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdio.h>
#include "types.h"
#include "filegen.h"
#include "common.h"

/*@ requires valid_register_header_check(file_stat); */
static void register_header_check_rx2(file_stat_t *file_stat);

const file_hint_t file_hint_rx2= {
  .extension="rx2",
  .description="Zotope RX 2, Audio Repair Software file",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=1,
  .register_header_check=&register_header_check_rx2
};

struct rx2_header
{
  uint32_t magic;
  uint32_t size;
} __attribute__ ((gcc_struct, __packed__));

/*@
  @ requires buffer_size >= sizeof(struct rx2_header);
  @ requires separation: \separated(&file_hint_rx2, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_rx2(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  const struct rx2_header *rx2=(const struct rx2_header *)buffer;
  if(memcmp(&buffer[8], "REX2HEAD", 8)!=0 || be32(rx2->size) < 4)
    return 0;
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension=file_hint_rx2.extension;
  file_recovery_new->calculated_file_size=(uint64_t)be32(rx2->size)+8;
  file_recovery_new->data_check=&data_check_size;
  file_recovery_new->file_check=&file_check_size;
  return 1;
}

static void register_header_check_rx2(file_stat_t *file_stat)
{
  static const unsigned char rx2_header[4]=  { 'C' , 'A' , 'T' , ' ' };
  register_header_check(0, rx2_header, sizeof(rx2_header), &header_check_rx2, file_stat);
}
#endif
