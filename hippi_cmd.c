/*
 *	hippi_cmd
 *
 *	Command front end for get statistics from the switches.
 *	Output meant to be piped into hippisw.
 *
 *	$Id: hippi_cmd.c,v 1.3 1996/08/20 20:20:27 vwelch Exp $
 *
 */

#include "basic_defines.h"
#include "find.h"
#include "parse_token.h"
#include "port.h"
#include "switch.h"
#include "sw_output.h"


#include <stdio.h>
#include <string.h>


enum cmd_types {		/* Commands					*/
  DISPLAY_STATS,		/* Display Stats			*/
  CLEAR_STATS			/* Clear Stats				*/
};

struct token_mapping cmds[] = {		/* Commands 			*/
  { "disp",	DISPLAY_STATS },		/* Display Stats		*/
  { "display",	DISPLAY_STATS },	/* Display Stats		*/
  { "clear",	CLEAR_STATS },		/* Clear Stats			*/
  { NULL,	0 }
};


static void	usage		PROTO((VOID));

static FILE	*errfile = stderr;
static FILE	*outfile = stdout;


/*	Undefined port number		*/
#define PORT_NOT_DEFINED		-1



main(argc, argv)
     int	argc;
     char	**argv;
{
  extern char		*optarg;
  extern int		optind, opterr;
  int				arg;

  int				arg_index;

  char				*config_file = NULL;

  char				*command;
  int				cmd_type;

  char				*what;

  int				errflg = FALSE;

  struct sw_port	*port;
  struct sw			*sw;

  FILE				*conf_file;

  int				port_num = PORT_NOT_DEFINED;


  while((arg = getopt(argc, argv, "c:v")) != -1)
	  switch(arg) {
	  case 'c':
		  config_file = optarg;
		  break;

	  case 'v':
		  print_version_info();
		  exit(0);

	  default:
		  errflg++;

	  }

  if (optind == argc)
	  errflg++;

  else
	  what = argv[optind++];

  /*
   * Check commands for errors before we start.
   */
  if (optind == argc)
	  errflg++;

  else
	  for (arg_index = optind; arg_index < argc; arg_index++)
		  if (parse_token(argv[arg_index], cmds) == TOKEN_NOT_FOUND) {
			  errflg++;
			  fprintf(errfile, "Unrecognized command: \"%s\"\n", argv[arg_index]);
		  }

  if (errflg) {
	  usage();
	  exit(1);
  }

  read_config(config_file, &config_file);

  
  /* Parse host/switch
   */

  /* Check for "<port number>@<switch>"
   */
  if (strchr(what, '@') != NULL) {
	  char swname[SWNAMELEN];

	  sscanf(what, "%d@%s", &port_num, swname);

	  if ((sw = find_switch_by_name(swname, FIND_DEFAULT)) == NULL) {
		  fprintf(errfile, "Unrecognized switch: \"%s\"\n", swname);
		  usage();
		  exit(1);
	  }

  } else {
	  /* Assume plain old host or switch name
     */
	  if ((port = find_port_by_name(what, FIND_SUBSTRING)) == NULL) {
		  if ((sw = find_switch_by_name(what, FIND_SUBSTRING)) == NULL) {
			  fprintf(errfile, "Unrecognized device or host: \"%s\"\n", what);
			  usage();
			  exit(1);
		  }
	  } else {
		  sw = port->swp_switch;
	  }
  }

  set_switch_output_stream(outfile);

  if (set_output_switch(sw) == NULL)
	  exit(1);

  if (port != NULL)
	  port_num = port->swp_num;

  while (optind < argc) {
	  command = argv[optind++];

	  cmd_type = parse_token(command, cmds);

	  switch(cmd_type) {

	  case DISPLAY_STATS:
		  if (port_num == PORT_NOT_DEFINED)
			  display_switch_stats();
		  else
			  display_port_stats(port_num);

		  break;

	  case CLEAR_STATS:
		  if (port_num == PORT_NOT_DEFINED)
			  clear_switch_stats();
		  else
			  clear_port_stats(port_num);

		  break;
	  }
  }

  fprintf(outfile, "quit\n");

}




/*
 *	Print Usage
 */
static void
usage()
{
  fprintf(errfile, "usage: hippi_cmd <options> <host or switch> <command> [<commands> ...]\n\n");
  fprintf(errfile, "  Valid commands are:\n");
  fprintf(errfile, "\tdisplay\n");
  fprintf(errfile, "\t\tDisplay stats\n");
  fprintf(errfile, "\tclear\n");
  fprintf(errfile, "\t\tClear stats\n");
  fprintf(errfile, "\n");
  fprintf(stderr, "  Options:\n");
  fprintf(stderr, "\t-c <config file>   Specify configuration file to use.\n");
  fprintf(stderr, "\t-v                 Print version information and quit.\n");
}
