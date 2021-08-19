/*

    File: file_sqlite.c

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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_sqlite)
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
static void register_header_check_sqlite(file_stat_t *file_stat);

const file_hint_t file_hint_sqlite= {
  .extension="sqlite",
  .description="SQLite",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=1,
	.register_header_check=&register_header_check_sqlite
};

/* http://www.sqlite.org/fileformat.html */
struct db_header
{
 char magic[16];
 uint16_t pagesize;
 uint8_t  ffwrite;
 uint8_t  ffread;
 uint8_t  reserved;
 uint8_t  max_emb_payload_frac;
 uint8_t  min_emb_payload_frac;
 uint8_t  leaf_payload_frac;
 uint32_t file_change_counter;
 uint32_t filesize_in_page;
 uint32_t first_freelist_page;
 uint32_t freelist_pages;
 uint32_t schema_cookie;
 uint32_t schema_format;
 uint32_t default_page_cache_size;
 uint32_t largest_root_btree;
 uint32_t text_encoding;
 uint32_t user_version;
 uint32_t inc_vacuum_mode;
 uint32_t app_id;
 char     reserved_for_expansion[20];
 uint32_t version_valid_for;
 uint32_t version;
} __attribute__ ((gcc_struct, __packed__));

/*@
  @ requires buffer_size >= sizeof(struct db_header);
  @ requires separation: \separated(&file_hint_sqlite, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_sqlite(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  const struct db_header *hdr=(const struct db_header *)buffer;
  unsigned int pagesize=be16(hdr->pagesize);
  const unsigned int filesize_in_page=be32(hdr->filesize_in_page);
  /* Must be a power of two between 512 and 32768 inclusive, or the value 1 representing a page size of 65536. */
  if(pagesize==1)
    pagesize=65536;
  if(pagesize<512 || ((pagesize-1) & pagesize)!=0)
    return 0;
  reset_file_recovery(file_recovery_new);
#ifdef DJGPP
  file_recovery_new->extension="sql";
#else
  file_recovery_new->extension=file_hint_sqlite.extension;
#endif
  file_recovery_new->min_filesize=sizeof(struct db_header);
  if(filesize_in_page!=0 && be32(hdr->file_change_counter)==be32(hdr->version_valid_for))
  {
    file_recovery_new->calculated_file_size=(uint64_t)filesize_in_page * pagesize;
    file_recovery_new->data_check=&data_check_size;
    file_recovery_new->file_check=&file_check_size;
  }
  return 1;
}

static void register_header_check_sqlite(file_stat_t *file_stat)
{
  register_header_check(0, "SQLite format 3", 16, &header_check_sqlite, file_stat);
}
#endif
