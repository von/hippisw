/*
 *	logger.c
 *
 *	Loging routines.
 */

#include "basic_defines.h"
#include "logger.h"
#include "time_string.h"

#include <stdio.h>


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
