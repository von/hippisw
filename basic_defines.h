/*
 *	basic_defines.h
 *
 *	Basic definitions that everything uses.
 *
 */

#ifndef _BASIC_DEFINES_H
#define _BASIC_DEFINES_H



/*
 *	Debugging stuff
 */
#ifdef DEBUG_ALL
#define	DEBUG
#define DEBUG_CONF		/* Configuration reading routines	*/
#define DEBUG_PORTLIST		/* Portlist routines			*/
#define DEBUG_SWITCHLIST	/* Switchlist routines			*/
#define DEBUG_SWMAP		/* Switch map build routines		*/
#endif /* DEBUG_ALL */

#ifdef DEBUG
#include <stdio.h>
#endif


typedef	int		Boolean;

#define	TRUE		((Boolean) 1)
#define	FALSE		((Boolean) 0)


#ifndef NULL
#define	NULL	((void *) 0)
#endif

#define NULL_STRING(s)		(*(s) = '\0')

#define	HIPPI_MTU		65280

#define	TELNET_PORT		23

#define COMMENT_CHAR		'#'


/*	For return codes
 */
#define	NO_ERROR		0
#define ERROR			-1


/*
 *	Lengths of strings
 */
#define BUFFERLEN	120			/* For I/O buffers */
#define	HNAMELEN	40			/* Hostname string length */
#define SWNAMELEN	20			/* Switch name string length */
#define PROMPTLEN	20			/* Length of prompt string */
#define MAGICLEN	20			/* Length of magic string */
#define UNAMELEN	20			/* User name length */
#define	PASSWDLEN	20			/* Password lengths */
#define TIMELEN		18			/* Length of time strings */
#define COMMENTLEN	40			/* Length of comment */
#define PATHLEN		120			/* Length of dir pathes */
#define PIDLEN		8			/* Length of PID */
#define STARTLOGLEN	40			/* Length of start log string */

/*
 *	Convert a string to an integer.
 */
#define str_to_int(str)		((int) strtol((str), NULL, 0))

/*
 *	Function prototypes
 */
#ifdef NO_PROTOTYPES
#define	PROTO(x)		()
#else
#define PROTO(x)		x
#endif

#define VOID			void


#endif /* _BASIC_DEFINES_H */
