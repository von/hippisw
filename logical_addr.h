/*
 *	logical_addr.h
 *
 *	Definitions for handling logical addresses and logical address to port
 *	mappings.
 */

#ifndef _LOGICAL_ADDR_H
#define _LOGICAL_ADDR_H

#include "basic_defines.h"
#include "ifield.h"


typedef	int		Logaddr;
/*
 *	Used to indicate that no logical address exists.
 */
#define	LOGADDR_NULL			((Logaddr) -1)

/*
 *	Used to indicate that the logical address has not yet been assigned.
 */
#define	LOGADDR_NOT_ASSIGNED	((Logaddr) -2)	

#define	LOGADDR_MAX				0xfff

Ifield logical_to_ifield	PROTO((Logaddr logaddr));
Boolean legal_logical_addr	PROTO((Logaddr logaddr));
Boolean legal_mode_bits		PROTO((Logaddr mode_bits));

/*
 *	Convert a string to a logical address
 */
#define	str_to_log(str)		((Logaddr) strtol((str), NULL, 0))


#endif /* _LOGICAL_ADDR_H */
