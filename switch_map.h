/*
 *	switch_map.h
 *
 *	Definitions for switch to switch mapping routines.
 */

#ifndef _SWITCH_MAP_H
#define _SWITCH_MAP_H


#include "basic_defines.h"
#include "switch.h"
#include "path.h"

PATH *find_path		PROTO((SWITCH *from, SWITCH *to));

#endif /* _SWITCH_MAP_H */
