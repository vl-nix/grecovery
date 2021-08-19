/*

    File: file_ecryptfs.c

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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_ecryptfs)
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
static void register_header_check_ecryptfs(file_stat_t *file_stat);

const file_hint_t file_hint_ecryptfs= {
  .extension="eCryptfs",
  .description="Encrypted file by eCryptfs",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=1,
  .register_header_check=&register_header_check_ecryptfs
};

static const unsigned char ecryptfs_header[2]= {0, 0};

struct ecrypfs_header {
  uint64_t unencrypted_file_size;
  uint32_t marker1;
  uint32_t marker2;
  unsigned char	version;
  unsigned char reserved1;
  unsigned char reserved2;
  uint32_t flags;
} __attribute__ ((gcc_struct, __packed__));

/*@
  @ requires file_recovery->file_check == &file_check_ecryptfs;
  @ requires valid_file_check_param(file_recovery);
  @ ensures  valid_file_check_result(file_recovery);
  @ assigns  file_recovery->file_size;
  @*/
static void file_check_ecryptfs(file_recovery_t *file_recovery)
{
  if(file_recovery->file_size < file_recovery->calculated_file_size)
    file_recovery->file_size=0;
  else if(file_recovery->file_size > file_recovery->calculated_file_size+1024*1024)
    file_recovery->file_size=file_recovery->calculated_file_size+1024*1024;
}

/*@
  @ requires buffer_size >= sizeof(struct ecrypfs_header);
  @ requires separation: \separated(&file_hint_ecryptfs, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_ecryptfs(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  const struct ecrypfs_header *e=(const struct ecrypfs_header *)buffer;
  const uint64_t unencrypted_file_size=be64(e->unencrypted_file_size);
  if((be32(e->marker1) ^ be32(e->marker2)) != 0x3c81b7f5)
    return 0;
  if(unencrypted_file_size < sizeof(struct ecrypfs_header))
    return 0;
  reset_file_recovery(file_recovery_new);
#ifdef DJGPP
  file_recovery_new->extension="ecr";
#else
  file_recovery_new->extension=file_hint_ecryptfs.extension;
#endif
  file_recovery_new->min_filesize=unencrypted_file_size;
  file_recovery_new->calculated_file_size=unencrypted_file_size;
  file_recovery_new->data_check=NULL;
  file_recovery_new->file_check=&file_check_ecryptfs;
  return 1;
}

static void register_header_check_ecryptfs(file_stat_t *file_stat)
{
  register_header_check(0, ecryptfs_header, sizeof(ecryptfs_header), &header_check_ecryptfs, file_stat);
}
#endif
