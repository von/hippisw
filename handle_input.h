/*
 *	handle_input.h
 */

#ifndef _HANDLE_INPUT_H
#define _HANDLE_INPUT_H

#include "basic_defines.h"
#include "connections.h"

void handle_switch_input		PROTO((Connection *conn));
void handle_client_input		PROTO((Connection *conn));

#endif /* _HANDLE_INPUT_H */
