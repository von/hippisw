/*
 *	logger.h
 *
 *	 logging include file.
 */

#ifndef _LOGGER_H
#define _LOGGER_H

#include "basic_defines.h"

int open_log_file		PROTO((char *filename,
				       char *default_name));
/*
 *	XXX Kludge - no proto for log() or log_message() since they have
 *		 a variable number of arguments.
 */
/*	For loggging daemon-specific messages			*/
void log();		/* string, arg1, arg2, arg3, ...	*/
/*	For logging other stuff					*/
void log_message();	/* name, string, arg1, arg2, arg3, ...	*/

void close_log_file		PROTO((VOID));

#endif /* _LOGGER_H */
