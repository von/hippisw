/*
 *	logger.c
 *
 *	Loging routines.
 */

#include "basic_defines.h"
#include "logger.h"
#include "time_string.h"
#include "daemon_config.h"

#include <stdio.h>
#include <errno.h>
#include <syslog.h>


static FILE	*log_file;

static char	*default_name;


/*
 *	Open the log file.
 *
 *	Returns ERROR or NO_ERROR.
 */
int
open_log_file(filename, myname)
	char		*filename;
	char		*myname;
{
	log_file = fopen(filename, "a");

	if (log_file == NULL)
		return ERROR;

	default_name = myname;
}



/*
 *	Write a message to the log file.
 */
void
log_message(myname, string, arg1, arg2, arg3, arg4, arg5, arg6)
	char		*myname;
	char		*string;
	char		*arg1;
	char		*arg2;
	char		*arg3;
	char		*arg4;
	char		*arg5;
	char		*arg6;
{
	fprintf(log_file, "%s %s: ", time_string(CURRENT_TIME), myname);
	fprintf(log_file, string,  arg1, arg2, arg3, arg4, arg5, arg6);
	if (string[strlen(string) - 1] != '\n')
		fprintf(log_file, "\n");
	fflush(log_file);
}


/*
 *	Write a message to the log file using default name.
 */
void
log(string, arg1, arg2, arg3, arg4, arg5, arg6)
	char		*string;
	char		*arg1;
	char		*arg2;
	char		*arg3;
	char		*arg4;
	char		*arg5;
	char		*arg6;
{
	log_message(default_name, string, arg1, arg2, arg3, arg4, arg5, arg6);
	return;
}


/*
 *	Close log file
 */
void
close_log_file()
{
	fclose(log_file);
}



/*
 *      Open log command.
 *
 *	Returns a open stream to stdin of log command or NULL on error.
 *
 */
FILE *
open_log_cmd(switch_name)
	char		*switch_name;
{
	FILE		*to_cmd;
	int			pipe_des[2];
	int			pid;
	char		*argv[3];


	/* Is there a command to run? */
	if (strlen(daemon_config.log_command) == 0)
		return NULL;

	if (pipe(pipe_des) == -1) {
		log_message("open_log_cmd()", "pipe() failed. errno = %d.\n", errno);
		return NULL;
	}

	pid = fork();

	if (pid == -1) {
		log_message("open_log_cmd()", "fork() failed. errno = %d.\n", errno);
		close(pipe_des[0]);
		close(pipe_des[1]);
		return NULL;
	}

	if (pid == 0) {	/* Child */
		if(dup2(pipe_des[0], fileno(stdin)) == -1) {
			log_message("open_log_cmd()", "dup2() failed. errno = %d.\n", errno);
			close(pipe_des[0]);
			close(pipe_des[1]);
			exit(1);
		}

		close(pipe_des[0]);
		close(pipe_des[1]);

		argv[0] = daemon_config.log_command;
		argv[1] = switch_name;
		argv[2] = NULL;
    
		execv(daemon_config.log_command, argv);

		log_message("open_log_cmd()",
					"Exec of log command (\"%s\") failed. errno = %d.\n",
					daemon_config.log_command, errno);

		exit(1);
	}

	/* Parent */
	close(pipe_des[0]);

	to_cmd = fdopen(pipe_des[1], "w");

	if (to_cmd == NULL) {
		log_message("open_log_cmd()", "fdopen() failed. errno = %d.\n", errno);
		close(pipe_des[1]);
	}

	return to_cmd;
}


/*
 *	Close pipe to log function.
 */
void
close_log_cmd(to_cmd)
	FILE		*to_cmd;
{
	fclose(to_cmd);
	close(fileno(to_cmd));
}
  


/*
 *	Initialize syslog.
 */
void
init_syslog(myname)
	char		*myname;
{
	/* In case this is a restart and it's already open. */
	closelog();

	openlog(myname, SYSLOG_OPTS, SYSLOG_FACILITY);
}
