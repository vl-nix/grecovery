/*

    File: file_pzh.c

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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_pzh)
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
static void register_header_check_pzh(file_stat_t *file_stat);

/* Presto http://www.soft.es/ */

const file_hint_t file_hint_pzh= {
  .extension="pzh",
  .description="Presto",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=1,
  .register_header_check=&register_header_check_pzh
};

static const unsigned char pzh_header[10]=  {
  0x00, 0x00, 0x01, '8', '.', '0', 0x00, 0x02,
  0x05, 0x03
};

/*@
  @ requires file_recovery->file_rename==&file_rename_pzh;
  @ requires valid_file_rename_param(file_recovery);
  @ ensures  valid_file_rename_result(file_recovery);
  @*/
static void file_rename_pzh(file_recovery_t *file_recovery)
{
  unsigned char buffer[512];
  FILE *file;
  int buffer_size;
  if((file=fopen(file_recovery->filename, "rb"))==NULL)
    return;
  if(fseek(file, 0x9ce, SEEK_SET)<0)
  {
    fclose(file);
    return ;
  }
  buffer_size=fread(buffer, 1, sizeof(buffer), file);
  fclose(file);
  if(buffer_size > 0)
    file_rename(file_recovery, buffer, buffer_size, 0, "pzh", 0);
}

/*@
  @ requires separation: \separated(&file_hint_pzh, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_pzh(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension=file_hint_pzh.extension;
  file_recovery_new->file_rename=&file_rename_pzh;
  file_recovery_new->min_filesize=0x9c4 + sizeof(pzh_header);
  return 1;
}

static void register_header_check_pzh(file_stat_t *file_stat)
{
  register_header_check(0x9c4, pzh_header, sizeof(pzh_header), &header_check_pzh, file_stat);
}
#endif
