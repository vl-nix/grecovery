/*

    File: jfs.h

    Copyright (C) 2004,2006,2008 Christophe GRENIER <grenier@cgsecurity.org>
  
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
#ifndef _JFS_H
#define _JFS_H
#ifdef __cplusplus
extern "C" {
#endif
/* real size is 184 */
#define JFS_SUPERBLOCK_SIZE 512

#define L2BPERDMAP 13      /* l2 num of blks per dmap */
/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ requires \valid(partition);
  @ requires separation: \separated(disk_car, partition);
  @*/
int check_JFS(disk_t *disk_car, partition_t *partition);

/*@
  @ requires \valid_read(disk_car);
  @ requires valid_disk(disk_car);
  @ requires \valid_read(sb);
  @ requires \valid(partition);
  @ requires separation: \separated(disk_car, sb, partition);
  @*/
int recover_JFS(const disk_t *disk_car, const struct jfs_superblock *sb, partition_t *partition, const int verbose, const int dump_ind);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif
#endif
