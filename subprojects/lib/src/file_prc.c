/*

    File: file_prc.c

    Copyright (C) 2006-2007 Christophe GRENIER <grenier@cgsecurity.org>
  
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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_prc)
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
static void register_header_check_prc(file_stat_t *file_stat);

const file_hint_t file_hint_prc = {
  .extension = "prc",
  .description = "PalmOS application",
  .max_filesize = PHOTOREC_MAX_FILE_SIZE,
  .recover = 1,
  .enable_by_default = 1,
  .register_header_check = &register_header_check_prc
};

struct DatabaseHdrType_s
{
  unsigned char name[32];
  uint16_t attributes;         /* 0x20 */
  uint32_t creationDate;       /* 0x22 */
  uint32_t modificationDate;   /* 0x26 */
  uint32_t lastBackupDate;     /* 0x2a */
  uint32_t modificationNumber; /* 0x2e */
  unsigned char appInfoID[5];  /* 0x32 */
  unsigned char sortInfoID[5];
  uint32_t type;         /* 0x3c */
  uint32_t creator;      /* 0x40 */
  uint32_t uniqueIDSeed; /* 0x44 */
  /*  RecordListType recordList; */
} __attribute__((gcc_struct, __packed__));

/*@
  @ requires buffer_size >= sizeof(struct DatabaseHdrType_s);
  @ requires separation: \separated(&file_hint_prc, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_prc(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  const struct DatabaseHdrType_s *prc = (const struct DatabaseHdrType_s *)buffer;
  if(be32(prc->uniqueIDSeed) != 0)
    return 0;
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension = file_hint_prc.extension;
  file_recovery_new->time = be32(prc->modificationDate);
  return 1;
}

static void register_header_check_prc(file_stat_t *file_stat)
{
  static const unsigned char prc_header[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 'a', 'p', 'p', 'l' };
  register_header_check(0x30, prc_header, sizeof(prc_header), &header_check_prc, file_stat);
}
#endif
