/*
 *	portlist.h
 *
 *	Definitions for the portlist structure.
 *
 */

#ifndef _PORTLIST_H
#define _PORTLIST_H

#include "basic_defines.h"
#include "port.h"


struct portlist {
	int			num_ports;
	PORT		**ports;
};

typedef	struct portlist	PORTLIST;

PORTLIST	*add_to_portlist		PROTO((PORTLIST *list, PORT *port));
PORTLIST	*sort_portlist			PROTO((PORTLIST *list));
PORTLIST	*append_portlist		PROTO((PORTLIST *list1, PORTLIST *list2));
void		free_portlist			PROTO((PORTLIST *list));
void		disperse_portlist		PROTO((PORTLIST *list));
PORT		*first_port				PROTO((PORTLIST *list, int *portnum));
PORT		*next_port				PROTO((PORTLIST *list, int *portnum));
PORT		*last_port				PROTO((PORTLIST *list, int *portnum));
PORT		*prev_port				PROTO((PORTLIST *list, int *portnum));
PORT		*find_port_by_number	PROTO((PORTLIST *list, int portnum));

#define	FOR_EACH_PORT(portlist, port, portnum)	\
	for ((port) = first_port(portlist, &(portnum)); \
	     (port) != NULL; \
	     (port) = next_port(portlist, &(portnum)))

/*	Go through the ports in reverse order		*/
#define FOR_EACH_PORT_REVERSED(portlist, port, portnum)	\
	for ((port) = last_port(portlist, &(portnum)); \
	     (port) != NULL; \
	     (port) = prev_port(portlist, &(portnum)))


#endif /* _PORTLIST_H */

