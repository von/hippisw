/*
 *	handle_input.c
 *
 *	Routines that handle input from client and switch.
 */


#include "basic_defines.h"
#include "connections.h"
#include "client_request.h"
#include "password_config.h"
#include "prompt.h"
#include "telnet.h"
#include "logger.h"

#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>


static int read_from_switch	        PROTO((Connection *conn,
				        char *buffer,
				        int len));
static Boolean poll_switch		PROTO((Connection *conn,
					       int timeout));
static void send_sw_password		PROTO((Connection *conn));



/*
 *	Mamimum times we send a password to a switch.
 */
#define	MAX_PASSWORD_SENDS		5

/*
 *	Seconds to wait for output from switch
 */
#define SELECT_TIMEOUT			2	


/*
 *	Handle switch input.
 */
void
handle_switch_input(conn)
     Connection			*conn;
{
  char				buffer[BUFFERLEN];
  int				bytes_read;
  Boolean			done = FALSE;
  FILE				*to_logcmd = NULL;

  while (!done) {

    bytes_read = read_from_switch(conn, buffer, BUFFERLEN);

    if ((bytes_read == ERROR) || (bytes_read == 0)) {
      done = TRUE;
      continue;
    }

    /*
     *	Send string to client if connected
     */
    if (conn->client_sock != CLOSED_SOCK) {
      write_to_client(conn, buffer, bytes_read);

      /*
       * Log it, unless client has asked otherwise.
       */
      if (!(conn->flags & HIPPISWD_FLG_NOLOG))
	  log_message(conn->sw->sw_name, buffer);

    } else {
      /*
       * Always log it if not connected.
       */
      log_message(conn->sw->sw_name, buffer);

      /*
       * Ignore NULL or carriage-return only strings.
       */
      if (strlen(buffer) < 2)
	continue;
      

      if (conn->log_unexpected) {
	if (to_logcmd == NULL)	  	    
	  to_logcmd = open_log_cmd(conn->sw->sw_name);
	
	if (to_logcmd != NULL)
	  fprintf(to_logcmd, buffer);
	  
      } else {
	/*
	 * See if this is the start of log string.
	 */
	int len = strlen(conn->sw->sw_start_log);
	
	if (len > 0)
	  conn->log_unexpected =
	    (strncmp(buffer, conn->sw->sw_start_log, len) == 0);
      }
    }

    /*
     *	Check for password prompt
     */
    if (is_password_prompt(buffer)) {
      send_sw_password(conn);
      done = TRUE;		/* XXX - should we keep reading here? */
      continue;
    }

    if (is_prompt(conn, buffer)) {
      /* If we have not already sent the init string then do so now, otherwise
       * we wait for user input.
       */
      if (conn->sent_init) {
        conn->got_prompt = TRUE;
        done = TRUE;

      } else {
	sw_init(conn);
	conn->sent_init = TRUE;
      }
    }
  }

  if (to_logcmd)
    close_log_cmd(to_logcmd);
}



/*
 *	Handle input from client
 */
void
handle_client_input(conn)
     Connection			*conn;
{
  char 				buffer[BUFFERLEN];
  int				bytes_read;

  if (fgets(buffer, BUFFERLEN, conn->client_in) == NULL) {
    /*	EOF 	*/
    log_message(conn->sw->sw_name, "Client disconnected\n");
    close_client_conn(conn);
    return;
  }

  write_to_switch(conn, buffer, strlen(buffer));

  conn->got_prompt = FALSE;
  
  conn->last_input_time = time(NULL);
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
   *  If last read attempt returned an empty buffer, then poll
   *  for data first.
   */
  if (conn->last_status == TELNET_EOB)
    if (poll_switch(conn, 0) == FALSE)
      return 0;

  while(!done) {
    byte = telnet(conn->sw_sock, &conn->telnet_state);

    conn->last_status = byte;

    switch(byte) {
      
    case 0:
      continue;			/* Ignore nulls	*/

    case TELNET_EOF:
      close_switch_conn(conn);
      syslog(SYSLOG_SW_EOF, "Get EOF from %s.\n", conn->sw->sw_name);
      log_message(conn->sw->sw_name, "Got EOF.\n");
      if (conn->client_state == CONNECTION_ESTABLISHED) {
		  char buffer[BUFFERLEN];

		  /* XXX - try to reconnect */
		  sprintf(buffer,
				  "\nDaemon lost connection to %s.\n",
				  conn->sw->sw_name);
		  write_to_client(conn, buffer, strlen(buffer));
		  close_client_conn(conn);
      }
      return ERROR;

    case TELNET_EOB:
      if (poll_switch(conn, SELECT_TIMEOUT) == FALSE)
		  done = TRUE;
      break;

    case TELNET_OVERFLOW:
      log_message("telnet", "telnet buffer overflow!\n");
      syslog(SYSLOG_DIED, "Telnet buffer overflow. Dying.");
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
 *	Poll a switch for input.
 *
 *	Returns TRUE if data is waiting, FALSE otherwise.
 */
static Boolean
poll_switch(conn, wait_time)
     Connection			*conn;
     int			wait_time;
{
  struct timeval	timeout;
  fd_set      		readfds;
  static int		max_fd = -1;

  
  if (max_fd == -1)
    max_fd = (int) ulimit(4, 0);

  FD_ZERO(&readfds);
  FD_SET(conn->sw_sock, &readfds);
  
  timeout.tv_usec = 0;
  timeout.tv_sec = wait_time;

  
  if (select(max_fd, &readfds, NULL, NULL, &timeout) < 0) {
    log("select() error %d\n", errno);
    return FALSE;
  }
  
  return FD_ISSET(conn->sw_sock, &readfds);
}



/*
 *	Send the password to a switch.
 */
static void
send_sw_password(conn) 
     Connection			*conn;
{
  char				*passwd;
  char				buffer[BUFFERLEN];


  passwd = conn->sw->sw_password;

  if (strlen(passwd) == 0)
    passwd = password_config.default_sw_password;

  if (strlen(passwd) == 0)
    return;		/* No password to send		*/


  if ((conn->passwd_count)++ > MAX_PASSWORD_SENDS) {
    log("Reached maximum number of password sends to %s. Giving up.\n",
	conn->sw->sw_name);
    
    if (conn->client_state == CONNECTION_ESTABLISHED) {
      char buffer[BUFFERLEN];
      
      sprintf(buffer,
	      "\nSorry maximum password send count to %s reached.\n",
	      conn->sw->sw_name);

      write_to_client(conn, buffer, strlen(buffer));
      
      close_client_conn(conn);
    }

    syslog(SYSLOG_SW_FAILED_CONN,
	   "Cannot log onto %s due to bad password.",
	   conn->sw->sw_name);

    close_switch_conn(conn);
    conn->switch_state = CONNECTION_FAILED;	
    conn->conn_fail_logged = TRUE;
    return;
  }

  log("Sending password to %s.\n", conn->sw->sw_name);
 
  strcpy(buffer, passwd);

  write_to_switch(conn, buffer, strlen(buffer));
}

