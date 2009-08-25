/*
 *  Copyright (C) 2004-2006, Eric Lund
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file setting.c
 * Functions that operate on individual settings
 */
#include <sys/types.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <errno.h>
#include <mvp_refmem.h>
#include <mvp_debug.h>
#include <libaddon.h>
#include <addon_local.h>

/*
 * addon_setting_destroy(addon_setting_t p)
 *
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Destroy the setting structure pointed to by 'p' ad release
 * it's storage. This should only be called by refmem_release().
 *
 * Return Value:
 *
 * None.
 */
static void
addon_setting_destroy(addon_setting_t p)
{
  refmem_dbg(MVP_DBG_DEBUG, "%s {\n", __FUNCTION__);
  if (!p) {
    //refmem_dbg(MVP_DBG_DEBUG, "%s }!a\n", __FUNCTION__);
    return;
  }
  if (p->id) {
    refmem_release(p->id);
  }
  if(p->label) {
    refmem_release(p->label);
  }
  if(p->enable) {
    refmem_release(p->enable);
  }
  if(p->lvalues) {
    refmem_release(p->lvalues);
  }
  refmem_dbg(MVP_DBG_DEBUG, "%s }\n", __FUNCTION__);
}

/*
 * addon_setting_create(void)
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a new addon_setting structure and return pointer to the structure
 *
 * Return Value:
 *
 * Success: A non-NULL addon_setting_t
 *
 * Failure: A NULL addon_setting_t
 */
addon_setting_t
addon_setting_create(void)
{
  addon_setting_t ret = refmem_alloc(sizeof(*ret));

  refmem_dbg(MVP_DBG_DEBUG, "%s\n", __FUNCTION__);
  if(!ret) {
    return NULL;
  }
  refmem_set_destroy(ret, (refmem_destroy_t)addon_setting_destroy);

  ret->type = 0; // Label type
  ret->id = NULL;
  ret->label = NULL;
  ret->enable = NULL;
  ret->lvalues = NULL;
  ret->valid = 0; // valid
  return ret;
}

/*
* addon_setting_type(addon_setting_t setting)
*
*
* Scope: PUBLIC
*
* Retrieves the 'type' field of a setting structure
*
* Return Value:
*
* Sucess: An addon_setting_type_t enum.
*
*/
addon_setting_type_t
addon_setting_type(addon_setting_t setting)
{
  if (!setting) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL setting structure\n",
      __FUNCTION__);
    return SETTING_SEP;
  }
  return setting->type;
}

/*
* addon_setting_set_type(addon_setting_t setting, addon_setting_type_t type)
*
*
* Scope: PUBLIC
*
* Modifies the 'type' field of a setting structure
*
* Will release the ref counted element if non-NULL
*
* Return Value:
*
* Sucess: NULL.
*
* Failure: A non-NULL integer
*/
int
addon_setting_set_type(addon_setting_t setting, addon_setting_type_t type)
{
  if (!setting) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL setting structure\n",
      __FUNCTION__);
    return -1;
  }

  setting->type = type;
  return 0;
}

/*
 * addon_setting_id(addon_setting_t setting)
 *
 *
 * Scope: PUBLIC
 *
 * Retrieves the 'id' field of a setting structure
 *
 * Returns a pointer to the string within the setting
 * structure, so it should not be modified by the caller.
 * refmem_release() must be called when you are finished with the pointer.
 *
 * Return Value:
 *
 * Sucess: A 'char *' pointing to the field.
 *
 * Failure: A NULL 'char *'
 */
char * 
addon_setting_id(addon_setting_t setting)
{
  if (!setting) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL setting structure\n",
      __FUNCTION__);
    return NULL;
  }
  return refmem_hold(setting->id);
}

/*
* addon_setting_set_id(addon_setting_t setting, char *id)
*
*
* Scope: PUBLIC
*
* Modifies the 'id' field of a setting structure
*
* Will release the ref counted element if non-NULL
*
* Return Value:
*
* Sucess: NULL.
*
* Failure: A non-NULL integer
*/
int
addon_setting_set_id(addon_setting_t setting, char *id)
{
  if (!setting) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL setting structure\n",
      __FUNCTION__);
    return -1;
  }
  if(setting->id) {
    refmem_release(setting->id);
  }
  setting->id = refmem_strdup(id);

  return (int)refmem_hold(setting->id);
}

/*
 * addon_setting_label(addon_setting_t setting)
 *
 *
 * Scope: PUBLIC
 *
 * Retrieves the 'label' field of a setting structure
 *
 * Returns a pointer to the string within the setting
 * structure, so it should not be modified by the caller.
 * refmem_release() must be called when you are finished with the pointer.
 *
 * Return Value:
 *
 * Sucess: A 'char *' pointing to the field.
 *
 * Failure: A NULL 'char *'
 */
char * 
addon_setting_label(addon_setting_t setting)
{
  if (!setting) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL setting structure\n",
      __FUNCTION__);
    return NULL;
  }
  return refmem_hold(setting->label);
}

/*
* addon_setting_set_label(addon_setting_t setting, char *label)
*
*
* Scope: PUBLIC
*
* Modifies the 'label' field of a setting structure
*
* Will release the ref counted element if non-NULL
*
* Return Value:
*
* Sucess: NULL.
*
* Failure: A non-NULL integer
*/
int
addon_setting_set_label(addon_setting_t setting, char *label)
{
  if (!setting) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL setting structure\n",
      __FUNCTION__);
    return -1;
  }
  if(setting->label) {
    refmem_release(setting->label);
  }
  setting->label = refmem_strdup(label);

  return (int)refmem_hold(setting->label);
}

/*
 * addon_setting_enable(addon_setting_t setting)
 *
 *
 * Scope: PUBLIC
 *
 * Retrieves the 'enable' field of a setting structure
 *
 * Returns a pointer to the string within the setting
 * structure, so it should not be modified by the caller.
 * refmem_release() must be called when you are finished with the pointer.
 *
 * Return Value:
 *
 * Sucess: A 'char *' pointing to the field.
 *
 * Failure: A NULL 'char *'
 */
char * 
addon_setting_enable(addon_setting_t setting)
{
  if (!setting) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL setting structure\n",
      __FUNCTION__);
    return NULL;
  }
  return refmem_hold(setting->enable);
}

/*
 * addon_setting_lvalues(addon_setting_t setting)
 *
 *
 * Scope: PUBLIC
 *
 * Retrieves the 'lvalues' field of a setting structure
 *
 * Returns a pointer to the string within the setting
 * structure, so it should not be modified by the caller.
 * refmem_release() must be called when you are finished with the pointer.
 *
 * Return Value:
 *
 * Sucess: A 'char *' pointing to the field.
 *
 * Failure: A NULL 'char *'
 */
char * 
addon_setting_lvalues(addon_setting_t setting)
{
  if (!setting) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL setting structure\n",
      __FUNCTION__);
    return NULL;
  }
  return refmem_hold(setting->lvalues);
}

/*
* addon_setting_set_lvalues(addon_setting_t setting, char *lvalues)
*
*
* Scope: PUBLIC
*
* Modifies the 'lvalues' field of a setting structure
*
* Will release the ref counted element if non-NULL
*
* Return Value:
*
* Sucess: NULL.
*
* Failure: A non-NULL integer
*/
int
addon_setting_set_lvalues(addon_setting_t setting, char *lvalues)
{
  if (!setting) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL setting structure\n",
      __FUNCTION__);
    return -1;
  }
  if(setting->lvalues) {
    refmem_release(setting->lvalues);
  }
  setting->lvalues = refmem_strdup(lvalues);

  return (int)refmem_hold(setting->lvalues);
}

/*
 * addon_setting_valid(addon_setting_t setting)
 *
 *
 * Scope: PUBLIC
 *
 * Retrieves the 'valid' field of a setting structure
 *
 * Returns a signed interger representing the validity of the setting
 * a non ZERO value signifies invalid
 *
 * Return Value:
 *
 * Sucess: 0
 *
 * Failure: non ZERO integer
 */
int 
addon_setting_valid(addon_setting_t setting)
{
  if (!setting) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL setting structure\n",
      __FUNCTION__);
    return -1;
  }
  return setting->valid;
}

