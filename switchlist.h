/*
 *	switchlist.h
 *
 *	Definitions for the switchlist structure.
 *
 *	These routines maintain one list of switches internally.
 *
 */

#ifndef _SWITCHLIST_H
#define _SWITCHLIST_H

#include "basic_defines.h"
#include "switch.h"


int			add_switch			PROTO((SWITCH *sw));
int			number_of_switches  PROTO((VOID));
void		free_switchlist		PROTO((VOID));
SWITCH		*first_switch		PROTO((VOID));
SWITCH		*next_switch		PROTO((SWITCH *sw));

#define	FOR_ALL_SWITCHES(sw)	\
     for ((sw) = first_switch(); (sw) != NULL; (sw) = next_switch(sw))

#endif /* _SWITCHLIST_H */


