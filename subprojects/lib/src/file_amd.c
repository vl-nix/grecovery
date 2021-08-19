/*

    File: file_amd.c

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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_amd)
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
static void register_header_check_amd(file_stat_t *file_stat);

const file_hint_t file_hint_amd= {
  .extension="amd",
  .description="AlphaCAM (amd/amt/atd/att)",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=1,
  .register_header_check=&register_header_check_amd
};

/*@
  @ requires buffer_size >= 20;
  @ requires separation: \separated(&file_hint_amd, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_amd(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  reset_file_recovery(file_recovery_new);
  /* FIXME: I don't think it's a valid way to distinguish between files */
  if(buffer[16]=='1' && buffer[17]=='.' && buffer[18]=='1' && buffer[19]=='9')
    file_recovery_new->extension="atd";
  else
    file_recovery_new->extension=file_hint_amd.extension;
  return 1;
}

/*@
  @ requires buffer_size >= 25;
  @ requires separation: \separated(&file_hint_amd, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_amt(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  reset_file_recovery(file_recovery_new);
  /* FIXME: I don't think it's a valid way to distinguish between files */
  if(buffer[21]=='1' && buffer[22]=='.' && buffer[23]=='0' && buffer[24]=='8')
    file_recovery_new->extension="att";
  else
    file_recovery_new->extension="amt";
  return 1;
}

static void register_header_check_amd(file_stat_t *file_stat)
{
  /* amd 1.36
   * atd 1.19 */
  static const unsigned char amd_header[16]={
    'L', 'i', 'c', 'o', 'm', '-', 'A', 'P',
    'S', ' ', 'F', 'i', 'l', 'e', ' ', 'V' };

  /* amt 1.19
   * att 1.08 */
  static const unsigned char amt_header[20]={
    'L', 'i', 'c', 'o', 'm', '-', 'A', 'P',
    'S', ' ', 'T', 'o', 'o', 'l', ' ', 'F',
    'i', 'l', 'e', ' '};

  register_header_check(0, amd_header,sizeof(amd_header), &header_check_amd, file_stat);
  register_header_check(0, amt_header,sizeof(amt_header), &header_check_amt, file_stat);
}
#endif
