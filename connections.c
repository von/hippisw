/*
 *	connections.c
 *
 *	Connection structure routines.
 */

#include "basic_defines.h"
#include "connections.h"
#include "switch.h"
#include "switchlist.h"
#include "logger.h"
#include "telnet.h"
#include "prompt.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>


Connection **conn_table = NULL;
int conn_table_size = 0;


static int do_connect		PROTO((char *hostname, int port));


/*
 *	Return codes for do_connect();
 */
#define UNKNOWN_HOST		-1
#define CONNECT_FAILED		-2



/*
 *	Allocation list of connections.
 *
 *	Returns NO_ERROR or ERROR.
 */
int
alloc_conn_table()
{
	SWITCH	*sw;
	Connection	*conn;

	/* +1 for ease of reference			*/
	conn_table_size = number_of_switches() + 1;

	conn_table = (Connection **) calloc(conn_table_size, sizeof(Connection *));

	if (conn_table == NULL)
		return ERROR;

  
	FOR_ALL_SWITCHES(sw) {
		conn = (Connection *) malloc(sizeof(Connection));

		if (conn == NULL)
			return ERROR;

		conn_table[sw->sw_num] = conn;

		/*
		 * If not able to connect then set to null connection.
		 */
		if (strlen(sw->sw_hostname) == 0)
			conn->switch_state = CONNECTION_NULL;
		else
			conn->switch_state = NO_CONNECTION;
		conn->conn_fail_logged = FALSE;
		conn->sw = sw;
		conn->passwd_count = 0;
		conn->got_prompt = FALSE;
		conn->sw_sock = CLOSED_SOCK;
		conn->sw_in = CLOSED_FILE;
		conn->sw_out = CLOSED_FILE;
		conn->client_state = NO_CONNECTION;
		conn->client_sock = CLOSED_SOCK;
		conn->client_in = CLOSED_FILE;
		conn->client_out = CLOSED_FILE;
	}
  
	return NO_ERROR;
}

  
/*
 *	Return the connection structure associated with a given switch.
 */
Connection *
get_connection(sw)
	SWITCH			*sw;
{
	return conn_table[sw->sw_num];
}


/*
 *	Dump the current connection state table to a given stream.
 */
void
dump_connections(out_stream)
	FILE			*out_stream;
{
	Connection			*conn;
	int				conn_num;

	FOR_ALL_CONNECTIONS(conn, conn_num) {

		fprintf(out_stream, "Switch %s:\n",  conn->sw->sw_name);

		switch(conn->switch_state) {

		case NO_CONNECTION: 
			fprintf(out_stream, "\tConnecton pending.\n");
			break;

		case CONNECTION_NULL:
			fprintf(out_stream, "\tNo connection to make.\n");
			break;

		case CONNECTION_FAILED:
			fprintf(out_stream, "\tGiven up on connection.\n");
			break;

		case CONNECTION_ESTABLISHED:
			fprintf(out_stream, "\tConnection established. ");
			if (conn->got_prompt)
				fprintf(out_stream, "Got prompt.\n");
			else
				fprintf(out_stream, "Waiting for prompt.\n");

			if (conn->client_state == CONNECTION_ESTABLISHED) {
				fprintf(out_stream, "\tUser %s@%s connected since %s.\n",
						conn->client_name, conn->client_hostname,
						time_string(&(conn->logon_time)));
				fprintf(out_stream, "\tLast input %s\n",
						time_string(&(conn->last_input_time)));
			}
		}

		fprintf(out_stream, "\n");
	}
}



/*
 *	Open a connection to a switch.
 *
 *	Returns ERROR or NO_ERROR.
 */
int
open_switch_conn(conn)
	Connection			*conn;
{
    SWITCH		*sw = conn->sw;
    int			sock;


    if (conn->switch_state != NO_CONNECTION)
		return NO_ERROR;

    sock = do_connect(sw->sw_hostname, sw->sw_tport);

    if (sock < 0) {
		/* If we haven't logged this connection failure then do so.
		 */
		if (conn->conn_fail_logged == FALSE) {
			conn->conn_fail_logged = TRUE;

			switch(sock) {


			case UNKNOWN_HOST:

				conn->switch_state = CONNECTION_FAILED;

				log("Couldn't resolve name \"%s\" for switch \"%s\"\n",
					sw->sw_hostname, sw->sw_name);

				syslog(SYSLOG_SW_FAILED_CONN,
					   "Couldn't resolve name \"%s\" for switch \"%s\"\n",
					   sw->sw_hostname, sw->sw_name);

				break;

			case CONNECT_FAILED:


				log("Connect to %s (%s port %d) failed.\n",
					sw->sw_name, sw->sw_hostname, sw->sw_tport);

				syslog(SYSLOG_SW_FAILED_CONN,
					   "Couldn't connect to switch %s (%s port %d).",
					   sw->sw_name, sw->sw_hostname, sw->sw_tport);

				break;

			default:
				log("Connect to %s (%s port %d) failed for unknown reason.\n",
					sw->sw_name, sw->sw_hostname, sw->sw_tport);	
				
				syslog(SYSLOG_SW_FAILED_CONN,
					   "Couldn't connect to switch %s (%s port %d) for unknown reason.",
					   sw->sw_name, sw->sw_hostname, sw->sw_tport);

			}
		}

		return ERROR;
    }
  
    conn->switch_state = CONNECTION_ESTABLISHED;
    conn->last_status = 0;
    conn->telnet_state = 0;
    conn->got_prompt = FALSE;
    conn->sent_init = FALSE;
    conn->log_unexpected = FALSE;
    conn->sw_sock = sock;
    conn->sw_in = fdopen(sock, "r");
    conn->sw_out = fdopen(sock, "w");

    if ((conn->sw_in == NULL) || (conn->sw_out == NULL)) {
		log("fdopen() failed. Exiting.\n");
		syslog(SYSLOG_DIED, "fdopen() failed (%m). Exiting.");
		graceful_death(1);
    }

    telnet_init(sock);
  
    log("Connected to %s.\n", sw->sw_name);

    /* If we previously logged a failure connecting to this switch
     * then log our success.
     */
    if (conn->conn_fail_logged == TRUE) {
		conn->conn_fail_logged = FALSE;
		syslog(SYSLOG_SW_FAILED_CONN,
			   "Established connection to %s.\n",
			   sw->sw_name);
    }

    return NO_ERROR;
}
	


/*
 *	Close a connection to a switch.
 */
void
close_switch_conn(conn)
	Connection			*conn;
{
	conn->switch_state = NO_CONNECTION;

	if (conn->sw_sock != CLOSED_SOCK) {
		close(conn->sw_sock);
		fclose(conn->sw_in);
		fclose(conn->sw_out);
		conn->sw_sock = CLOSED_SOCK;
	}
}



/*
 *	Attach a new client to a switch.
 *
 */
int
new_client(conn, sock, username, hostname, flags)
	Connection		*conn;
	int				sock;
	char			*username;
	char			*hostname;
	int				flags;
{
	conn->client_state = CONNECTION_ESTABLISHED;
	conn->client_sock = sock;
	conn->client_in = fdopen(sock, "r");
	conn->client_out = fdopen(sock, "w");

	if ((conn->client_in == NULL) || (conn->client_out == NULL)) {
		log("fdopen() failed. Exiting.\n");
		syslog(SYSLOG_DIED, "fdopen() failed (%m). Exiting.");
		graceful_death(1);
	}

	strncpy(conn->client_name, username, UNAMELEN);
	strncpy(conn->client_hostname, hostname, HNAMELEN);
	conn->logon_time = conn->last_input_time = time(NULL);
	conn->flags = flags;

	return NO_ERROR;
}


/*
 *	Deattach a client from a switch.
 */
int
close_client_conn(conn)
	Connection			*conn;
{
	conn->client_state = NO_CONNECTION;

	if (conn->client_sock != CLOSED_SOCK) {
		close(conn->client_sock);
		fclose(conn->client_in);
		fclose(conn->client_out);
		conn->client_sock = CLOSED_SOCK;
	}

	return NO_ERROR;
}


/*
 *	Connect to a host and port.
 *
 *	Returns socket descriptor on success negitive on failure (see
 *	return codes above).
 */
static int
do_connect(hostname, port)
	char		*hostname;
	int		port;
{
	int			sock;
	struct sockaddr_in	addr;
	struct hostent	*hostent;


	hostent = gethostbyname(hostname);
  
	if (hostent == NULL)
		return UNKNOWN_HOST;
 
	sock = socket(PF_INET, SOCK_STREAM, 0);

	if (sock == -1) {
		log("do_connect(): socket() failed (errno = %d).\n", errno);
		syslog(SYSLOG_DIED, "socket() failed (%m). Exiting.");
		graceful_death(1);
	}


	addr.sin_family = AF_INET;
	bcopy(hostent->h_addr, &(addr.sin_addr), hostent->h_length);
	addr.sin_port = htons(port);

	if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		close(sock);
		return CONNECT_FAILED;
	}

	return sock;
}
	
