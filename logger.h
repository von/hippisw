/*
 *	logger.h
 *
 *	 logging include file.
 */

#ifndef _LOGGER_H
#define _LOGGER_H

#include "basic_defines.h"

#include <stdio.h>
#include <syslog.h>


int open_log_file		PROTO((char *filename,
							   char *default_name));
/*
 *	XXX Kludge - no proto for log() or log_message() since they have
 *		 a variable number of arguments.
 */

/*	For loggging daemon-specific messages			*/
void log();				/* string, arg1, arg2, arg3, ...	*/

/*	For logging other stuff					*/
void log_message();		/* name, string, arg1, arg2, arg3, ...	*/

void close_log_file		PROTO((VOID));


FILE *open_log_cmd		PROTO((char *switch_name));
void close_log_cmd		PROTO((FILE *pipe));

void init_syslog		PROTO((char *myname));


/*
 * syslog options.
 */
#ifndef SYSLOG_FACILITY
#define SYSLOG_FACILITY		LOG_DAEMON
#endif

#ifndef SYSLOG_OPTS
#define	SYSLOG_OPTS			LOG_CONS
#endif


/*
 * Priorities for syslog()
 */
#define SYSLOG_SW_EOF			LOG_ERR		/* Got EOF from switch		*/
#define SYSLOG_SW_FAILED_CONN	LOG_ERR		/* Failed to connect to
											   switch					*/
#define SYSLOG_KILLED			LOG_NOTICE	/* Killed by user or
											   terminate signal			*/
#define SYSLOG_DIED				LOG_ERR		/* Unexpected death
											   (i.e. segment failt)		*/
#define SYSLOG_RESTART			LOG_NOTICE	/* Restarted.				*/

#endif /* _LOGGER_H */
