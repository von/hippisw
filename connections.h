/*
 *	connections.h
 *
 *	Definitions for switch and client connections.
 */

#ifndef _CONNECTIONS_H
#define _CONNECTIONS_H

#include "basic_defines.h"
#include "switch.h"
#include "ip_addr.h"

#include <stdio.h>
#include <sys/types.h>


typedef enum	{		/* Connection states			*/
  NO_CONNECTION, 		/* Connection not yet established	*/
  CONNECTION_NULL,		/* No connection to establish		*/
  CONNECTION_FAILED,		/* Failed to establish connection	*/
  CONNECTION_ESTABLISHED	/* Connection established		*/
} Conn_state;


typedef struct	{
  Conn_state		switch_state;		/* Connection state	*/
  Boolean		conn_fail_logged;	/* Logged conn failure?	*/
  int			last_status;		/* Last telnet() value	*/
  int			telnet_state;		/* For telnet()		*/
  SWITCH		*sw;			/* Switch		*/
  Boolean		got_prompt;		/* Gotten prompt?	*/
  Boolean		sent_init;		/* Have we inited switch? */
  Boolean		log_unexpected;		/* Log unexpected output? */
  int			sw_sock;		/* Socket to switch	*/
  FILE			*sw_in;			/* Stream from switch	*/
  FILE			*sw_out;		/* Stream to switch	*/
  int			passwd_count;		/* # times passwd sent	*/
  Conn_state		client_state;		/* Client state		*/
  int			client_sock;		/* Client socket	*/
  FILE			*client_in;		/* Stream from client	*/
  FILE			*client_out;		/* Stream to client	*/
  char			client_name[UNAMELEN];	/* Client name		*/
  char			client_hostname[HNAMELEN];	
  						/* Client host		*/
  time_t		logon_time;		/* Time logged on	*/
  time_t		last_input_time;	/* Time of last input	*/
  int			flags;			/* Log the session?	*/
} Connection;


extern Connection **conn_table;
extern int conn_table_size;


#define	CLOSED_SOCK		-1
#define CLOSED_FILE		NULL



#define FOR_ALL_CONNECTIONS(conn, conn_num)	\
	for ((conn_num) = 1, (conn) = conn_table[conn_num];		\
	     (conn_num) < conn_table_size;				\
	     (conn_num)++, (conn) = conn_table[conn_num])


int alloc_conn_table		PROTO((VOID));
Connection *get_connection	PROTO((SWITCH *sw));
void dump_connections		PROTO((FILE *out_stream));

int open_switch_conn		PROTO((Connection *conn));
void close_switch_conn		PROTO((Connection *conn));

int new_client			PROTO((Connection *conn,
				       int sock,
				       char *name,
				       char *hostname,
				       int flags));
int close_client_conn		PROTO((Connection *conn));


#endif /* _CONNECTIONS_H */
