/*

    File: file_x3f.c

    Copyright (C) 1998-2005,2007 Christophe GRENIER <grenier@cgsecurity.org>
  
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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_x3f)
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

#if !defined(SINGLE_FORMAT)
extern const file_hint_t file_hint_x3i;
#endif

/*@ requires valid_register_header_check(file_stat); */
static void register_header_check_x3f(file_stat_t *file_stat);

const file_hint_t file_hint_x3f = {
  .extension = "x3f",
  .description = "Sigma/Foveon X3 raw picture",
  .max_filesize = PHOTOREC_MAX_FILE_SIZE,
  .recover = 1,
  .enable_by_default = 1,
  .register_header_check = &register_header_check_x3f
};

struct x3f_header
{
  uint32_t id;
  uint32_t version;
  unsigned char uuid[16];
  uint32_t mark;
  uint32_t columns;
  uint32_t rows;
  uint32_t rotation;
  /* version 2.1 and later have additional fields */
} __attribute__((gcc_struct, __packed__));

/*@
  @ requires buffer_size >= sizeof(struct x3f_header);
  @ requires separation: \separated(&file_hint_x3f, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_x3f(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  const struct x3f_header *h = (const struct x3f_header *)buffer;
  const unsigned int rotation = le32(h->rotation);
  if(le32(h->rows) == 0 || le32(h->columns) == 0)
    return 0;
  if(rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270)
    return 0;
#if !defined(SINGLE_FORMAT)
  if(file_recovery->file_stat != NULL && file_recovery->file_stat->file_hint == &file_hint_x3i && safe_header_only == 0)
    return 0;
#endif
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension = file_hint_x3f.extension;
  file_recovery_new->min_filesize = 1024;
  return 1;
}

static void register_header_check_x3f(file_stat_t *file_stat)
{
  static const unsigned char x3f_header[4] = { 'F', 'O', 'V', 'b' };
  register_header_check(0, x3f_header, sizeof(x3f_header), &header_check_x3f, file_stat);
}
#endif
