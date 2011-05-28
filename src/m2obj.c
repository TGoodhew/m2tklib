/*
  m2.c
  
  m2tklib = Mini Interative Interface Toolkit Library
  
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


  
  Simplified Top Level API of the m2tklib.
  Most important: Init and event processing  
  - read keys 
  - pass key values to the event handler
  
  
  
*/

#include "m2.h"

/*
  element: 	the root element
  es:			event source handler, can be NULL
  eh:			event handler to process the source events
  gh:			graphics handler
*/
void m2_InitM2(m2_p m2, m2_rom_void_p element, m2_es_fnptr es, m2_eh_fnptr eh, m2_gfx_fnptr gh)
{
  m2->is_frame_draw_at_end = 0;
  m2->key_queue_len = 0;
  m2->key_queue_pos = 0;
  m2->es = es;
  m2->eh = eh;
  m2->gh = gh;
  m2_gfx_init(gh);
  m2->is_frame_draw_at_end = m2_gfx_is_frame_draw_at_end();
  m2->forced_key = M2_KEY_REFRESH;
  m2_nav_init(m2_get_nav(m2),  element);
}

void m2_CheckKeyM2(m2_p m2)
{
  uint8_t key;
  
  /* step 1: get raw key */
  
  /* check if a key should be forced */
  key = m2->forced_key;
  if ( key != M2_KEY_NONE )
  {
    m2->forced_key = M2_KEY_NONE;
    m2_PutKeyIntoQueue(m2, key);
  }
  else
  {
    /* request key information from the event source */
    if ( m2->es != NULL )
    {
      key = m2->es(m2, M2_ES_MSG_GET_KEY, NULL);
      m2_SetDetectedKey(m2, key);
    }
    else
      key = M2_KEY_NONE;
  }
}


/*
  return:
    0: nothing happend
    1:	event has been processed
*/
uint8_t m2_StepM2(m2_p m2)
{
  uint8_t is_redraw_required = 0;
  uint8_t key;
  
  for(;;)
  {
    key = m2_GetKeyFromQueue(m2);

    /* if there are no more keys, break out of the loop */
    if ( key == M2_KEY_NONE )
      break;
    
    /* otherwise, process the key event */
    /* note, that key numbers are equal to message numbers */
    if ( m2->eh != NULL )
    {
      /* handle the key */
      m2->eh(m2, key, 0);
      /* check if the root node has been changed */
      m2_nav_check_and_assign_new_root(m2_get_nav(m2));
    }
    
    is_redraw_required = 1;
  }
  
  return is_redraw_required;
}

void m2_SetFontM2(m2_p m2, uint8_t font_idx, const void *font_ptr)
{
  m2_gfx_set_font(m2->gh, font_idx, font_ptr);
}

void m2_SetPinM2(m2_p m2, uint8_t msg, uint8_t pin)
{
  m2_pin_t pin_data;
  pin_data.msg = msg;
  pin_data.pin = pin;
  if ( m2->es != NULL )
    m2->es(m2, M2_ES_MSG_SET_PIN, &pin_data);
}
