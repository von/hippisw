/*
 *	handle_output.h
 *
 *	Definitions for switch and client output routines.
 */

#ifndef _HANDLE_OUTPUT_H
#define _HANDLE_OUTPUT_H

#include "basic_defines.h"
#include "connections.h"


int write_to_switch		PROTO((Connection *conn,
							   char *buffer,
							   int len));

int ping_switch			PROTO((Connection *conn));

int write_to_client		PROTO((Connection *conn,
							   char *string,
							   int len));


#endif /* _HANDLE_OUTPUT_H */
