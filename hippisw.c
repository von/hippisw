/*
 *	hippisw.c
 *
 */

#include "basic_defines.h"
#include "client_request.h"
#include "read_config.h"
#include "daemon_config.h"
#include "switchlist.h"
#include "switch.h"
#include "parse_token.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>


static void usage					PROTO((char *name));
static void handle_command			PROTO((CLIENT_PACKET *request,
										   char *daemon_host,
										   int daemon_port));
static void handle_logon			PROTO((CLIENT_PACKET *request,
										   char *daemon_host,
										   int daemon_port,
										   char *input_filename));
static int login_to_daemon			PROTO((CLIENT_PACKET *request,
										   char *daemon_host,
										   int daemon_port));

static int handle_daemon_input		PROTO((int daemon_fd,
										   FILE *output_file,
										   Boolean *got_prompt));

static Boolean handle_user_input  	PROTO((FILE* input_file,
										   Boolean *connected,
										   int daemon_fd,
										   Boolean *got_prompt,
										   CLIENT_PACKET *request));

static void init_request			PROTO((CLIENT_PACKET *request));

static void get_password			PROTO((CLIENT_PACKET *request));



/*
 *	Return codes for login_to_daemon
 */
#define	LOGIN_OK			0
#define	LOGIN_NO_CONNECT	-1
#define LOGIN_BAD_CONNECT	-2
#define LOGIN_BAD_SWITCH	-3
#define LOGIN_BAD_PASSWD	-4
#define LOGIN_NO_PERM		-5




main(argc, argv)
	int			argc;
	char		**argv;
{
	char			*daemon_host = NULL;
	int				daemon_port = -1;
	char			*config_file = NULL;

	char			*input_filename = NULL;
  
	int				arg;
	extern int		optind;
	extern char		*optarg;
	int				errflg = FALSE;
 
	Boolean			request_password = FALSE;

	CLIENT_PACKET	request;



	/*
	 *	Fill in default request parameters
	 */
	init_request(&request);
	request.code = HIPPISWD_REQ_LOGON;
	request.flags = HIPPISWD_FLG_NOLOG;
	NULL_STRING(request.password);
	NULL_STRING(request.sw_name);

  
	/*
	 * Crack the command options
	 */
	while ( ( arg = getopt ( argc, argv, "c:df:h:klnp:Prsuv" ) ) != EOF)  {
		switch ( arg )	{
		case 'c':
			config_file = optarg;
			break;

		case 'd':		/* Request dump			*/
			request.code = HIPPISWD_REQ_DUMP;
			break;

		case 'h':		/* Host name parameter		*/
			daemon_host = optarg;
			break;

		case 'f':		/* Filename parameter		*/
			input_filename = optarg;
			break;

		case 'p':		/* Port number			*/
			daemon_port = str_to_int(optarg);
			break;

		case 'l':		/* Do logging			*/
			request.flags |= HIPPISWD_FLG_NOLOG;
			break;

		case 'n':		/* Suppress logging		*/
			request.flags &= ~HIPPISWD_FLG_NOLOG;
			break;

		case 's':		/* ping only			*/
			request.code = HIPPISWD_REQ_PING;
			break;

		case 'u':		/* Usurp the connection		*/
			request.flags |= HIPPISWD_FLG_USURP;
			break;

		case 'k':		/* Kill hippiswd		*/
			request.code = HIPPISWD_REQ_KILL;
			break;

		case 'r':		/* Restart hippiswd		*/
			request.code = HIPPISWD_REQ_RESTART;
			break;

		case 'P':		/* Force password request	*/
			request_password = TRUE;
			break;

		case 'v':		/* Print version		*/
			print_version_info();
			exit(0);

		default:
			errflg = TRUE;
		}
	}

	if ((optind < argc) &&
		(request.code == HIPPISWD_REQ_LOGON)) {	
		/* switch name parameter */
		strncpy(request.sw_name, argv[optind++], SWNAMELEN);
	}
  
	if(optind < argc)  {		/* not all arguments processed */
		fprintf(stderr, "%s, unrecognized argument:  %s\n", argv[0],
				argv[optind]);
		errflg = TRUE;
	}

	if (errflg) {
		usage(argv[0]);
		exit(1);
	}

	read_config(config_file, NULL);

	if (daemon_host == NULL)
		daemon_host = daemon_config.hostname;

	if (daemon_port == -1)
		daemon_port = daemon_config.daemon_port;
    
 
	/*
	 *	Verify certain operations.
	 */	
	if ((request.code == HIPPISWD_REQ_RESTART) ||
		(request.code == HIPPISWD_REQ_KILL)) {
		switch (request.code) {
		case HIPPISWD_REQ_RESTART:
			fprintf(stderr, "Really restart the daemon? ");
			break;

		case HIPPISWD_REQ_KILL:
			fprintf(stderr, "Really kill the daemon? ");
			break;
		}
    
		if (getchar() != 'y')
			exit(0);
	}

	/*
	 *	Get the passowrd if needed.
	 */
	request_password |= REQUEST_NEEDS_PASSWORD(&request);
  
	if (request_password)
		get_password(&request);
  

	/*
	 *	Do it.
	 */
	switch (request.code) {

	case HIPPISWD_REQ_LOGON:
		handle_logon(&request, daemon_host, daemon_port, input_filename);
		break;

	default:
		handle_command(&request, daemon_host, daemon_port);
		break;

	}
  
	exit(0);
}



/*
 *	Handle a simple command.
 */
static void
handle_command(request, daemon_host, daemon_port)
	CLIENT_PACKET		*request;
	char				*daemon_host;
	int					daemon_port;
{
	login_to_daemon(request, daemon_host, daemon_port);
}



/*
 *	Handle logon to daemon
 */
static void
handle_logon(request, daemon_host, daemon_port, input_filename)
	CLIENT_PACKET		*request;
	char				*daemon_host;
	int					daemon_port;
	char				*input_filename;
{
	int					daemon_fd;

	FILE				*input_file;
	FILE				*output_file = stdout;
	int					input_fd;
  
	Boolean				istty = FALSE;
	Boolean				connected = FALSE;
	Boolean				client_done = FALSE;
	Boolean				got_prompt = FALSE;

	fd_set				readfds;
	int					max_fd = ulimit(4,0);


	/*
	 *	Was an input file given?
	 */
	if (input_filename != NULL) {
		/*
		 *	Open the input file.
		 */
		input_fd = open(input_filename, O_RDONLY);

		if (input_fd == -1) {
			perror(input_filename);
			exit(1);
		}

	} else { 
		struct stat		statbuf;
		FILE		*in_file;
    
		/*
		 * Duplicate stdio and determine if it's a tty.
		 */
		input_fd = dup(0);
    
		if (input_fd < 0) {
			perror("dup(stdio)");
			exit(1);
		}
    
		if (fstat(input_fd, &statbuf) >= 0) {
			if (S_ISCHR(statbuf.st_mode))
				istty = TRUE;
    
		} else {
			perror("fstat(stdio)");
			exit(1);
		}
	}
  
	input_file = fdopen(input_fd, "r");
  
	if (input_file == NULL) {
		perror("fdopen()");
		exit(1);
	}

	if (strlen(request->sw_name) == 0) {
		SWITCH		*sw;
    
		/*
		 *	If no switchname is given, see if there is only one switch.
		 *	If so connect to that one, otherwise ask user to select one.
		 */
		if (number_of_switches() == 1) {

			sw = first_switch();
			strcpy(request->sw_name, sw->sw_name);
    
		} else {
			if (istty) {
				fprintf(stderr,
						"You are not connected to a switch. Use \"switch <switchname>\" to connect.\n");

				fprintf(stderr,
						"Switches available are:");

	
				FOR_ALL_SWITCHES(sw)
					fprintf(stderr, " %s", sw->sw_name);

				fprintf(stderr, "\n");

			}
		}      
	}

 
	/*
	 *	Main Loop.
	 */
	while (!client_done || !got_prompt) {

		/*	Try to connect if we aren't already and we have a switch name to
		 *	connect to.
		 */
		if (!connected) {

			/* Always read from client if not connected.
			 */
			got_prompt = TRUE;

			/*
			 * Attempt logon if we have a switch name.
			 */

			while ((strlen(request->sw_name) > 0) && !connected) {

				daemon_fd = login_to_daemon(request, daemon_host, daemon_port);

				if (daemon_fd < 0) {
					/* If this isn't a tty exit to prevent a bunch of commands from
					 * going to the wrong switch.
					 */
                           
					if (!istty)
						exit(1);

					switch(daemon_fd) {
					case LOGIN_NO_CONNECT:
					case LOGIN_BAD_CONNECT:
					case LOGIN_NO_PERM:
						/* These errors are likely to repeat no mater what we do, so
						 * let's just bail out.
						 */
						exit(1);

					case LOGIN_BAD_SWITCH:      
						/* Make user select another switch */
						NULL_STRING(request->sw_name);
						break;

					case LOGIN_BAD_PASSWD:
						/* Make user reenter password */
						get_password(request);
						break;
	    
					default:
						/* Just to catch any programming goofs...*/
						fprintf(stderr,
								"Oops! Reached default on switch(daemon_fd)! Dying.\n");
						exit(1);
					}
	
				} else {
					connected = TRUE;

					/*
					 *	Send a CR to the switch in order to solicit a prompt.
					 */
					write(daemon_fd, "\n", 1);
					got_prompt = FALSE;
				}
			}
		}
    
		/*
		 *	Select on daemon connection and user connection.
		 */
		FD_ZERO(&readfds);		
		if (got_prompt && !client_done)
			FD_SET(input_fd, &readfds);
		if (connected)
			FD_SET(daemon_fd, &readfds);

		if (select(max_fd, &readfds, NULL, NULL, NULL) < 0) {
			perror("select()");
			exit(1);
		}

		/*
		 *  Input from daemon?
		 */
		if (connected && FD_ISSET(daemon_fd, &readfds))
			if (handle_daemon_input(daemon_fd, output_file, &got_prompt) == ERROR)
				break;

		/*
		 *	Input from client?
		 */
		if (got_prompt && !client_done && FD_ISSET(input_fd, &readfds))
			client_done = handle_user_input(input_file, &connected,
											daemon_fd, &got_prompt,
											request);
	}

	fprintf(output_file, "\n\nSession done.\n");
}
    

/*
 *	Log in to the daemon.
 *
 *	Overwrites request with response from daemon.
 *	Returns either a socket descriptor, LOGIN_OK, or
 *	a negitive error code (see above).
 */
static int
login_to_daemon(request, daemon_host, daemon_port)
	CLIENT_PACKET		*request;
	char				*daemon_host;
	int					daemon_port;
{
	int					sock;
	struct sockaddr_in	addr;
	struct hostent		*hostent;
	CLIENT_PACKET		response_pkt, *response = &response_pkt;


	hostent = gethostbyname(daemon_host);
  
	if (hostent == NULL) {
		fprintf(stderr, "Couldn't resolve hostname \"%s\".\n", daemon_host);
		return LOGIN_NO_CONNECT;
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);

	if (sock == -1) {
		perror("socket()");
		return LOGIN_NO_CONNECT;
	}

	addr.sin_family = AF_INET;
	bcopy(hostent->h_addr, &(addr.sin_addr), hostent->h_length);
	addr.sin_port = htons(daemon_port);

	if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		fprintf(stderr, "Couldn't connect to daemon (%s port %d).\n",
				daemon_host, daemon_port);
		perror("connect()");
		close(sock);
		return LOGIN_NO_CONNECT;
	}

	init_request(request);

	if (write(sock, request, sizeof(CLIENT_PACKET)) < sizeof(CLIENT_PACKET)) {
		perror("Error writing to daemon");
		close(sock);
		return LOGIN_BAD_CONNECT;
	}

	if (read(sock, response, sizeof(CLIENT_PACKET)) < sizeof(CLIENT_PACKET)) {
		perror("Error reading from daemon");
		close(sock);
		return LOGIN_BAD_CONNECT;
	}

	switch(response->code) {

	case HIPPISWD_RSP_OK:
		printf("Login successful to %s.\n", response->sw_name);
		break;

	case HIPPISWD_RSP_BUSY:
		printf("Switch %s busy:\n\tUser %s@%s.\n\tLast input %s.\n",
			   response->sw_name, response->username, response->hostname,
			   response->idle_time);
		close(sock);
		sock = LOGIN_BAD_SWITCH;
		break;

	case HIPPISWD_RSP_NO_CONN:
		printf("Daemon has no connection to switch %s.\n",
			   response->sw_name);
		close(sock);
		sock = LOGIN_BAD_SWITCH;
		break;

	case HIPPISWD_RSP_NO_SUCH:
		printf("Unknown switch \"%s\".\n",
			   response->sw_name);
		close(sock);
		sock = LOGIN_BAD_SWITCH;
		break;

	case HIPPISWD_RSP_PING:
		printf("Daemon is alive.\n");
		close(sock);
		sock = NO_ERROR;
		break;

	case HIPPISWD_RSP_RSOK:
		printf("Command acknowledged by daemon.\n");
		close(sock);
		sock = NO_ERROR;
		break;

	case HIPPISWD_RSP_BAD_PW:
		printf("Bad password.\n");
		close(sock);
		sock = LOGIN_BAD_PASSWD;
		break;
  
	case HIPPISWD_RSP_NO_PERM:
		printf("Permission denied.\n");
		close(sock);
		sock = LOGIN_NO_PERM;
		break;

	case HIPPISWD_RSP_ERROR:
		printf("Daemon reports error in connection.\n");
		close(sock);
		sock = LOGIN_BAD_CONNECT;
		break;

	case HIPPISWD_RSP_BAD_REQ:
		printf("Request not supported by daemon.\n");
		close(sock);
		sock = LOGIN_BAD_CONNECT;
		break;

	case HIPPISWD_RSP_BAD_MAGIC:
		printf("Bad Magic String.\n");
		close(sock);
		sock = LOGIN_BAD_CONNECT;
		break;

	case HIPPISWD_RSP_DUMP:
		printf("Response to dump request:\n");

		{
			FILE	*in_stream;
			char	buffer[BUFFERLEN];

			in_stream = fdopen(sock, "r");

			if (in_stream == NULL) {
				perror("fdopen()");
				exit(1);
			}

			while (fgets(buffer, BUFFERLEN, in_stream) != NULL)
				printf(buffer);
    
			fclose(in_stream);
		}

		close(sock);
		sock = NO_ERROR;
		break;
    
	case HIPPISWD_RSP_BAD_VER:
		printf("Version mismatch between client and daemon.\n");
		close(sock);
		sock = LOGIN_BAD_CONNECT;
		break;
	  
	default:
		printf("Unknown response '%c' from daemon.\n",
			   response->code);
		close(sock);
		sock = LOGIN_BAD_CONNECT;
		break;
	}
    
	return sock;
}



/*
 *	Initialize a request packet.
 *
 *	Fills in magic string, user id, hostname, and pid.
 */
static void
init_request(request)
	CLIENT_PACKET		*request;
{
	/*
	 *	Fill in default request parameters
	 */
	strncpy(request->magic, daemon_config.magic_string, MAGICLEN);
	strncpy(request->username, cuserid(NULL), UNAMELEN);
	gethostname(request->hostname, HNAMELEN);
	sprintf(request->pid, "%d", getpid());
	strncpy(request->version, HIPPISW_VERSION, VERSIONLEN);
}



static void
usage(myname)
	char			*myname;
{
	fprintf(stderr, "usage:  %s <options> <command> [switch]\n\n", myname);
	fprintf(stderr, "  Options:\n");
	fprintf(stderr, "\t-c <config file>   Specify configuration file to use.\n");
	fprintf(stderr, "\t-h <host>          Specify host on which daemon is running.\n");
	fprintf(stderr, "\t-p <port>          Specify port on which to contact daemon.\n");
	fprintf(stderr, "\t-f <file>          Specify file to use as input.\n");
	fprintf(stderr, "\t-l                 Log session.\n");
	fprintf(stderr, "\t-n                 Don't log session (default).\n");
	fprintf(stderr, "\t-u                 Usurp any current connection.\n");
	fprintf(stderr, "\t-P                 Force password request.\n");
	fprintf(stderr, "\t-v                 Print version information and exit.\n");

	fprintf(stderr, "\n  Commands:\n");
	fprintf(stderr, "\tdefault\t\tLog on to switch.\n");
	fprintf(stderr, "\t-d                 Request dump.\n");
	fprintf(stderr, "\t-s                 Ping daemon.\n");
	fprintf(stderr, "\t-k                 Kill daemon.\n");
	fprintf(stderr, "\t-r                 Restart daemon.\n");


	fprintf(stderr, "\n\t<switch>           Name of switch to connect to.\n");
}


/*
 *	Handle input from the daemon.
 *
 *	Returns NO_ERROR or ERROR on disconnections.
 */
static int
handle_daemon_input(daemon_fd, user_file, got_prompt)
	int				daemon_fd;
	FILE			*user_file;
	Boolean			*got_prompt;
{
    DAEMON_PACKET	packet;
    int				total_read = 0;
  
    /*
     * Read packet from daemon
     */
    while (total_read < sizeof(packet)) {
		int bytes_read;

		bytes_read = read(daemon_fd,
						  &packet + total_read,
						  sizeof(packet) - total_read);

		if (bytes_read == 0) {
			fprintf(user_file, "\nDaemon closed connection.\n");
			close(daemon_fd);
			return ERROR;
		}

		if (bytes_read < 0) {
			perror("read()");
			close(daemon_fd);
			return ERROR;
		}

		total_read += bytes_read;
    }

    /*
     * Parse flags
     */
    *got_prompt =
		(packet.flags & DAEMON_PKT_GOT_PROMPT) == DAEMON_PKT_GOT_PROMPT;

    /*
     *	Echo string to the user.
     */
    fprintf(user_file, packet.string);
    fflush(user_file);

    return NO_ERROR;
}



/*
 *	Handle input from user.
 *
 *	Returns TRUE if user disconnects, FALSE otherwise.
 */
static Boolean 
handle_user_input(input_file, connected, daemon_fd, got_prompt, request)
	FILE			*input_file;
	Boolean			*connected;
	int				daemon_fd;
	Boolean			*got_prompt;
	CLIENT_PACKET	*request;
{
	char			buffer[BUFFERLEN];
	char			tmp_buf[BUFFERLEN];
	char			*token;
	char			*sep = " \t\n";
	char			*comment;
	int				length;
	int				cmd;

	enum cmd_types {
		CMD_QUIT,
		CMD_SWITCH
	};

	static struct token_mapping cmds[] = {
		{ "quit",		CMD_QUIT },
		{ "switch",		CMD_SWITCH },
		{ NULL,		0 }
	};

	if (fgets(buffer, BUFFERLEN, input_file) == NULL)
		return feof(input_file);
  
	/*
	 *	Strip comments.
	 */
	comment = strchr(buffer, COMMENT_CHAR);

	if (comment != NULL)
		*comment = '\0';

	length = strlen(buffer);

	/*
	 *	Check for commands
	 */
	strcpy(tmp_buf, buffer);

	token = strtok(tmp_buf, sep);

	/*	Was string all comment?		*/
	if ((token == NULL) && (comment != NULL))	
		return FALSE;

	if (token) {
		cmd = parse_token(token, cmds);

		if (cmd != TOKEN_NOT_FOUND) {

			switch (cmd) {
			case CMD_QUIT:
				return TRUE;

			case CMD_SWITCH:
				token = strtok(NULL, sep);
				if (token == NULL) {
					fprintf(stderr, "Switchname required for switch command.\n");

				} else {
					request->code = HIPPISWD_REQ_LOGON;
					strncpy(request->sw_name, token, SWNAMELEN);
					if (*connected) {
						*connected = FALSE;
						close(daemon_fd);
					}
				}

				return FALSE;
      
			}
		}
	}

	if (!(*connected)) {
		fprintf(stderr, "Not connected to a switch.\n");
		return FALSE;
	}
  
	write(daemon_fd, buffer, length);
  
	*got_prompt = FALSE;

	return FALSE;
}
      

/*
 *	Get the password from the user and store in the request
 *	structure.
 *
 *	exit if the user enters a null password.
 */
static void
get_password(request)
	CLIENT_PACKET		*request;
{
	extern char 		*getpass PROTO((char *prompt));

	strncpy(request->password,
			getpass("Enter hippiswd password: "),
			PASSWDLEN);

	if (strlen(request->password) == 0)
		exit(1);
}
