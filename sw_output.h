/*
 *	sw_output.h
 *
 *	Definitions for switch command output routines.
 */

#ifndef _SW_OUTPUT_H
#define _SW_OUTPUT_H

#include "basic_defines.h"
#include "switch.h"
#include "path.h"
#include "port.h"
#include "logical_addr.h"

#include <stdio.h>

char *set_output_switch			PROTO((SWITCH *sw));
FILE *set_switch_output_stram	PROTO((FILE *stream));
void sw_no_comments				PROTO((VOID));
/*
 *	XXX KLUDGE - no proto for sw_comment since the number of args
 *		is variable.
 */
char *sw_comment();
char *disable_all_ports			PROTO((VOID));
char *enable_all_ports			PROTO((VOID));
char *clear_all_pathways        PROTO((VOID));
char *save_all_pathways			PROTO((VOID));
char *set_pathway_all			PROTO((Logaddr address,
									   PATH *portlist));
char *set_pathway				PROTO((int in_port,
									   Logaddr address,
									   PATH *portlist));
char *display_port_stats		PROTO((int portnum));
char *display_switch_stats		PROTO((VOID));
char *clear_port_stats			PROTO((int portnum));
char *clear_switch_stats		PROTO((VOID));

#endif /* _SW_OUTPUT_H */
