/*
 *	read_config.h
 *
 *	Definitions for the configuration reading routines.
 *
 */

#ifndef _READ_CONFIG_H
#define _READ_CONFIG_H

#include "basic_defines.h"
#include "switch.h"


void read_config		PROTO((char *config_file,
							   char **file_opened));
Boolean is_numeric		PROTO((char *string));
/*
 *	XXX Kludge - no proto for config_error() since we pass it a variable
 *		number of arguments. Maybe fix with varargs?
 */
void config_error();	
void config_error_l();


#endif /* _READ_CONFIG_H */
