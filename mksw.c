/*
 *	mksw	Generate the configuration commands for a HIPPI switch
 *
 *	The commands are written to standard output.
 *
 *
 *	$Id: mksw.c,v 1.3 1995/04/06 22:23:51 vwelch Exp $
 * 
 */

#define BUILD_TYPE_STRINGS

#include "basic_defines.h"
#include "switch.h"
#include "port.h"
#include "read_config.h"
#include "sw_output.h"
#include "find.h"
#include "switchlist.h"
#include "path.h"
#include "switch_map.h"
#include "address_map.h"
#include "address_config.h"


static void doswitch	PROTO((SWITCH *sw));
static void usage	PROTO((VOID));


main(argc, argv)
     int		argc;
     char		**argv;
{
  FILE *conf_file;
  FILE *err_file = stderr;
  FILE *out_file = stdout;
  SWITCH *sw, *mysw = NULL;
  char *cfn = NULL;
  char *swname = NULL;
  
  register int c, i;
  extern int optind;
  extern char *optarg;
  int errflg = 0;
  
  /*
   * Crack the command options
   */
  while ( ( c = getopt ( argc, argv, "c:tv" ) ) != EOF)  {
    switch ( c )	{

    case 'c':			/* string parameter		*/
      cfn = optarg;
      break;

    case 't':			/* Don't do comments */
      sw_no_comments();
      break;

    case 'v':
      print_version_info();
      exit(0);

    default:
      errflg = TRUE;
    }
  }

  if(optind < argc)	swname = argv[optind++];
  
  if(optind < argc) 
    fprintf(stderr, "%s: Ignoring extra arguments\n", argv[0]);
  
  if(errflg)	{
    usage();
    exit(1);
  }
  
  
  read_config(cfn, &cfn);


  /* Set where output from the sw_output commands should go.
   */
  set_switch_output_stream(out_file);
  
  if(swname)	{ 		/* Switch specified */
    /*
     * Find the indicated switch in the table.
     */
    mysw = find_switch_by_name(swname, FIND_DEFAULT);
    if(!mysw)	{
      fprintf(stderr,
	      "Switch %s not found in configuration\n",
	      swname);
      exit(1);
    } else if(!mysw->sw_is_smart)	{
      fprintf(stderr, "Switch %s is not programmable\n",
	      swname);
      exit(1);
    }

    doswitch(mysw);
  } else	{		/* ELSE NO -s, DO ALL SWITCHES */
    
    FOR_ALL_SWITCHES(sw) {
      if(sw->sw_is_smart && sw->sw_hostname[0] != '\0')	{
	mysw = sw;
	doswitch(mysw);
      }
    }
    if(!mysw)	{
      fprintf(stderr,
	      "There are no intelligent switches configured\n");
      exit(1);
    }
    
  }
  exit(0);
}


/*
 * Output the commands for one switch.
 */
static void doswitch(mysw)
     SWITCH		*mysw;
{
  PORT		*sp;
  int		portnum;
  PATH		*to_tester;	/* Ports to tester			*/
  PATH		*local_port;	/* List consisting of only local port 	*/
  PATH		*to_dest;	/* Either to switch or local_port	*/
  SWITCH	*sw;
  
  SWITCH	*tester_sw = NULL;
  PORT		*tester_port = NULL;
  
  LOGICAL_MAP	*map;


  if (set_output_switch(mysw) == NULL)
    return;
  
  to_tester = malloc_path();
  local_port = malloc_path();
  
  tester_port = addr_config.tester_port;
  if (tester_port)
    tester_sw = tester_port->swp_switch;
  
  
  sw_comment("==================================\n");
  sw_comment("\n");
  sw_comment("\n");
  sw_comment("\tDestination lookup table setup\n");
  sw_comment("\n");
  
  sw_comment("\t    Switch %s (%d-port %s)\n",
	     mysw->sw_name,
	     mysw->sw_num_ports,
	     switch_types[mysw->sw_type]);
  sw_comment("\n");
  
  sw_comment(" Disable Ports (if needed)\n");
  disable_all_ports();
  sw_comment("\n");

  sw_comment(" Remove all old mappings\n");
  clear_all_pathways();
	
  if (tester_port != NULL) {

    /* Determine our path to the tester */
    if (mysw == tester_sw)
      to_tester = add_port_to_path(to_tester, tester_port);
    else
      to_tester = find_path(mysw, tester_sw);
			
    if (path_empty(to_tester))
      sw_comment("\tNOTE: switch has no path to tester.\n");
  }

  FOR_ALL_SWITCHES(sw) {
    
    sw_comment("\n");
    sw_comment("----------------------------------\n");
    sw_comment("\n");
    sw_comment(" Routes to Switch %s\n", sw->sw_name);
    sw_comment("\n");
    
    if (sw == mysw) {		/* To myself */

      to_dest = local_port;

    } else {			/* To another switch */
      PATH		*path;
      
      path = find_path(mysw, sw);
      
      if (path_empty(path)) {
	sw_comment(" ** NO PATH **\n\n");
	continue;
      }
      
      to_dest = path;
    }

    if (tester_port && !path_empty(to_tester)) {
      /*
       * Tester loopback address to that switch
       */

      sw_comment("\n");
      sw_comment("----------------------------------\n");
      sw_comment("\n");
      sw_comment(" Tester loopback paths\n");

      if (sw == mysw) {
	/* This is the destination -
	 * go back to tester
	 */
	set_pathway_all(get_tester_address(sw),
			to_tester);

      } else {
	/* Otherwise we send it on to the switch
	 * that the test address is for...
	 */
	set_pathway_all(get_tester_address(sw),
			to_dest);
			  
	/* ...unless the connection is coming
	 * from the switch in which case we
	 * go back to tester.
	 */
			  
	FOR_EACH_PORT_IN_PATH(to_dest, sp, portnum) {
	  set_pathway(sp->swp_num,
		      get_tester_address(sw),
		      to_tester);
	}
      }
    }

    FOR_EACH_PORT(sw->sw_ports, sp, portnum) {

      sw_comment("\n");
      sw_comment(" Port %4d\t%s\n", sp->swp_num, sp->swp_name);

      if (sw == mysw)		/* Local */
	local_port = better_path(local_port, sp, INFINITE_COST);

      FOR_ALL_LOGICAL_MAPS_BY_PORT(map, sp) {

	if (map->duplicate_logaddr == TRUE)
	  continue;
				
	set_pathway_all(map->logaddr, to_dest);
      }

      if ((addr_config.loopback_addr != LOGADDR_NULL) &&
	  (sw == mysw) && 
	  DO_LOOPBACK(sp)) {
	/* Loopback address to self */
	set_pathway(sp->swp_num, addr_config.loopback_addr, local_port);
      }
    }
  }
  
  sw_comment("\n");
  sw_comment("\n");
  save_all_pathways();
  enable_all_ports();
  sw_comment("\n");
  sw_comment("\n");
  
  free(to_tester);
  free(local_port);
}



static void
usage()
{
  fprintf(stderr, "usage: mksw <options> [<switch name>]\n\n");
  fprintf(stderr, "  Options:\n");
  fprintf(stderr, "\t-c <config file>   Specify configuration file to use.\n");
  fprintf(stderr, "\t-C                 Terse mode, don't print comments.\n");
  fprintf(stderr, "\t-v                 Print version information and quit.\n");
}
