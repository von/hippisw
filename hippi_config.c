/*
 *	hippi_config
 *
 *	Read and dump the hippisw config file.
 *
 *	$Id: hippi_config.c,v 1.1 1995/02/28 23:17:20 vwelch Exp $
 */
#include <stdio.h>

/*	Build the switch_types and port_types tables		*/
#define BUILD_TYPE_STRINGS

#include "basic_defines.h"
#include "read_config.h"
#include "daemon_config.h"
#include "address_config.h"
#include "port.h"
#include "switch.h"
#include "switchlist.h"
#include "ip_addr.h"
#include "address_map.h"
#include "path.h"
#include "switch_map.h"


static void	dump_config		PROTO((FILE *file));
static void	dump_addresses		PROTO((FILE *file));
static void	dump_netaddresses     	PROTO((FILE *file));
static void	dump_switch_mappings	PROTO((FILE *file));


#define	DUMP_CONFIG	0x001
#define	DUMP_LA_MAPPING	0x002
#define	DUMP_IP_MAPPING	0x004
#define	DUMP_SW_MAPPING	0x008

#define DUMP_ALL	0xfff


main(argc, argv)
int argc;
char **argv;
{
  FILE *conf_file;
  FILE *err_file = stderr;
  FILE *out_file = stdout;
  char *cfn = NULL;
  int dump_flags = DUMP_CONFIG;
  
  int c;
  extern int optind;
  extern char *optarg;
  int errflg = 0;
  
  
  while ( (c = getopt(argc, argv, "ac:ilsv")) != EOF)
    switch(c) {
    case 'a':
      dump_flags = DUMP_ALL;
      break;
    case 'c':
      cfn = optarg;
      break;
    case 'i':
      dump_flags = DUMP_IP_MAPPING;
      break;
    case 'l':
      dump_flags = DUMP_LA_MAPPING;
      break;
    case 's':
      dump_flags = DUMP_SW_MAPPING;
      break;
    case 'v':
      print_version_info();
      exit(0);
    default:
      errflg++;
    }
  
  if (errflg) {
    fprintf(err_file, "usage: %s [-c config_file] -[ail]\n",
	    argv[0]);
    fprintf(err_file, "\t-a Dump all\n");
    fprintf(err_file, "\t-i Dump IP mapping\n");
    fprintf(err_file, "\t-l Dump logical address mapping\n");
    exit(1);
  }
  
  if (optind < argc)
    fprintf(err_file, "%s: extra arguments ignored\n", argv[0]);
  
  read_config(cfn, &cfn);
  
  printf("\nUsing configuration file %s\n", cfn);
  
  if (dump_flags & DUMP_CONFIG)
    dump_config(out_file);
  if (dump_flags & DUMP_LA_MAPPING)
    dump_addresses(out_file);
  if (dump_flags & DUMP_IP_MAPPING)
    dump_netaddresses(out_file);
  if (dump_flags & DUMP_SW_MAPPING)
    dump_switch_mappings(out_file);

  
  exit(0);
}

static void
dump_config(file)
FILE *file;
{
  SWITCH		*sw;
  PORT			*sp;
  int			spnum;
  
  /*
   * HIPPISWD Information
   */
  
  fprintf(file, "\nHIPPISWD Configuration\n");
  if (*daemon_config.working_dir)
    fprintf(file, "\tDir: %s\n", daemon_config.working_dir);
  if (*daemon_config.hostname) {
    fprintf(file, "\tHost: %s", daemon_config.hostname);
    if (daemon_config.daemon_port)
      fprintf(file, " (Port: %d)", daemon_config.daemon_port);
    fprintf(file, "\n");
  }
  fprintf(file, "\tPassword File: %s\n", daemon_config.password_file);
  fprintf(file, "\tLog File: %s\n", daemon_config.log_file);
  fprintf(file, "\tMail Command: %s\n", daemon_config.mail_command);
  fprintf(file, "\n");

  /*
   * Address configuration
   */
  fprintf(file, "Address Configuration\n");
  if (addr_config.hippi_network != NETADDR_NULL)
    fprintf(file, "\tIP Network: %s\n",
	    netaddr_to_ascii(addr_config.hippi_network));
  if (addr_config.loopback_addr != LOGADDR_NULL)
    fprintf(file, "\tLoopback address: 0x%03x\t\t", addr_config.loopback_addr);
  switch (addr_config.default_type) {
  case NO_DEFAULT_LA:
    fprintf(file, "No default logical addresses.\n");
    break;
  case DO_LA_1374:
    fprintf(file, "RFC 1374 default logical addresses.\n");
    break;
  case DO_LA_IP_8:
    fprintf(file, "Default logical addresses are based on lower 8 bits of IP address.\n");
    break;
  case DO_LA_IP_12:
    fprintf(file, "Default logical addresses are based on lower 12 bits of IP address.\n");
    break;
  }
  if (addr_config.tester_port != NULL)
    fprintf(file, "\tTester: %s (switch %s port %d)\n",
	    addr_config.tester_port->swp_name,
	    addr_config.tester_port->swp_switch->sw_name,
	    addr_config.tester_port->swp_num);
  fprintf(file, "\n");

  fprintf(file, "%d Switches Configured:\n", number_of_switches());

  /* 
   * Traverse the switch structure chain
   */
  FOR_ALL_SWITCHES(sw) {

    /* Switch Info */
    
    fprintf(file, "\nswitch #%-2d\t%-20s  %-10s %d ports\n",
	    sw->sw_num,
	    sw->sw_name,
	    switch_types[sw->sw_type],
	    sw->sw_num_ports);
    if (*sw->sw_hostname)  {
      fprintf(file, "   Address: %s", sw->sw_hostname);
      if (sw->sw_tport)
	fprintf(file, " (Port: %d)", sw->sw_tport);
      if (*sw->sw_prompt)
	fprintf(file, "  Prompt: \"%s\"", 
		sw->sw_prompt);
      fprintf(file, "\n");
    }
    fprintf(file, "   %s, %s, %d bits, version %d\n",
	    (sw->sw_is_smart ? "SMART" : "DUMB"),
	    (sw->sw_has_default ? "DOES DEFAULT" : "NO DEFAULT"),
	    sw->sw_bits_shifted, sw->sw_version);
    if (strlen(sw->sw_comment) > 0)
      fprintf(file, "   comment: %s\n", sw->sw_comment);

    FOR_EACH_PORT(sw->sw_ports, sp, spnum) {
      fprintf(file, "\t%s %-2d %s\t%-20s",
	      (sp->swp_width == SINGLE_WIDE ? "port " : "dport"),
	      sp->swp_num,
	      (sp->swp_tester ? "tester" : port_types[sp->swp_type]),
	      sp->swp_name);
      switch(sp->swp_type)	{
	
      case HIPPI_NULL:
      case HIPPI_DEVICE:
	break;

      case HIPPI_LINK:
	if (sp->link_metric !=
	    DEFAULT_LINK_METRIC)
	  fprintf(file, " (metric %d)",
		  sp->link_metric);
	if (sp->link_default)
	  fprintf(file, " (Default)");
	break;

      case HIPPI_HOST:
	fprintf(file, "\t%-14s",
		netaddr_to_ascii(sp->host_addr));
	if (sp->host_mtu != HIPPI_MTU)
	  fprintf(file, " mtu %5d", sp->host_mtu);
	if ((sp->host_idev != NO_DEVICE_NUMBER) &&
	    (sp->host_odev != NO_DEVICE_NUMBER))
	  fprintf(file," dev %3d/%-3d  ", sp->host_idev, sp->host_odev);
	break;
	
      case HIPPI_DX:
      case HIPPI_CNT:
	fprintf(file, "\t%-14s",
		netaddr_to_ascii(sp->host_addr));
	fprintf(file, " mtu %5d",
		sp->host_mtu);
	break;
	
      default:
	fprintf(file, "[unknown type]");
      }
      fprintf(file, "\n");
      
      if (strlen(sp->swp_comment) > 0)
	fprintf(file, "\t   comment: %s\n", sp->swp_comment);
   }
  }
}



static void
dump_addresses(file)
     FILE *file;
{
  LOGICAL_MAP	*map;

  fprintf(file, "\n\nLogical Address Mapping\n\n");
  fprintf(file, "Address\tHost\t\t\tSwitch\t\t\tPort\n");
  fprintf(file, "-------\t-----------\t\t-----------\t\t----\n");

  FOR_ALL_LOGICAL_MAPS(map) {
    fprintf(file, "0x%03x\t%-20s\t%-20s\t%4d\n",
	    map->logaddr,
	    map->hostname,
	    map->port->swp_switch->sw_name,
	    map->port->swp_num);
  }

}

static void
dump_netaddresses(file)
     FILE *file;
{
  HOST_MAP	*map;

  fprintf(file, "\n\nIP Address Mapping\n\n");
  fprintf(file, "Host\t\t\t\t\tLogical Address\n");
  fprintf(file, "----------\t\t\t\t---------------\n");

  FOR_ALL_HOST_MAPS(map)
    fprintf(file, "%-40s\t0x%03x\n",
	    hostname_to_fullname(map->hostname),
	    map->logaddr);

}



static void
dump_switch_mappings(file)
     FILE *file;
{
  SWITCH		*from_switch, *to_switch;
  PATH			*path;
  PORT			*port;
  int			port_num;


  fprintf(file, "\nSwitch Mappings\n\n");

  FOR_ALL_SWITCHES(from_switch) {
    fprintf(file, "From %s (switch %d)\n",
	    from_switch->sw_name, from_switch->sw_num);

    FOR_ALL_SWITCHES(to_switch) {
      
      if (to_switch == from_switch)
	continue;

      path = find_path(from_switch, to_switch);

      fprintf(file, "\tTo %-20s (metric %3d):",
	      to_switch->sw_name, path->cost);

      if (path->num_ports == 0)
	fprintf(file, "\tNo path.");
      else
	FOR_EACH_PORT_IN_PATH(path, port, port_num)
	  fprintf(file, "  %-2d", port->swp_num);

      fprintf(file, "\n");
    }
  }
}
