/*
 *	time_string.h
 *
 *	Header file for time_string() function.
 */

#ifndef _TIME_STRING_H
#define _TIME_STRING_H

#include "basic_defines.h"

#include <sys/types.h>

char *time_string			PROTO((time_t *clock));

#define CURRENT_TIME		((time_t *) NULL)

#endif /* _TIME_STRING_H */
