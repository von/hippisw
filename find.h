/*
 *	find.h
 *
 */

#ifndef _FIND_H
#define _FIND_H

#include "basic_defines.h"

#include "port.h"
#include "switch.h"
#include "address_map.h"


/*	Flags for find commands,
 */
#define FIND_DEFAULT		0
#define FIND_SUBSTRING		1


/*	Return codes.
 */
#define	NOT_FOUND		NULL

SWITCH *find_switch_by_name		PROTO((char *name, int flags));
SWITCH *find_switch_by_number		PROTO((int number, int flags));
PORT *find_port_by_name			PROTO((char *name, int flags));
ADDRESS_MAP *find_addr_map_by_name	PROTO((char *name, int flags));

#endif /* _FIND_H */

