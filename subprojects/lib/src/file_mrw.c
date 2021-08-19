/*

    File: file_mrw.c

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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_mrw)
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdio.h>
#include "types.h"
#include "common.h"
#include "filegen.h"
#include "log.h"

/*@ requires valid_register_header_check(file_stat); */
static void register_header_check_mrw(file_stat_t *file_stat);

const file_hint_t file_hint_mrw= {
  .extension="mrw",
  .description="Minolta Raw picture",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=1,
  .register_header_check=&register_header_check_mrw
};

struct hdr {
  uint32_t fourcc;
  uint32_t size;
#if 0
  char data[0];
#endif
}  __attribute__ ((gcc_struct, __packed__));

struct prd {
  char ver[8];
  struct {
    uint16_t y;
    uint16_t x;
  } ccd;
  struct {
    uint16_t y;
    uint16_t x;
  } img;
  uint8_t datasize;  // bpp, 12 or 16
  uint8_t pixelsize; // bits used, always 12
  uint8_t storagemethod; // 0x52 means not packed
  uint8_t unknown1;
  uint16_t unknown2;
  uint16_t pattern; // 0x0001 RGGB, or 0x0004 GBRG
}  __attribute__ ((gcc_struct, __packed__));

/* Minolta */
/*@
  @ requires buffer_size >= 2*sizeof(struct hdr) + sizeof(struct prd);
  @ requires separation: \separated(&file_hint_mrw, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_mrw(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  const unsigned char prd_header[4]= { 0x00,'P','R','D'};
  const struct hdr *mrmhdr = (const struct hdr*)buffer;
  const struct hdr *prdhdr = (const struct hdr*)&buffer[sizeof(struct hdr)];
  /* Picture Raw Dimensions */
  const struct prd *prd = (const struct prd*)&buffer[2*sizeof(struct hdr)];
  if(memcmp(&prdhdr->fourcc, prd_header, sizeof(prd_header))!=0)
    return 0;
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension=file_hint_mrw.extension;
  file_recovery_new->calculated_file_size= (uint64_t)be32(mrmhdr->size)+ 8 +
    ((uint64_t)be16(prd->ccd.x) * be16(prd->ccd.y) * prd->datasize + 8 - 1) / 8;
  file_recovery_new->data_check=&data_check_size;
  file_recovery_new->file_check=&file_check_size;
  /*
     log_debug("size=%lu x=%lu y=%lu datasize=%lu\n", be32(mrmhdr->size),
     be16(prd->ccd.x), be16(prd->ccd.y), prd->datasize);
     log_debug("mrw_file_size %lu\n", (long unsigned)file_recovery_new->calculated_file_size);
     */
  return 1;
}

static void register_header_check_mrw(file_stat_t *file_stat)
{
  static const unsigned char mrw_header[4]= { 0x00,'M','R','M'}; /* Minolta Raw */
  register_header_check(0, mrw_header,sizeof(mrw_header), &header_check_mrw, file_stat);
}
#endif
