/*

    File: file_qxd.c

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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_qxd)
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
static void register_header_check_qxd(file_stat_t *file_stat);

const file_hint_t file_hint_qxd= {
  .extension="qxd",
  .description="QuarkXpress Document",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=1,
  .register_header_check=&register_header_check_qxd
};

/*@
  @ requires separation: \separated(&file_hint_qxd, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_qxd(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension=file_hint_qxd.extension;
  file_recovery_new->min_filesize=4;
  return 1;
}

/*@
  @ requires separation: \separated(&file_hint_qxd, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_qxp(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  /* Intel or Mac QuarkXpress Document */
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension="qxp";
  file_recovery_new->min_filesize=8;
  return 1;
}

static void register_header_check_qxd(file_stat_t *file_stat)
{
  static const unsigned char qxd_header[4]={'X','P','R','3' };
  static const unsigned char qxp_header_be[6]={'I','I','X','P','R','3' };
  static const unsigned char qxp_header_le[6]={'M','M','X','P','R','3' };
  register_header_check(0, qxd_header,sizeof(qxd_header), &header_check_qxd, file_stat);
  register_header_check(2, qxp_header_be,sizeof(qxp_header_be), &header_check_qxp, file_stat);
  register_header_check(2, qxp_header_le,sizeof(qxp_header_le), &header_check_qxp, file_stat);
}
#endif
