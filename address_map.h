/*
 *	address_map.h
 *
 *	Definitions for handing IP/Logical Address/Port mappings.
 */

#ifndef _ADDRESS_MAP_H
#define _ADDRESS_MAP_H

#include "basic_defines.h"
#include "ip_addr.h"
#include "logical_addr.h"
#include "port.h"


struct address_map {
	char				hostname[HNAMELEN];
	char				comment[COMMENTLEN];
	Netaddr				netaddr;
	Logaddr				logaddr;
	PORT				*port;
	Boolean				duplicate_ip;		/* Is duplicate except
											   for hostname? 			*/
	Boolean				duplicate_logaddr;	/* Is duplicate la->port?   */
	struct address_map	*next_map;
};

typedef struct address_map 	ADDRESS_MAP;
typedef ADDRESS_MAP 		HOST_MAP;
typedef ADDRESS_MAP 		LOGICAL_MAP;

int add_address_map    			PROTO((char *hostname,
									   Logaddr logaddr,
									   PORT *port,
									   char *comment));

#define	add_host_map(hostname, logaddr, port)	\
		add_address_map(hostname, logaddr, port, NULL)

#define add_logical_map(logaddr, port, comment)		\
		add_address_map(NULL, logaddr, port, comment)

LOGICAL_MAP *find_map_by_logaddr	PROTO((Logaddr logaddr));
HOST_MAP *find_map_by_netaddr		PROTO((Netaddr netaddr));

HOST_MAP *first_host_map			PROTO((VOID));
HOST_MAP *next_host_map				PROTO((VOID));

#define	FOR_ALL_HOST_MAPS(map)		\
	for ((map) = first_host_map();	\
	 (map) != NULL;		\
	 (map) = next_host_map())

LOGICAL_MAP *first_logical_map		PROTO((PORT *port));
LOGICAL_MAP *next_logical_map		PROTO((VOID));

#define	FOR_ALL_LOGICAL_MAPS(map)			\
	for ((map) = first_logical_map(NULL);	\
	 (map) != NULL;							\
	 (map) = next_logical_map())

#define FOR_ALL_LOGICAL_MAPS_BY_PORT(map, port)	\
	for ((map) = first_logical_map(port);		\
	     (map) != NULL;							\
	     (map) = next_logical_map())


/*
 *	List of errors returned by add_address.
 */
#define	ADD_ADDRESS_MALLOC_ERROR		-1
#define	ADD_ADDRESS_LA_ILLEGAL			-2
#define	ADD_ADDRESS_LA_DUP				-3
#define	ADD_ADDRESS_HOST_UNKNOWN		-4
#define ADD_ADDRESS_IP_DUP				-5


#endif /* _ADDRESS_MAP_H */

		
