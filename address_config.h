/*
 *	Address configuration structure
 */

#ifndef _ADDRESS_CONFIG_H
#define _ADDRESS_CONFIG_H

#include "basic_defines.h"
#include "ip_addr.h"
#include "logical_addr.h"
#include "port.h"
#include "switch.h"


struct address_config {
	Netaddr		hippi_network;	/* For "hippi-xxx" addresses				*/
	Logaddr		loopback_addr;	/* Loopback addresses						*/
	int			default_type;	/* What type of default logical addresses	*/
	int			mode_bits;		/* What mode bits to use for logical
								   addressing 								*/
	PORT		*tester_port;	/* Port the tester is on					*/
	Logaddr		tester_addr;	/* Start address for tester					*/
};

/*
 *	default_type values
 */
#define	NO_DEFAULT_LA		0	/* Don't assign any default address			*/
#define	DO_LA_1374			1	/* Assign according to RFC 1374				*/
#define	DO_LA_IP_8			2	/* Use lower byte of IP address				*/
#define DO_LA_IP_12			3	/* Use lower 12 bits of IP address			*/

Logaddr get_default_logical		PROTO((PORT *port));
Logaddr get_tester_address     	PROTO((SWITCH *sw));

#ifdef BUILD_CONFIG_STRUCTURES

struct address_config	addr_config =
{ NETADDR_NULL,
  0x000,
  DO_LA_1374,
  LOGICAL_SELECT | CAMPON,
  NULL,
  0xf00 };

#else /* BUILD_CONFIG_STRUCTURES */

extern struct address_config	addr_config;

#endif /* BUILD_CONFIG_STRUCTURES */


#endif /* _ADDRESS_CONFIG_H */
