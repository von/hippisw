/*
 *	time_string.c
 *
 *	Return a ascii string description of time.
 */

#include "basic_defines.h"
#include "time_string.h"

#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

char *
time_string(clock)
	time_t			*clock;
{
	time_t			current_time;
	struct tm		*tm;
	static char		time_buffer[TIMELEN];


	if (clock == CURRENT_TIME) {
		current_time = time(NULL);
		clock = &current_time;
	}

	tm = localtime(clock);

	strftime(time_buffer, TIMELEN, "%D %T", tm);

	return time_buffer;
}
