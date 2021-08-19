/*

    File: file_mysql.c

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

#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_mysql)
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdio.h>
#include "types.h"
#include "filegen.h"

/*@
  @ requires valid_register_header_check(file_stat);
  @*/
static void register_header_check_mysql(file_stat_t *file_stat);

const file_hint_t file_hint_mysql= {
  .extension="MYI",
  .description="MySQL (myi/frm)",
  .max_filesize=0,
  .recover=1,
  .enable_by_default=1,
  .register_header_check=&register_header_check_mysql
};

/*@
  @ requires separation: \separated(&file_hint_mysql, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_myisam(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  /* MySQL MYISAM compressed data file Version 1 */
  reset_file_recovery(file_recovery_new);
  file_recovery_new->extension="MYI";
  return 1;
}

/*@
  @ requires buffer_size >= 6;
  @ requires separation: \separated(&file_hint_mysql, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns  *file_recovery_new;
  @*/
static int header_check_mysql(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  /* MySQL table definition file Version 7 up to 10 */
  if(buffer[0]==0xfe && buffer[1]==0x01 && (buffer[2]>=0x07 && buffer[2]<=0x0A) && buffer[3]==0x09 && buffer[5]==0)
  {
    reset_file_recovery(file_recovery_new);
    file_recovery_new->extension="frm";
    return 1;
  }
  return 0;
}

static void register_header_check_mysql(file_stat_t *file_stat)
{
  static const unsigned char mysql_header[4]= {0xfe, 0xfe, 0x07, 0x01};
  static const unsigned char mysql_header_def[2]= {0xfe, 0x01};
  register_header_check(0, mysql_header,sizeof(mysql_header), &header_check_myisam, file_stat);
  register_header_check(0, mysql_header_def,sizeof(mysql_header_def), &header_check_mysql, file_stat);
}
#endif
