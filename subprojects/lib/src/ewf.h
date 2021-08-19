/*

    File: ewf.h

    Copyright (C) 2006 Christophe GRENIER <grenier@cgsecurity.org>
  
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
#ifndef _EWF_H
#define _EWF_H
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__FRAMAC__) || defined(MAIN_photorec)
#undef HAVE_LIBEWF
#endif

#if defined(HAVE_LIBEWF_H) && defined(HAVE_LIBEWF)
/*@
  @ requires valid_read_string(device);
  @ ensures  valid_disk(\result);
  @*/
disk_t *fewf_init(const char *device, const int testdisk_mode);
#endif
/*@ assigns \nothing; */
const char*td_ewf_version(void);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif
#endif
