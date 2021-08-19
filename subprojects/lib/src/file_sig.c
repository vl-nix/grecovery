/*

    File: file_sig.c

    Copyright (C) 2010 Christophe GRENIER <grenier@cgsecurity.org>
  
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
#if ((!defined(SINGLE_FORMAT) && !defined(__FRAMAC__)) || defined(SINGLE_FORMAT_sig))
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <ctype.h>
#if defined(__FRAMAC__)
#include "__fc_builtin.h"
#endif
#include "types.h"
#include "filegen.h"
#include "common.h"
#include "log.h"

/*@ requires valid_string_s: valid_read_string(s);
  @ ensures  valid_string(\result);
  @*/
static char *td_strdup(const char *s)
{
  size_t l = strlen(s) + 1;
  char *p = MALLOC(l);
  /*@ assert valid_read_string(s); */
  memcpy(p, s, l);
  p[l-1]='\0';
  /*@ assert valid_read_string(p); */
  return p;
}

/*@ requires valid_register_header_check(file_stat); */
static void register_header_check_sig(file_stat_t *file_stat);

const file_hint_t file_hint_sig= {
  .extension="custom",
  .description="Own custom signatures",
  .max_filesize=PHOTOREC_MAX_FILE_SIZE,
  .recover=1,
  .enable_by_default=1,
  .register_header_check=&register_header_check_sig
};

#define WIN_PHOTOREC_SIG "\\photorec.sig"
#define DOT_PHOTOREC_SIG "/.photorec.sig"
#define PHOTOREC_SIG "photorec.sig"

typedef struct signature_s signature_t;
struct signature_s
{
  struct td_list_head list;
  const char *extension;
  unsigned char *sig;
  unsigned int sig_size;
  unsigned int offset;
};

static signature_t signatures={
  .list = TD_LIST_HEAD_INIT(signatures.list)
};

#ifndef __FRAMAC__
static
#endif
int signature_cmp(const struct td_list_head *a, const struct td_list_head *b)
{
  const signature_t *sig_a=td_list_entry_const(a, const signature_t, list);
  const signature_t *sig_b=td_list_entry_const(b, const signature_t, list);
  int res;
  if(sig_a->sig_size==0 && sig_b->sig_size!=0)
    return -1;
  if(sig_a->sig_size!=0 && sig_b->sig_size==0)
    return 1;
  res=sig_a->offset-sig_b->offset;
  if(res!=0)
    return res;
  if(sig_a->sig_size<=sig_b->sig_size)
  {
    res=memcmp(sig_a->sig,sig_b->sig, sig_a->sig_size);
    if(res!=0)
      return res;
    return 1;
  }
  else
  {
    res=memcmp(sig_a->sig,sig_b->sig, sig_b->sig_size);
    if(res!=0)
      return res;
    return -1;
  }
}

/*@
  @ requires offset <= PHOTOREC_MAX_SIG_OFFSET;
  @ requires 0 < sig_size <= PHOTOREC_MAX_SIG_SIZE;
  @ requires offset + sig_size <= PHOTOREC_MAX_SIG_OFFSET;
  @ requires \valid_read(sig + (0 .. sig_size-1));
  @ requires valid_read_string(extension);
  @*/
static void signature_insert(const char *extension, unsigned int offset, unsigned char *sig, unsigned int sig_size)
{
  /* FIXME: small memory leak */
  signature_t *newsig=(signature_t*)MALLOC(sizeof(*newsig));
  newsig->extension=extension;
  newsig->sig=sig;
  newsig->sig_size=sig_size;
  newsig->offset=offset;
  td_list_add_sorted(&newsig->list, &signatures.list, signature_cmp);
}

/*@
  @ requires separation: \separated(&file_hint_sig, buffer+(..), file_recovery, file_recovery_new);
  @ requires valid_header_check_param(buffer, buffer_size, safe_header_only, file_recovery, file_recovery_new);
  @ ensures  valid_header_check_result(\result, file_recovery_new);
  @ assigns *file_recovery_new;
  @*/
static int header_check_sig(const unsigned char *buffer, const unsigned int buffer_size, const unsigned int safe_header_only, const file_recovery_t *file_recovery, file_recovery_t *file_recovery_new)
{
  struct td_list_head *pos;
  /*@ loop assigns pos; */
  td_list_for_each(pos, &signatures.list)
  {
    const signature_t *sig = td_list_entry(pos, signature_t, list);
    /*@ assert sig->offset + sig->sig_size <= buffer_size; */
    /*@ assert valid_read_string(sig->extension); */
    if(memcmp(&buffer[sig->offset], sig->sig, sig->sig_size)==0)
    {
      reset_file_recovery(file_recovery_new);
      file_recovery_new->extension=sig->extension;
      /*@ assert valid_file_recovery(file_recovery_new); */
      return 1;
    }
  }
  return 0;
}

static FILE *open_signature_file(void)
{
#if defined(__CYGWIN__) || defined(__MINGW32__)
  {
    char *path;
    path = getenv("USERPROFILE");
    if (path == NULL)
      path = getenv("HOMEPATH");
    if(path!=NULL)
    {
      FILE*handle;
      char *filename=NULL;
      filename=(char*)MALLOC(strlen(path)+strlen(WIN_PHOTOREC_SIG)+1);
      strcpy(filename, path);
      strcat(filename, WIN_PHOTOREC_SIG);
      handle=fopen(filename,"rb");
      if(handle!=NULL)
      {
	log_info("Open signature file %s\n", filename);
	free(filename);
	return handle;
      }
      free(filename);
    }
  }
#endif
#ifndef DJGPP
  {
    const char *home = getenv("HOME");
    if (home != NULL)
    {
      FILE*handle;
      char *filename;
      size_t len_home;
      const size_t len_sig=strlen(DOT_PHOTOREC_SIG);
#ifdef __FRAMAC__
      home="/home/user";
#endif
      len_home=strlen(home);
      /*@ assert len_home == strlen(home); */
      filename=(char*)MALLOC(len_home + len_sig + 1);
      /*@ assert \valid(filename + (0 .. len_home + len_sig)); */
      strcpy(filename, home);
      /*@ assert len_home == strlen(filename); */
      strcat(filename, DOT_PHOTOREC_SIG);
      handle=fopen(filename,"rb");
      if(handle!=NULL)
      {
#ifndef __FRAMAC__
	log_info("Open signature file %s\n", filename);
#endif
	free(filename);
	return handle;
      }
      free(filename);
    }
  }
#endif
  {
    FILE *handle=fopen(PHOTOREC_SIG,"rb");
    if(handle!=NULL)
    {
#ifndef __FRAMAC__
      log_info("Open signature file %s\n", PHOTOREC_SIG);
#endif
      return handle;
    }
  }
  return NULL;
}

/*@
  @ requires valid_read_string(src);
  @ requires \valid(resptr);
  @ requires \separated(src+(..), resptr);
  @ ensures  valid_read_string(\result);
  @ assigns  *resptr;
  @*/
static char *str_uint_hex(char *src, unsigned int *resptr)
{
  unsigned int res=0;
  /*@
    @ loop invariant valid_read_string(src);
    @ loop invariant res < 0x10000000;
    @ loop assigns src, res;
    @*/
  for(;;src++)
  {
    const char c=*src;
    if(c>='0' && c<='9')
      res=res*16+(c-'0');
    else if(c>='A' && c<='F')
      res=res*16+(c-'A'+10);
    else if(c>='a' && c<='f')
      res=res*16+(c-'a'+10);
    else
    {
      *resptr=res;
      return src;
    }
    if(res >= 0x10000000)
    {
      *resptr=res;
      return src;
    }
  }
}

/*@
  @ requires valid_read_string(src);
  @ requires \valid(resptr);
  @ requires \separated(src+(..), resptr);
  @ ensures  valid_read_string(\result);
  @ assigns  *resptr;
  @*/
static char *str_uint_dec(char *src, unsigned int *resptr)
{
  unsigned int res=0;
  /*@
    @ loop invariant valid_read_string(src);
    @ loop invariant res < 0x10000000;
    @ loop assigns src, res;
    @*/
  for(;*src>='0' && *src<='9';src++)
  {
    res=res*10+(*src)-'0';
    if(res >= 0x10000000)
    {
      *resptr=res;
      return src;
    }
  }
  *resptr=res;
  return src;
}

/*@
  @ requires valid_read_string(src);
  @ requires \valid(resptr);
  @ requires \separated(src+(..), resptr);
  @ ensures  valid_read_string(\result);
  @ assigns  *resptr;
  @*/
static char *str_uint(char *src, unsigned int *resptr)
{
  if(*src=='0' && (*(src+1)=='x' || *(src+1)=='X'))
    return str_uint_hex(src+2, resptr);
  return str_uint_dec(src, resptr);
}

/*@
  @ requires valid_register_header_check(file_stat);
  @ requires valid_file_stat(file_stat);
  @ requires valid_string((char *)pos);
  @ ensures  valid_string((char *)\result);
  @*/
static unsigned char *parse_signature_file(file_stat_t *file_stat, unsigned char *pos)
{
  const unsigned int signatures_empty=td_list_empty(&signatures.list);
#ifndef __FRAMAC__
  while(*pos!='\0')
#endif
  {
    /* skip comments */
    /*@
      @ loop invariant valid_string((char *)pos);
      @ loop assigns pos;
      @*/
    while(*pos=='#')
    {
      /*@
	@ loop invariant valid_string((char *)pos);
	@ loop assigns pos;
	@*/
      while(*pos!='\0' && *pos!='\n')
	pos++;
      if(*pos=='\0')
	return pos;
      pos++;
    }
    /* each line is composed of "extension offset signature" */
    {
      char *extension;
      unsigned int offset=0;
      unsigned char *tmp=NULL;
      unsigned int signature_max_size=512;
      unsigned int signature_size=0;
      {
	const char *extension_start=(const char *)pos;
	/*@
	  @ loop invariant valid_string((char *)pos);
	  @ loop assigns pos;
	  @*/
	while(*pos!='\0' && !isspace(*pos))
	  pos++;
	if(*pos=='\0')
	  return pos;
	*pos='\0';
	pos++;
	/*@ assert valid_string((char *)pos); */
	extension=td_strdup(extension_start);
	if(extension == NULL)
	  return pos;
	/*@ assert valid_read_string(extension); */
	/*@ assert extension==\null || \freeable(extension); */
      }
      /* skip space */
      /*@ assert valid_string((char *)pos); */
      /*@
	@ loop invariant valid_string((char *)pos);
        @ loop assigns pos;
	@*/
      while(*pos=='\t' || *pos==' ')
      {
	/*@ assert *pos == '\t' || *pos== ' '; */
	/*@ assert valid_string((char *)pos); */
	pos++;
	/*@ assert valid_string((char *)pos); */
      }
      /* read offset */
      pos=(char *)str_uint((char *)pos, &offset);
      if(offset > PHOTOREC_MAX_SIG_OFFSET)
      {
	/* Invalid offset */
	free(extension);
	return pos;
      }
      /*@ assert offset <= PHOTOREC_MAX_SIG_OFFSET; */
      /* read signature */
      tmp=(unsigned char *)MALLOC(signature_max_size);
      /*@ assert valid_string((char *)pos); */
#ifdef __FRAMAC__
      if(*pos!='\n' && *pos!='\0')
#else
      /*@
	@ loop invariant valid_string((char *)pos);
	@ loop assigns pos, signature_size, tmp[0 .. signature_max_size-1];
	@*/
      while(*pos!='\n' && *pos!='\0')
#endif
      {
	if(signature_size>=signature_max_size)
	{
#ifdef __FRAMAC__
	  free(tmp);
	  free(extension);
	  return pos;
#else
	  unsigned char *tmp_old=tmp;
	  signature_max_size*=2;
	  tmp=(unsigned char *)realloc(tmp, signature_max_size);
	  if(tmp==NULL)
	  {
	    free(tmp_old);
	    free(extension);
	    return pos;
	  }
#endif
	}
	/*@ assert signature_size < signature_max_size; */
	if(signature_size > PHOTOREC_MAX_SIG_SIZE)
	{
	  free(tmp);
	  free(extension);
	  return pos;
	}
	/*@ assert signature_size <= PHOTOREC_MAX_SIG_SIZE; */
	if(isspace(*pos) || *pos=='\r' || *pos==',')
	  pos++;
	else if(*pos== '\'')
	{
	  pos++;
	  if(*pos=='\0')
	  {
	    free(extension);
	    free(tmp);
	    return pos;
	  }
	  else if(*pos=='\\')
	  {
	    pos++;
	    if(*pos=='\0')
	    {
	      free(extension);
	      free(tmp);
	      return pos;
	    }
	    else if(*pos=='b')
	      tmp[signature_size++]='\b';
	    else if(*pos=='n')
	      tmp[signature_size++]='\n';
	    else if(*pos=='t')
	      tmp[signature_size++]='\t';
	    else if(*pos=='r')
	      tmp[signature_size++]='\r';
	    else if(*pos=='0')
	      tmp[signature_size++]='\0';
	    else
	      tmp[signature_size++]=*pos;
	    pos++;
	  }
	  else
	  {
	    tmp[signature_size++]=*pos;
	    pos++;
	  }
	  if(*pos!='\'')
	  {
	    free(tmp);
	    free(extension);
	    return pos;
	  }
	  pos++;
	}
	else if(*pos=='"')
	{
	  pos++;
	  /*@
	    @ loop invariant valid_string((char *)pos);
	    @ loop assigns pos, extension, signature_size, tmp[0 .. signature_max_size-1];
	    @*/
	  for(; *pos!='"' && *pos!='\0'; pos++)
	  {
	    if(signature_size>=signature_max_size)
	    {
#ifdef __FRAMAC__
	      free(tmp);
	      free(extension);
	      return pos;
#else
	      unsigned char *tmp_old=tmp;
	      signature_max_size*=2;
	      tmp=(unsigned char *)realloc(tmp, signature_max_size);
	      if(tmp==NULL)
	      {
		free(tmp_old);
		free(extension);
		return pos;
	      }
#endif
	    }
	    if(*pos=='\\')
	    {
	      pos++;
	      if(*pos=='\0')
	      {
		free(tmp);
		free(extension);
		return pos;
	      }
	      else if(*pos=='b')
		tmp[signature_size++]='\b';
	      else if(*pos=='n')
		tmp[signature_size++]='\n';
	      else if(*pos=='r')
		tmp[signature_size++]='\r';
	      else if(*pos=='t')
		tmp[signature_size++]='\t';
	      else if(*pos=='0')
		tmp[signature_size++]='\0';
	      else
		tmp[signature_size++]=*pos;
	    }
	    else
	      tmp[signature_size++]=*pos;;
	  }
	  if(*pos!='"')
	  {
	    free(tmp);
	    free(extension);
	    return pos;
	  }
	  pos++;
	}
	else if(*pos=='0' && (*(pos+1)=='x' || *(pos+1)=='X'))
	{
	  pos+=2;
	  /*@ assert valid_string((char *)pos); */
#ifdef __FRAMAC__
	  if(signature_size < signature_max_size-1 &&
	      *pos!='\0' &&
	      isxdigit(*pos) &&
	      isxdigit(*(pos+1)))
#else
	  /*@
	    @ loop invariant valid_string((char *)pos);
	    @ loop assigns pos, signature_size, tmp[0 .. signature_max_size-1];
	    @*/
	  while(signature_size < signature_max_size-1 &&
	      isxdigit(*pos) &&
	      isxdigit(*(pos+1)))
#endif
	  {
	    unsigned int val;
	    unsigned char c;
	    c=*pos;
	    if(c>='0' && c<='9')
	      val=c-'0';
	    else if(c>='A' && c<='F')
	      val=c-'A'+10;
	    else if(c>='a' && c<='f')
	      val=c-'a'+10;
#ifdef __FRAMAC__
	    else
	    {
	      free(tmp);
	      free(extension);
	      return pos;
	    }
#endif
	    /*@ assert 0 <= val < 16; */
	    /*@ assert valid_string((char *)pos); */
	    /*@ assert *pos != 0; */
	    pos++;
	    /*@ assert valid_string((char *)pos); */
	    val*=16;
	    /*@ assert 0 <= val <= 240; */
	    c=*pos;
	    if(c>='0' && c<='9')
	      val+=c-'0';
	    else if(c>='A' && c<='F')
	      val+=c-'A'+10;
	    else if(c>='a' && c<='f')
	      val+=c-'a'+10;
#ifdef __FRAMAC__
	    else
	    {
	      free(tmp);
	      free(extension);
	      return pos;
	    }
#endif
	    /*@ assert valid_string((char *)pos); */
	    /*@ assert *pos != 0; */
	    pos++;
	    /*@ assert valid_string((char *)pos); */
	    tmp[signature_size++]=val;
	  }
	}
	else
	{
	  free(tmp);
	  free(extension);
	  return pos;
	}
	/*@ assert valid_string((char *)pos); */
      }
      if(*pos=='\n')
	pos++;
      if(signature_size>0 && offset + signature_size <= PHOTOREC_MAX_SIG_OFFSET )
      {
	/*@ assert signature_size > 0; */
	/*@ assert offset + signature_size <= PHOTOREC_MAX_SIG_OFFSET; */
#ifndef __FRAMAC__
	/* FIXME: Small memory leak */
	unsigned char *signature=(unsigned char *)MALLOC(signature_size);
	/*@ assert \valid(signature + (0 .. signature_size - 1)); */
#ifndef __FRAMAC__
	log_info("register a signature for %s\n", extension);
#endif
	memcpy(signature, tmp, signature_size);
	/*@ assert \valid_read(signature + (0 .. signature_size - 1)); */
	register_header_check(offset, signature, signature_size, &header_check_sig, file_stat);
	if(signatures_empty)
	  signature_insert(extension, offset, signature, signature_size);
#endif
      }
      else
      {
	free(extension);
      }
      free(tmp);
    }
  }
  return pos;
}

static void register_header_check_sig(file_stat_t *file_stat)
{
  unsigned char *pos;
  char *buffer;
  size_t buffer_size;
  struct stat stat_rec;
  FILE *handle;
  handle=open_signature_file();
  if(!handle)
    return;
#ifdef __FRAMAC__
  buffer_size=1024*1024;
#else
  if(fstat(fileno(handle), &stat_rec)<0 || stat_rec.st_size>100*1024*1024)
  {
    fclose(handle);
    return;
  }
  buffer_size=stat_rec.st_size;
#endif
  buffer=(char *)MALLOC(buffer_size+1);
  if(fread(buffer,1,buffer_size,handle)!=buffer_size)
  {
    fclose(handle);
    free(buffer);
    return;
  }
  fclose(handle);
#if defined(__FRAMAC__)
  Frama_C_make_unknown(buffer, buffer_size);
#endif
  buffer[buffer_size]='\0';
  pos=buffer;
  pos=parse_signature_file(file_stat, pos);
  if(*pos!='\0')
  {
#ifndef __FRAMAC__
    log_warning("Can't parse signature: %s\n", pos);
#endif
  }
  free(buffer);
}
#endif
