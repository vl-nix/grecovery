/*

    File: geometry.c

    Copyright (C) 1998-2008 Christophe GRENIER <grenier@cgsecurity.org>
  
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "types.h"
#include "common.h"
#include "intrf.h"
#include "log.h"
#include "hdaccess.h"
#include "geometry.h"
#include "autoset.h"

#define MAX_HEADS 255

void set_cylinders_from_size_up(disk_t *disk_car)
{
  disk_car->geom.cylinders=(disk_car->disk_size / disk_car->sector_size +
      (uint64_t)disk_car->geom.sectors_per_head * disk_car->geom.heads_per_cylinder - 1) /
    ((uint64_t)disk_car->geom.sectors_per_head * disk_car->geom.heads_per_cylinder);
}

int change_sector_size(disk_t *disk, const int cyl_modified, const unsigned int sector_size)
{
  /* Using 3*512=1536 as sector size and */
  /* 63/3=21 for number of sectors is an easy way to test */
  /* MS Backup internal blocksize is 256 bytes */
  switch(sector_size)
  {
    case 1:
    case 256:
    case 512:
    case 1024:
    case 3*512:
    case 2048:
    case 4096:
    case 8192:
      disk->sector_size = sector_size;
      if(cyl_modified==0)
	set_cylinders_from_size_up(disk);
      return 0;
    default:
      return 1;
  }
}

int change_geometry_cli(disk_t *disk_car, char ** current_cmd)
{
  int done = 0;
  int tmp_val=0;
  int cyl_modified=0;
  int geo_modified=0;
  if(*current_cmd==NULL)
    return 0;
#ifndef __FRAMAC__
  log_info("Current geometry\n%s sector_size=%u\n", disk_car->description(disk_car), disk_car->sector_size);
#endif
  /*@ loop invariant valid_read_string(*current_cmd); */
  while (done==0)
  {
    skip_comma_in_command(current_cmd);
    if(check_command(current_cmd,"C,",2)==0)
    {
      tmp_val = get_int_from_command(current_cmd);
      if (tmp_val > 0)
      {
	disk_car->geom.cylinders = tmp_val;
	cyl_modified=1;
	if(geo_modified==0)
	  geo_modified=1;
      }
#ifndef __FRAMAC__
      else
	log_error("Illegal cylinders value\n");
#endif
    }
    else if(check_command(current_cmd,"H,",2)==0)
    {
      tmp_val = get_int_from_command(current_cmd);
      if (tmp_val > 0 && tmp_val <= MAX_HEADS)
      {
	disk_car->geom.heads_per_cylinder = tmp_val;
	if(geo_modified==0)
	  geo_modified=1;
	if(cyl_modified==0)
	  set_cylinders_from_size_up(disk_car);
      }
#ifndef __FRAMAC__
      else
	log_error("Illegal heads value\n");
#endif
    }
    else if(check_command(current_cmd,"S,",2)==0)
    {
      tmp_val = get_int_from_command(current_cmd);
      /* SUN partition can have more than 63 sectors */
      if (tmp_val > 0) {
	disk_car->geom.sectors_per_head = tmp_val;
	if(geo_modified==0)
	  geo_modified=1;
	if(cyl_modified==0)
	  set_cylinders_from_size_up(disk_car);
      }
#ifndef __FRAMAC__
      else
	log_error("Illegal sectors value\n");
#endif
    }
    else if(check_command(current_cmd,"N,",2)==0)
    {
      tmp_val = get_int_from_command(current_cmd);
      if(change_sector_size(disk_car, cyl_modified, tmp_val)==0)
	geo_modified=2;
#ifndef __FRAMAC__
      else
	  log_error("Illegal sector size\n");
#endif
    }
    else
    {
      done = 1;
    }
    if(cyl_modified!=0)
      disk_car->disk_size=(uint64_t)disk_car->geom.cylinders*disk_car->geom.heads_per_cylinder*disk_car->geom.sectors_per_head*disk_car->sector_size;
  }
  /*@ assert valid_read_string(*current_cmd); */
  if(geo_modified!=0)
  {
    disk_car->disk_size=(uint64_t)disk_car->geom.cylinders*disk_car->geom.heads_per_cylinder*disk_car->geom.sectors_per_head*disk_car->sector_size;
#ifdef __APPLE__
    /* On MacOSX if HD contains some bad sectors, the disk size may not be correctly detected */
    disk_car->disk_real_size=disk_car->disk_size;
#endif
#ifndef __FRAMAC__
    log_info("New geometry\n%s sector_size=%u\n", disk_car->description(disk_car), disk_car->sector_size);
#endif
    autoset_unit(disk_car);
    if(geo_modified==2)
    {
      /*@ assert valid_read_string(*current_cmd); */
      return 1;
    }
  }
  /*@ assert valid_read_string(*current_cmd); */
  return 0;
}
