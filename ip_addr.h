/*
 *	ip_addr.h
 *
 *	Basic definitions for handling IP addresses (Netaddrs) and for mapping
 *	IP addresses to logical addresses.
 */

#ifndef _IP_ADDR_H
#define _IP_ADDR_H

#include "basic_defines.h"


typedef	unsigned long		Netaddr;
#define	NETADDR_NULL		((Netaddr) NULL)

char *netaddr_to_ascii		PROTO((Netaddr netaddr));
char *netaddr_to_fullname	PROTO((Netaddr netaddr));
Netaddr hostname_to_netaddr	PROTO((char *hostname));
char *hostname_to_fullname	PROTO((char *hostname));
int handle_hippi_addr		PROTO((char *hostname,
								   Netaddr *netaddr));
#endif /* _IP_ADDR_H */
