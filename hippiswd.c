/*
 *	hippiswd.c
 */

#include "basic_defines.h"
#include "logger.h"
#include "connections.h"
#include "client_request.h"
#include "handle_input.h"
#include "read_config.h"
#include "daemon_config.h"
#include "time_string.h"
#include "prompt.h"
#include "ip_addr.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/termios.h>
#include <strings.h>

#ifdef HIPPISWD_MAKE_CORE
#include <sys/resource.h>
#endif


static void usage		PROTO((char *myname));
static void background		PROTO((VOID));
static void register_signals	PROTO((VOID));
static void signal_catcher	PROTO((int signum));
static int create_server_sock	PROTO((VOID));
static int accept_new_client	PROTO((int server_sock));
static int read_pid_file	PROTO((VOID));
static void make_pid_file	PROTO((VOID));
static void remove_pid_file	PROTO((VOID));
void graceful_death		PROTO((int exit_status));
void restart			PROTO((VOID));
static void cleanup		PROTO((char *reason));
static void check_host		PROTO((char *specified_host,
				       char *actual_host));


#define HIPPISWD_PID_FILE	"hippiswd.pid"

/*
 *	Used by restart();
 */
static char			**hippiswd_argv;



main(argc, argv)
     int		argc;
     char		**argv;
{
  char			*config_file = NULL;
  char			*passwd_file = NULL;

  char			hostname[HNAMELEN];

  int			c;
  extern int		optind;
  extern char		*optarg;
  int			errflg = 0;

  Boolean		debug = FALSE;
  Boolean		force_start = FALSE;

  char			*myname;

  int			server_sock = CLOSED_SOCK;

  Boolean		done = FALSE;

  int			max_fd;			/* For select()		*/

  int			pid;

  struct timeval	*select_timeout, timeout;


  /*	Save for restart()		*/
  hippiswd_argv = argv;

  /*	Chop off path from name		*/
  myname = rindex(argv[0], '/');

  if (myname == NULL)
    myname = argv[0];
  else
    myname++;

  /*	Process arguments		*/
  while ((c = getopt(argc, argv, "c:dfv")) != EOF)
    switch(c) {

    case 'c':
      config_file = optarg;
      break;

    case 'd':
      debug = TRUE;
      fprintf(stderr, "%s: Running in debug mode.\n", myname);
      break;

    case 'f':
      force_start = TRUE;
      break;

    case 'v':
      print_version_info();
      exit(0);

    default:
      errflg++;
    }

  if (optind < argc)
    fprintf(stderr, "%s: Ignoring extra arguments starting with %s\n",
	    myname, argv[optind]);

  if (errflg) {
    usage(myname);
    exit(0);
  }
  
  read_config(config_file, &config_file);

  /*
   *	Change to working directory
   */
  if (chdir(daemon_config.working_dir) < 0) {
    fprintf(stderr, "Couldn't chdir to %s\n", daemon_config.working_dir);
    perror("chdir()");
    exit(1);
  }

  /*
   *	Check for existing pid file. If there is one and we're
   *	not in force_start state, then abort.
   */
  if (((pid = read_pid_file()) != ERROR) && (!force_start)) {
    fprintf(stderr, "File %s already exists. Daemon with pid %d possibly\n",
	    HIPPISWD_PID_FILE, pid);
    fprintf(stderr, "already running. If not remove file %s in working directory.\n",
	    HIPPISWD_PID_FILE);
    exit(1);
  }

  /*
   *	Read password configuration.
   */
  passwd_conf(&passwd_file);
    
  if (open_log_file(daemon_config.log_file, myname) == ERROR) {
    fprintf(stderr, "Couldn't open log file \"%s\".\n",
	    daemon_config.log_file);
    exit(1);
  }

  init_syslog(myname);

  /*	Initialize table of connections			*/
  alloc_conn_table();
  
  register_signals();

  server_sock = create_server_sock();
  
  max_fd = (int) ulimit(4, 0);

  /*	Time in between attempts to establish connections to a switch	*/
  timeout.tv_usec = 0;
  timeout.tv_sec = 15;

  /*
   *	Background ourselves.
   */
  if (!debug)
    background();

  make_pid_file();

  if (gethostname(hostname, sizeof(hostname)) < 0)
    strcpy(hostname, "Unknown");

  log("\n");
  log("******************************\n");
  if (debug)
    log("hippiswd started in debug mode\n");
  else
    log("hippiswd started\n");
  log("On host %s with pid %d\n", hostname, getpid());

  check_host(daemon_config.hostname, hostname);

  log("Listening for clients on port %d\n", daemon_config.daemon_port);
  log("Working directory is %s\n", daemon_config.working_dir);
  log("Read config file: %s\n", config_file);
  if (passwd_file != NULL)
    log("Read password file: %s\n", passwd_file);
  log("\n");


  /*
   *		MAIN LOOP
   */
  while (!done) {
    fd_set		readfds, exceptfds;
    Connection		*conn;
    int			conn_num;
    static int		client_sock = CLOSED_SOCK;


    /*
     *	Try to connect to any switches we aren't yet connected to.
     *
     *	Poll all the following for input:
     *	1. The HIPPI switch connections.
     *	2. Current active clients.
     *	3. Server port.
     *	4. New client (client_sock) trying to connect.
     *
     *	If unable to connect to one or more switches set a timeout
     *	on select to try again.
     */
    FD_ZERO(&readfds);

    select_timeout = NULL;	/* Wait indefinitely by default	*/

    FOR_ALL_CONNECTIONS(conn, conn_num) {
      
       if ((conn->switch_state == NO_CONNECTION) && (!debug))
	 if (open_switch_conn(conn) == ERROR)
	   select_timeout = &timeout;

       if (conn->switch_state == CONNECTION_ESTABLISHED)
	 FD_SET(conn->sw_sock, &readfds);

       if (conn->client_state == CONNECTION_ESTABLISHED)
	 FD_SET(conn->client_sock, &readfds);
     }

    FD_SET(server_sock, &readfds);
    
    if (client_sock != CLOSED_SOCK)
      FD_SET(client_sock, &readfds);
    
    /* Also check for exceptions. */
    bcopy(&readfds, &exceptfds, sizeof(exceptfds));

    if (select(max_fd, &readfds, NULL, &exceptfds, select_timeout) < 0) {
      log("select() error (errno = %d)\n", errno);
      syslog(SYSLOG_DIED, "select() failed (%m). Exiting.");
      graceful_death(1);
    }

    FOR_ALL_CONNECTIONS(conn, conn_num) {

      if ((conn->client_state == CONNECTION_ESTABLISHED) &&
	  (FD_ISSET(conn->client_sock, &readfds) ||
	  FD_ISSET(conn->client_sock, &exceptfds)))
	handle_client_input(conn);
      
      if ((conn->switch_state == CONNECTION_ESTABLISHED) &&
	  (FD_ISSET(conn->sw_sock, &readfds) ||
	  FD_ISSET(conn->sw_sock, &exceptfds)))
	handle_switch_input(conn);
    }


    if ((client_sock != CLOSED_SOCK) &&
	(FD_ISSET(client_sock, &readfds) ||
	 FD_ISSET(client_sock, &exceptfds))) {
      handle_client_request(client_sock);
      client_sock = CLOSED_SOCK;
    }
    
    if (FD_ISSET(server_sock, &readfds) && (client_sock == CLOSED_SOCK))
      client_sock = accept_new_client(server_sock);
  }
}
    


/*
 *	Print usage
 */
static void
usage(myname)
     char		*myname;
{
  fprintf(stderr, "Usage: %s <options>\n\n", myname);
  fprintf(stderr, "  Options:\n");
  fprintf(stderr, "\t-c <config file>   Specify configuration file to use.\n");
  fprintf(stderr, "\t-d                 Debug mode.\n");
  fprintf(stderr, "\t-f                 Force start (don't die unless we really have to).\n");
  fprintf(stderr, "\t-v                 Print version information and quit.\n");
}



/*
 *    	Go into the background and close down stdin, stdout, stderr.
 */
static void
background()
{
  int		null_fd;
  int		tty_fd;


  switch(fork()) {
  case -1:		/* ERROR */
    perror("fork()");
    exit(1);

  case 0:		/* CHILD */
    break;
    
  default:		/* PARENT */
    exit(0);
  }

  /* CHILD */
  null_fd = open("dev/null", O_RDWR);

  dup2(null_fd, fileno(stdin));
  dup2(null_fd, fileno(stdout));
  dup2(null_fd, fileno(stderr));
  
  close(null_fd);
  
  tty_fd = open("/dev/tty", O_RDWR);
  
  if (tty_fd >= 0) {
    ioctl(tty_fd, TIOCNOTTY, 0);
    close(tty_fd);
  }
  
  return;
}


/*
 *	Register all of our signals.
 */
static void
register_signals()
{
  int			signum;
  int			rc;


#ifdef HIPPISWD_MAKE_CORE
  /*
   *	Set the limit so that we can create a core file.
   */

  struct rlimit core_limit;

  if (getrlimit(RLIMIT_CORE, &core_limit) != 0) {
    perror("getrlimit(RLIMIT_CORE)");

  } else {
    core_limit.rlim_cur = core_limit.rlim_max;

    if (setrlimit(RLIMIT_CORE, &core_limit) != 0)
      perror("setrlimit(RLIMIT_CORE)");
  }
#endif


  for (signum = 1; signum < NSIG; signum++) {
    switch (signum) {
    case SIGKILL:		/* Can't register       */
    case SIGSTOP:		/* Can't register       */
      continue;

#ifdef HIPPISWD_MAKE_CORE    
    case SIGBUS:
    case SIGSEGV:
      rc = (int) signal(signum, SIG_DFL);
      break;
#endif

    case SIGTTOU:		/* Ignore the following */
    case SIGTTIN:
    case SIGPIPE:
    case SIGTSTP:
    case SIGURG:
    case SIGCHLD:
      rc = (int) signal(signum, SIG_IGN);
      break;
      
    default:			/* catch */
      rc = (int) signal(signum, signal_catcher);
      break;
    }
    
    if(rc < 0)       {
      fprintf(stderr, "Couldn't register signal %d\n", signum);
      perror("signal()");
      exit(1);
    }
  }
}


/*
 *	Signal catcher routine
 */
static void
signal_catcher(signum)
     int		signum;
{
  switch (signum) {

  case SIGTERM:
    log("Killed.\n");
    syslog(SYSLOG_KILLED, "Caught terminate signal. Dying.");
    graceful_death(0);
    
  case SIGINT:
    log("Interrupted.\n");
    syslog(SYSLOG_KILLED, "Caught interrupt signal. Dying.");
    graceful_death(0);

  case SIGBUS:
    log("Bus error, bye-bye!\n");
    syslog(SYSLOG_DIED, "Bus error. Dying.");
    graceful_death(0);
    
  case SIGSEGV:
    log("Segment Violation, bye-bye!\n");
    syslog(SYSLOG_DIED, "Segment violation. Dying.");
    graceful_death(0);
    
  default:
    log("Signal %d caught.\n", signum);
    syslog(SYSLOG_DIED, "Caught signal %d. Dying.", signum);
    graceful_death(0);
  }
}

  
    
/*
 *	Create the server socket
 */
static int
create_server_sock()
{
  struct sockaddr_in		 addr;
  int				sock;
  int				reuseaddr = 1;

  
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket()");
    exit(1);
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(daemon_config.daemon_port);

  /*
   *	Make it so we can reuse address for quick restarts.
   */
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
	     &reuseaddr, sizeof(reuseaddr));
  
  if (bind(sock, &addr, sizeof(addr)) == -1) {
    perror("bind()");
    exit(1);
  }

  if (listen(sock, 5) == -1) {
    perror("listen()");
    exit(1);
  }

  return sock;
}


/*
 *	Accept a connection on the server socket.
 */
static int
accept_new_client(server_sock)
     int			server_sock;
{
  int				client_sock;

  
  client_sock = accept(server_sock, NULL, NULL);

  if (client_sock == -1) {
    log("Error accepting new client (errno = %d)\n", errno);
    return CLOSED_SOCK;
  }

  return client_sock;
}

  

/***********************************************************************
 *	Routines dealing with the process id file
 **********************************************************************/


/*
 *	Opens and reads pid file.
 *
 *	Returns pid from file, or ERROR if an error occurs.
 */
static int
read_pid_file()
{
  FILE		*pid_file;
  int		pid;

  pid_file = fopen(HIPPISWD_PID_FILE, "r");
  
  if (pid_file == NULL)
    return ERROR;

  fscanf(pid_file, "%d", &pid);

  return pid;
}


/*
 *	Make the pid file.
 */
static void
make_pid_file()
{
  FILE		*pid_file;


  pid_file = fopen(HIPPISWD_PID_FILE, "w");

  if (pid_file == NULL) {
    
    log("Error opening hippiswd.pid (errno = %d)\n", errno);

  } else {

    fprintf(pid_file, "%d", getpid());
    fclose(pid_file);
  }
}


/*
 *	Remove pid file.
 */
static void
remove_pid_file()
{
  unlink(HIPPISWD_PID_FILE);
}


/*
 *	Die gracefully.
 *
 *	Does not return.
 */
void
graceful_death(exit_status)
     int		exit_status;
{
  cleanup("Daemon dying.");

  exit(exit_status);
}


/*
 *	Restart with original arguments.
 */
void
restart()
{
  char			hippiswd_cmd[PATHLEN];


  cleanup("Daemon restarting.");
 
  /*
   * If the binary path starts with a '/' then don't append working directory.
   */
  if (*(daemon_config.binary_path) == '/')
    strcpy(hippiswd_cmd, daemon_config.binary_path);
  else
    sprintf(hippiswd_cmd, "%s/%s",
	    daemon_config.working_dir, daemon_config.binary_path);

  execv(hippiswd_cmd, hippiswd_argv);

  fprintf(stderr, "Exec of %s failed.\n", hippiswd_cmd);
  perror("execv()");

  syslog(SYSLOG_DIED, "Restart failed: %m");

  exit(0);
}


/*
 *	Clean up before death or restart
 */
static void
cleanup(reason)
     char		*reason;
{
  Connection		*conn;
  int			conn_num;

  remove_pid_file();

  FOR_ALL_CONNECTIONS(conn, conn_num) {
    close_switch_conn(conn);

    if (conn->client_state == CONNECTION_ESTABLISHED) {
      /* XXX - need CR? */
      write_to_client(conn, reason, strlen(reason));
      close_client_conn(conn);
    }
  }
  
  log("Closing log file.\n");
  
  close_log_file();
}


/*
 *	Check that we're running on the correct host and print a
 *	warning to the log if we're not.
 */
static void
check_host(specified_host, actual_host)
     char		*specified_host;
     char		*actual_host;
{
  if (hostname_to_netaddr(specified_host) !=
      hostname_to_netaddr(actual_host))
    log("Warning: Host %s specified in config file.\n", specified_host);
}
	
