/*

  mas.c
  
  mass storage interface subsystem

  part of m2tklib (Mini Interative Interface Toolkit Library)
  
  Copyright (C) 2011  olikraus@gmail.com

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mas.h"

/*======================================================================*/
/* global variables */

/* ptr to the low level procedures */
mas_device_fn *mas_device = (mas_device_fn *)0;

/* common work buffer for one file name */
char mas_entry_name[12+1];   /* 12 chars for the name, 1 for the terminating '\0' */
uint8_t mas_entry_is_dir = 0;

/* present working directory */
char mas_pwd[MAS_PATH_MAX];

/* cache size should be at least the number of lines, displayed with m2tk STRLIST element */
#define MAS_CACHE_SIZE 6
char mas_cache_entry_name[MAS_CACHE_SIZE][2+12+1];               /* the cached name of the file or directory */
uint8_t mas_cache_entry_is_dir[MAS_CACHE_SIZE];                            /* is the cached entry a file or directory */
uint16_t mas_cache_entry_idx[MAS_CACHE_SIZE];                               /* position of the cached entry within the directory  */
uint8_t mas_cache_entry_rr = 0;                                                     /* where to place the next missed entry (round robin strategy) */

uint16_t mas_cache_dir_entry_cnt = 0;

/*======================================================================*/
/* cache procedures */


/* called by all directory change procedures */
static void mas_clear_cache(void)
{
  uint8_t i;
  mas_cache_entry_rr = 0;
  for( i = 0 ; i < MAS_CACHE_SIZE; i++ )
    mas_cache_entry_idx[i] = 0x0ffff;
  
  mas_cache_dir_entry_cnt = 0x0ffff;
}

/*
  Description: 
    Find an entry in the cache, return cache number when found and copy filename 
    and directory information to the common work buffer.
  Argument: 
    The position within the directory
  Return: 
    255 if there was no cache hit.
*/
static uint8_t mas_get_cache_entry(uint16_t n)
{
  uint8_t i;
  for( i = 0 ; i < MAS_CACHE_SIZE; i++ )
    if ( mas_cache_entry_idx[i] == n )
    {
      strcpy(mas_entry_name, mas_cache_entry_name[i]);
      mas_entry_is_dir = mas_cache_entry_is_dir[i];
      return i;
    }
  return 255;
}

/*
  Description: 
    Put the content of the common work buffer into the cache. 
    This procedure does NOT check if this entry is already inside the cache.
  Argument: 
    1) Valid filename and directory information in the common work buffer
    2) The position within the directory as parameter
*/
static void mas_put_into_cache(uint16_t n)
{
  strcpy(mas_cache_entry_name[mas_cache_entry_rr], mas_entry_name);
  mas_cache_entry_is_dir[mas_cache_entry_rr] = mas_entry_is_dir;
  mas_cache_entry_idx[mas_cache_entry_rr] = n;
  mas_cache_entry_rr++;
  if ( mas_cache_entry_rr >= MAS_CACHE_SIZE )
    mas_cache_entry_rr = 0;
}


/*======================================================================*/
/* present working directory */

uint8_t mas_change_dir_down(const char *subdir)
{
  uint8_t len;

  mas_clear_cache();
    
  len = strlen(mas_pwd);
  
  /* ensure that the subdir fits into the pwd buffer */
  if ( len + strlen(subdir) + 2 > MAS_PATH_MAX )
    return 0;
  mas_pwd[len] = MAS_DIR_SEP;
  strcpy(mas_pwd+len+1, subdir);
  
  printf("mas_change_dir_down: %s\n", mas_pwd);
  
  return 1;
}

uint8_t mas_change_dir_up(void)
{
  uint8_t len;
  
  mas_clear_cache();
  
  len = strlen(mas_pwd);
  
  /* remove the last directory entry from the pwd buffer */
  for(;;)
  {
    if ( len == 0 )
      break;
    len--;
    if ( mas_pwd[len] == MAS_DIR_SEP )
      break;
  }
  mas_pwd[len] = '\0';

  printf("mas_change_dir_up: %s\n", mas_pwd);
  
  /* move the file system to the new directory */
  return 1; 
}

void mas_change_dir_to_root(void)
{  
  mas_clear_cache();
  
  mas_pwd[0] = '\0';  
}



/*======================================================================*/
/* get file within the current working directory */

/*
  Description:
    Get an entry from a directory. Store the entry into the common work buffer.
  Argument:
    Position within the directory, 0 .. mas_get_dir_entry_cnt()-1;
  Returns:
    0 for error
*/
uint8_t mas_get_nth_file(uint16_t n)
{
  mas_arg_get_dir_entry_at_pos_t arg;
  
  if ( mas_device == (mas_device_fn *)0 )
    return 0;
  
  /* check if the position is in the cache */
  if ( mas_get_cache_entry(n) != 255 )
    return 1;   /* cache hit, result stored in the common work buffer */
  
  /* copy the entry from the file system into the common work buffer */
  arg.path = mas_pwd;
  arg.n = n;
  arg.buf = mas_entry_name;
  if ( mas_device(MAS_MSG_GET_DIR_ENTRY_AT_POS, &arg) == 0 )
    return 0; 
  mas_entry_is_dir = arg.is_dir;
  
  /* put the new entry into the cache */
  mas_put_into_cache(n);
  
  return 1;
}

/*======================================================================*/
/* get the number of entries in a directory */
uint16_t mas_get_dir_entry_cnt(void)
{
  mas_arg_get_dir_entry_cnt_t arg;
  
  printf("mas_get_dir_entry_cnt: ");
  
  if ( mas_cache_dir_entry_cnt != 0x0ffff )
    return printf("cached %d\n", mas_cache_dir_entry_cnt), mas_cache_dir_entry_cnt;
  
  if ( mas_device == (mas_device_fn *)0 )
    return printf("device not set %d\n", mas_cache_dir_entry_cnt), 0;
  
  arg.path = mas_pwd;
  arg.cnt = 0;

  if ( mas_device(MAS_MSG_GET_DIR_ENTRY_CNT, &arg) == 0 )
    return printf("device failed %d\n", mas_cache_dir_entry_cnt), 0; 
  
  mas_cache_dir_entry_cnt = arg.cnt;
  
  printf("calculated %d\n", mas_cache_dir_entry_cnt);
  
  return arg.cnt;
}


/*======================================================================*/
/* init */
uint8_t mas_init(mas_device_fn *device, uint8_t cs_pin)
{
  mas_arg_init_t arg;
  
  mas_device = device;
  arg.cs_pin = cs_pin;
  
  mas_clear_cache();
  
  return mas_device(MAS_MSG_INIT, &arg);
}

