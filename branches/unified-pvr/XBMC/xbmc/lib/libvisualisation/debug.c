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
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * debug.c - functions to produce and control debug output from
 *           libviz routines.
 */

#include <stdlib.h>
#include <mvp_refmem.h>
#include <libvisualisation.h>
#include <viz_local.h>
#include <mvp_debug.h>

static mvp_debug_ctx_t viz_debug_ctx = MVP_DEBUG_CTX_INIT("VIZ",
							    VIZ_DBG_NONE,
							    NULL);

/*
 * viz_dbg_level(int l)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Set the current debug level to the absolute setting 'l'
 * permitting all debug messages with a debug level less
 * than or equal to 'l' to be displayed.
 *
 * Return Value:
 *
 * None.
 */
void
viz_dbg_level(int l)
{
	mvp_dbg_setlevel(&viz_debug_ctx, l);
}

/*
 * viz_dbg_all()
 * 
 * Scope: PUBLIC
 * 
 * Description
 *
 * Set the current debug level so that all debug messages are displayed.
 *
 * Return Value:
 *
 * None.
 */
void
viz_dbg_all()
{
	mvp_dbg_setlevel(&viz_debug_ctx, VIZ_DBG_ALL);
}

/*
 * viz_dbg_none()
 * 
 * Scope: PUBLIC
 * 
 * Description
 *
 * Set the current debug level so that no debug messages are displayed.
 *
 * Return Value:
 *
 * None.
 */
void
viz_dbg_none()
{
	mvp_dbg_setlevel(&viz_debug_ctx, VIZ_DBG_NONE);
}

/*
 * viz_dbg()
 * 
 * Scope: PRIVATE (mapped to __viz_dbg)
 * 
 * Description
 *
 * Print a debug message of level 'level' on 'stderr' provided that
 * the current debug level allows messages of level 'level' to be
 * printed.
 *
 * Return Value:
 *
 * None.
 */
void
viz_dbg(int level, char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	mvp_dbg(&viz_debug_ctx, level, fmt, ap);
	va_end(ap);
}
