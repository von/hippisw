/*
 *	handle_input.c
 *
 *	Routines that handle input from client and switch.
 */


#include "basic_defines.h"
#include "connections.h"
#include "password_config.h"
#include "prompt.h"

#include <sys/types.h>
#include <time.h>
#include <sys/time.h>


static void send_sw_password		PROTO((Connection *conn));

/*
 *	Mamimum times we send a password to a switch.
 */
#define	MAX_PASSWORD_SENDS		5


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


  while (!done) {

    bytes_read = read_from_switch(conn, buffer, BUFFERLEN);

    if ((bytes_read == ERROR) || (bytes_read == 0)) {
      done = TRUE;
      continue;
    }

    /*
     *	Send string to client if connected, otherwise log it.
     */
    if (conn->client_sock != CLOSED_SOCK)
      write_to_client(conn, buffer, bytes_read);

    else {
      if (buffer[strlen(buffer) - 1] != '\n')
	log_message(conn->sw->sw_name, "%s\n", buffer);
      else
	log_message(conn->sw->sw_name, buffer);
    }

    /*
     *	Check for password prompt
     */
    if (is_password_prompt(buffer)) {
      send_sw_password(conn);
      done = TRUE;
      continue;
    }
    

    if (is_prompt(conn, buffer)) {
      conn->got_prompt = TRUE;
      done = TRUE;
    }
  }
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
    close_switch_conn(conn);
    conn->switch_state = CONNECTION_FAILED;	
    return;
  }

  log("Sending password to %s.\n", conn->sw->sw_name);
 
  strcpy(buffer, passwd);

  write_to_switch(conn, buffer, strlen(buffer));
}
