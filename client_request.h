/*
 *	client_request.h
 *
 *	Header file for client request routines.
 *
 */

#ifndef _CLIENT_REQUEST_H
#define _CLINET_REQUEDT_H

#include "basic_defines.h"


typedef struct {
	char		version[VERSIONLEN];	/* client version			*/
	char		magic[MAGICLEN];		/* Magic string				*/	
	char		code;					/* Request/response code	*/
	char		flags;					/* Request flags			*/
	char		password[PASSWDLEN];	/* Password					*/
	char		sw_name[SWNAMELEN];		/* Switch to attach to		*/
	char		username[UNAMELEN];		/* Client name				*/
	char		hostname[HNAMELEN];		/* Client host	 			*/
	char		pid[PIDLEN];			/* Client process ID		*/
	char		idle_time[TIMELEN];		/* For busy reponse			*/
} CLIENT_PACKET;

/*
 *	Request values sent from client to daemon
 */
#define HIPPISWD_REQ_LOGON      'l'     /* Normal log on                */
#define HIPPISWD_REQ_PING       'P'     /* Client pings hippiswd        */
#define HIPPISWD_REQ_RESTART    'R'     /* Restart request              */
#define HIPPISWD_REQ_KILL       'K'     /* Kill request                 */
#define HIPPISWD_REQ_DUMP		'd'		/* Dump request					*/
/*
 *	Request flag values
 */
#define HIPPISWD_FLG_NOLOG	0x01	/* Suppress logging for session	*/
#define	HIPPISWD_FLG_USURP	0x02	/* Preempt current connection	*/

/*
 *	Return values send from daemon to client
 */
#define HIPPISWD_RSP_OK         'k'     /* Logon accepted				*/
#define HIPPISWD_RSP_BUSY       'b'     /* Port busy					*/
#define HIPPISWD_RSP_NO_CONN    'n'     /* No connection to switch		*/
#define HIPPISWD_RSP_NO_SUCH    's'     /* No such switch				*/
#define HIPPISWD_RSP_PING       'p'     /* Ping reply to client         */
#define HIPPISWD_RSP_RSOK       'r'     /* Restart reply                */
#define HIPPISWD_RSP_BAD_PW     'B'     /* Bad password					*/
#define HIPPISWD_RSP_NO_PERM    'N'     /* Permission denied			*/
#define HIPPISWD_RSP_ERROR      'E'     /* Error in connection			*/
#define HIPPISWD_RSP_BAD_REQ    'Q'     /* Bad request					*/
#define HIPPISWD_RSP_BAD_MAGIC	'M'	   	/* Bad magic string				*/
#define HIPPISWD_RSP_DUMP		'D'		/* Dump response				*/
#define HIPPISWD_RSP_BAD_VER	'V'		/* Version mismatch				*/


void handle_client_request		PROTO((int client_sock));

/*
 *	Does a request require a password?
 */
#ifdef HIPPISWD_REQUIRE_PASSWD

/* Always */
#define REQUEST_NEEDS_PASSWORD(request)	\
(TRUE)

#else /* HIPPISWD_REQUIRE_PASSWD */

#define REQUEST_NEEDS_PASSWORD(request) 			\
	(((request)->code == HIPPISWD_REQ_RESTART) ||	\
	 ((request)->code == HIPPISWD_REQ_KILL) ||		\
	 ((request)->flags & HIPPISWD_FLG_USURP))

#endif /* HIPPISWD_REQUIRE_PASSWD */

/*
 * Header for strings send from daemon to client
 */
typedef struct {	
		char	flags;				/* Flags				*/
		char	string[BUFFERLEN];	/* String itself		*/
	} DAEMON_PACKET;

/*
 * Flags for DAEMON_PACKET
 */
#define DAEMON_PKT_GOT_PROMPT	0x01	/* Have received prompt from switch */


#endif /* _CLIENT_REQUEST_H */
