/*
 *	client_request.c
 *
 *	Routines to handle client requests.
 */

#include "basic_defines.h"
#include "client_request.h"
#include "logger.h"
#include "find.h"
#include "time_string.h"
#include "daemon_config.h"
#include "password_config.h"
#include "connections.h"

#include <string.h>
#include <errno.h>


static void handle_logon	PROTO((int client_sock,
				       CLIENT_PACKET *request));
static void handle_ping		PROTO((int client_sock,
				       CLIENT_PACKET *request));
static void handle_restart	PROTO((int client_sock,
				       CLIENT_PACKET *request));
static void handle_kill		PROTO((int client_sock,
				       CLIENT_PACKET *request));
static void handle_dump		PROTO((int client_sock,
				       CLIENT_PACKET *request));
static int send_response	PROTO((int client_sock,
				       CLIENT_PACKET *response));


/*
 *	Read and process a client connection
 */
void
handle_client_request(client_sock)
     int		client_sock;
{
  CLIENT_PACKET		request;
  Boolean		password_ok = FALSE;


  if (read(client_sock, &request, sizeof(request)) != sizeof(request)) {
    log("Received badly-sized request.\n");
    request.code = HIPPISWD_RSP_ERROR;
    send_response(client_sock, &request);
    close(client_sock);
    return;
  }

  /*
   *	Check MAGIC
   */
  if (strcmp(daemon_config.magic_string, request.magic) != 0) {
    log("Received bad magic string (%s) from %s@%s.\n",
	request.magic, request.username, request.hostname);
    request.code = HIPPISWD_RSP_BAD_MAGIC;
    send_response(client_sock, &request);
    close(client_sock);
    return;
  }

  /*
   *	Check Password
   */
  if (strcmp(password_config.daemon_password, request.password) == 0)
    password_ok = TRUE;

  bzero(request.password, PASSWDLEN);


  /*
   *	Reject if password is not correct.
   */
  if (REQUEST_NEEDS_PASSWORD(&request) && !password_ok) {
    log("Received bad password from %s@%s.\n",
	request.username, request.hostname);
    request.code = HIPPISWD_RSP_BAD_PW;
    send_response(client_sock, &request);
    close(client_sock);
    return;
  }

  /*
   *	Handle request
   */
  switch(request.code) {
  case HIPPISWD_REQ_LOGON:
    handle_logon(client_sock, &request);
    break;

  case HIPPISWD_REQ_PING:
    handle_ping(client_sock, &request);
    break;

  case HIPPISWD_REQ_RESTART:
    handle_restart(client_sock, &request);
    break;

  case HIPPISWD_REQ_KILL:
    handle_kill(client_sock, &request);
    break;

  case HIPPISWD_REQ_DUMP:
    handle_dump(client_sock, &request);
    break;

  default:
    log("Received bad command (\"%c\") from %s@%s.\n",
	request.code, request.username, request.hostname);
    request.code = HIPPISWD_RSP_BAD_REQ;
    send_response(client_sock, &request);
    close(client_sock);
  }

  return;
}
   

/*
 *	Handle a logon request
 */
static void
handle_logon(client_sock, request)
     int		client_sock;
     CLIENT_PACKET	*request;
{
  SWITCH		*sw;
  Connection		*conn;

  sw = find_switch_by_name(request->sw_name, FIND_SUBSTRING);

  if (sw == NULL) {
    log("Received bad switch name (\"%s\") from %s@%s.\n",
	request->sw_name, request->username, request->hostname);
    request->code = HIPPISWD_RSP_NO_SUCH;
    send_response(client_sock, request);
    close(client_sock);
    return;
  }

  strcpy(request->sw_name, sw->sw_name);
  strcpy(request->sw_prompt, sw->sw_prompt);

  conn = get_connection(sw);

  if (conn->switch_state != CONNECTION_ESTABLISHED) {
    log("Responding no connection for switch %s to %s@%s.\n",
	conn->sw->sw_name, request->username, request->hostname);
    request->code = HIPPISWD_RSP_NO_CONN;
    send_response(client_sock, request);
    close(client_sock);
    return;
  }

  if (conn->client_state == CONNECTION_ESTABLISHED) {
    /*
     *	Connection engaged
     */

    /*
     *	Is client preempting?
     */
    if (request->flags & HIPPISWD_FLG_USURP) {
      char buffer[BUFFERLEN];

      /*
       * Close current connection.
       */
      log("%s@%s kicking %s@%s off of switch %s.\n",
	  request->username, request->hostname,
	  conn->client_name, conn->client_hostname,
	  sw->sw_name);

      sprintf(buffer,
	      "\nYour connection was preempted by %s@%s.\n",
	      request->username, request->hostname);

      write_to_client(conn, buffer, strlen(buffer));

      close_client_conn(conn);
    
    } else {
      /*
       *  Not preempting. Respond busy.
       */
      log("Responding busy to %s@%s from switch %s.\n",
	  request->username, request->hostname,
	  sw->sw_name);
      
      request->code = HIPPISWD_RSP_BUSY;
      strcpy(request->username, conn->client_name);
      strcpy(request->hostname, conn->client_hostname);
      strcpy(request->idle_time, time_string(&(conn->last_input_time)));
      send_response(client_sock, request);
      close(client_sock);
      return;
    }
  }


  /*
   *	All cleared to log on.
   */
  log("Logging %s@%s onto switch %s.\n",
      request->username, request->hostname,
      sw->sw_name);
  request->code = HIPPISWD_RSP_OK;
  if (send_response(client_sock, request) == NO_ERROR)
    new_client(conn, client_sock, request->username,
	       request->hostname, request->flags);
  return;
}



/*
 *	Handle a ping request.
 */
static void
handle_ping(client_sock, request)
     int		client_sock;
     CLIENT_PACKET	*request;
{
  log("Responding to ping from %s@%s.\n",
      request->username, request->hostname);
  request->code = HIPPISWD_RSP_PING;
  send_response(client_sock, request);
  close(client_sock);
}



/*
 *	Handle a restart request.
 */
static void
handle_restart(client_sock, request)
     int		client_sock;
     CLIENT_PACKET	*request;
{
  syslog(SYSLOG_RESTART,
	 "Restarting on command from %s@%s.\n",
	 request->username, request->hostname);
  log("Restarting on command from %s@%s.\n",
      request->username, request->hostname);
  request->code = HIPPISWD_RSP_RSOK;
  send_response(client_sock, request);
  close(client_sock);

  restart();
  /* Not reached */
}



/*
 *	Handle a kill request.
 */
static void
handle_kill(client_sock, request)
     int		client_sock;
     CLIENT_PACKET	*request;
{
  syslog(SYSLOG_KILLED,
	 "Dying on command from %s@%s.\n",
	 request->username, request->hostname);
  log("Dying on command from %s@%s.\n",
      request->username, request->hostname);
  request->code = HIPPISWD_RSP_RSOK;
  send_response(client_sock, request);
  close(client_sock);

  graceful_death(0);
}


/*
 *	Handle a dump request.
 */
static void
handle_dump(client_sock, request)
     int		client_sock;
     CLIENT_PACKET	*request;
{
  FILE *client_out;


  log("Responding to dump request from %s@%s.\n",
      request->username, request->hostname);
  request->code = HIPPISWD_RSP_DUMP;
  send_response(client_sock, request);

  client_out = fdopen(client_sock, "w");

  if (client_out == NULL) {
    log("fdopen(client_sock) failed.\n");

  } else {
    dump_connections(client_out);
    fclose(client_out);
  }

  close(client_sock);
}
  

  
/*
 *	Send a response back to client.
 */
static int
send_response(client_sock, response)
     int		client_sock;
     CLIENT_PACKET	*response;
{
  if (write(client_sock, response, sizeof(CLIENT_PACKET))
      != sizeof(CLIENT_PACKET))	{
    log("Error responding to %s@%s (errno = %d).\n",
	response->username, response->hostname, errno);
    close(client_sock);
    return ERROR;
  }

  return NO_ERROR;
}
    
