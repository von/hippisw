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
static Boolean poll_switch	PROTO((Connection *conn));


#define SELECT_TIMEOUT		2		/* Seconds	*/

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
    conn->sw = sw;
    conn->got_prompt = FALSE;
    conn->prompt_char = sw->sw_prompt[strlen(sw->sw_prompt) - 1];
    conn->sw_sock = CLOSED_SOCK;
    conn->sw_in = CLOSED_FILE;
    conn->sw_out = CLOSED_FILE;
    conn->passwd_count  = 0;
    
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

  if (sock < 0)
    switch(sock) {

    case UNKNOWN_HOST:
      log("Couldn't resolve name \"%s\" for switch \"%s\"\n",
	  sw->sw_hostname, sw->sw_name);
      conn->switch_state = CONNECTION_FAILED;
      return ERROR;
     
    case CONNECT_FAILED:
      log("Connect to %s (%s port %d) failed.\n",
	  sw->sw_name, sw->sw_hostname, sw->sw_tport);
      return ERROR;
    }
  
  conn->switch_state = CONNECTION_ESTABLISHED;
  conn->last_status = 0;
  conn->telnet_state = 0;
  conn->got_prompt = FALSE;
  conn->sw_sock = sock;
  conn->sw_in = fdopen(sock, "r");
  conn->sw_out = fdopen(sock, "w");
  
  if ((conn->sw_in == NULL) || (conn->sw_out == NULL)) {
    log("fdopen() failed. Exiting.\n");
    graceful_death(1);
  }

  telnet_init(sock);
  
  log("Connected to %s.\n", sw->sw_name);

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
 *	Write a string to a switch.
 */
int
write_to_switch(conn, string, len)
     Connection			*conn;
     char			*string;
     int			len;
{
  char		buffer[BUFFERLEN];
  register int	read_pos = 0, write_pos = 0;
  

  while (read_pos < len) {
    /*
     *	Parse string handling backslash characters and converting
     *	carriage returns.
     */
    for ( write_pos = 0;
	 (read_pos < len) && (write_pos < BUFFERLEN); ) {
      char c = string[read_pos];

      switch(c) {
      case '\n':	/* STRIP */
      case '\r':
      case '\0':
	read_pos++;
	break;

      case '\\':
	read_pos++;
	c = string[read_pos++];
	switch(c) {
	case 'n':
	  buffer[write_pos++] = '\r';
	  buffer[write_pos++] = '\n';
	  break;

	default:
	  buffer[write_pos++] = c;
	  break;
	}
	break;

      default:
	buffer[write_pos++] = string[read_pos++];
      }
    }
    
    buffer[write_pos++] = '\r';
    buffer[write_pos++] = '\n';

    /*
     *	We dont't actually want to send the NULL.
     */
    buffer[write_pos] = '\0';

    write(conn->sw_sock, buffer, write_pos);
  }

  return NO_ERROR;
}



/*
 *	Read a line of input from a switch.
 *
 *	Returns ERROR or number of bytes read.
 */
int
read_from_switch(conn, buffer, len)
     Connection			*conn;
     char			*buffer;
     int			len;
{
  int			bytes_read = 0;
  int			byte;
  Boolean		done = FALSE;


  NULL_STRING(buffer);

  /*
   *	If last read attempt returned an empty buffer, the poll
   *	for data first.
   */
  if (conn->last_status == TELNET_EOB)
    if (poll_switch(conn) == FALSE)
      return 0;

  while(!done) {
    byte = telnet(conn->sw_sock, &conn->telnet_state);

    conn->last_status = byte;

    switch(byte) {
      
    case 0:
      continue;		/* Ignore nulls	*/

    case TELNET_EOF:
      close_switch_conn(conn);
      log_message(conn->sw->sw_name, "Got EOF.\n");
      if (conn->client_state == CONNECTION_ESTABLISHED)
	close_client_conn(conn);
      return ERROR;

    case TELNET_EOB:
      if (poll_switch(conn) == FALSE)
	done = TRUE;
      break;

    case TELNET_OVERFLOW:
      log_message("telnet", "telnet buffer overflow!\n");
      graceful_death(1);

    case '\r':			/* Strip */
      continue;

    default:			/* Real character	*/
      buffer[bytes_read++] = (char) byte;
      buffer[bytes_read] = '\0';
      /* Are we done with the line?			*/
      if (byte == '\n' ||
	  (byte == conn->prompt_char && is_prompt(conn, buffer)) ||
	  (byte == PASSWD_PROMPT_CHAR && is_password_prompt(buffer)))
	done = TRUE;
    }
  }

  return bytes_read;
}
  
    
/*
 *	Attach a new client to a switch.
 *
 */
int
new_client(conn, sock, username, hostname, flags)
     Connection			*conn;
     int			sock;
     char			*username;
     char			*hostname;
     int			flags;
{
  conn->client_state = CONNECTION_ESTABLISHED;
  conn->client_sock = sock;
  conn->client_in = fdopen(sock, "r");
  conn->client_out = fdopen(sock, "w");

  if ((conn->client_in == NULL) || (conn->client_out == NULL)) {
    log("fdopen() failed. Exiting.\n");
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
 *	Write a string to the client.
 */
int
write_to_client(conn, string, len)
     Connection			*conn;
     char			*string;
     int			len;
{
  char				length;

  len++;				/* Include NULL */

  length = (char) len;

  write(conn->client_sock, &length, sizeof(char));
	
  return write(conn->client_sock, string, len);
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
    graceful_death(1);
  }


  addr.sin_family = AF_INET;
  bcopy(hostent->h_addr, &(addr.sin_addr), hostent->h_length);
  addr.sin_port = htons(port);

  if (connect(sock, &addr, sizeof(addr)) == -1) {
    close(sock);
    return CONNECT_FAILED;
  }

  return sock;
}
	

/*
 *	Poll a switch for input.
 *
 *	Returns TRUE if data is waiting, FALSE otherwise.
 */
static Boolean
poll_switch(conn)
     Connection			*conn;
{
  struct timeval	timeout;
  fd_set      		readfds;
  static int		max_fd = -1;

  
  if (max_fd == -1)
    max_fd = (int) ulimit(4, 0);

  FD_ZERO(&readfds);
  FD_SET(conn->sw_sock, &readfds);
  
  if (conn->got_prompt == TRUE) {
    timeout.tv_usec = timeout.tv_sec = 0;

  } else {
    timeout.tv_usec = 0;
    timeout.tv_sec = SELECT_TIMEOUT;
  }
  
  if (select(max_fd, &readfds, NULL, NULL, &timeout) < 0) {
    log("select() error %d\n", errno);
    return FALSE;
  }
  
  return FD_ISSET(conn->sw_sock, &readfds);
}
