/*
 *	ifield.h
 *
 *	Basic defines for handling ifields.
 */

#ifndef _IFIELD_H
#define _IFIELD_H

#include "basic_defines.h"


typedef	unsigned int	Ifield;

/* Ifield Definitions */
#define	CAMPON				0x01000000
#define	LOGICAL_SELECT		0x06000000
#define LOGICAL_FIRST		0x02000000
#define	SOURCE_ROUTE		0x00000000

#define MAX_SRC_ROUTE_BITS	24
#define MODE_BITS_SHIFT		MAX_SRC_ROUTE_BITS

#endif /* _IFIELD_H */
