/*
 *	logical_addr.c
 *
 *	Routines for handling logical addresses.
 */

#include "basic_defines.h"
#include "logical_addr.h"
#include "address_config.h"
#include "port.h"
#include "switch.h"


/*
 *	Convert a logical address to an Ifield
 */
Ifield
logical_to_ifield(logaddr)
	Logaddr		logaddr;
{
	return (Ifield) (logaddr | addr_config.mode_bits);
}


/*
 *	Return TRUE if given value is a legal logical address.
 */
Boolean
legal_logical_addr(logaddr)
	Logaddr		logaddr;
{
	if ((logaddr < 0x0) || (logaddr > LOGADDR_MAX))
		return FALSE;

	return TRUE;
}


/*
 *	Return TRUE if the given value is a legal mode bits value.
 */
Boolean
legal_mode_bits(mode_bits)
	Logaddr		mode_bits;
{
	if ((mode_bits < 0x0) || (mode_bits > 0xff))
		return FALSE;

	return TRUE;
}
