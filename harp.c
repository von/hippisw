/*
 *	harp		Make HIPPI ip/ifield map
 *
 *
 *	The map file is written to standard output.
 *
 *	$Id: harp.c,v 1.2 1995/03/27 16:50:44 vwelch Exp $
 */
#include <stdio.h>
#include "basic_defines.h"
#include "parse_token.h"
#include "find.h"
#include "port.h"
#include "switch.h"
#include "address_map.h"
#include "ip_addr.h"
#include "logical_addr.h"

#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>



enum system_type {
  ARCH_HOSTS,			/* /etc/hosts format		*/
  ARCH_CRAY,			/* Cray				*/
  ARCH_SGI,			/* SGI				*/
  ARCH_DXE,			/* NSC DXE HIPPI gateway	*/
  ARCH_CONVEX,			/* CONVEX			*/
  ARCH_CS6400
};

struct token_mapping tokens[] = {
  { "hosts",		ARCH_HOSTS },
  { "sgi",		ARCH_SGI },
  { "cray",		ARCH_CRAY },
  { "dxe",		ARCH_DXE },
  { "convex",		ARCH_CONVEX },
  { "cs6400",		ARCH_CS6400 },
  { NULL,		0 }
};

char *arches[] = {
  "/etc/hosts format",  
  "Cray",
  "SGI",
  "NSC DXE gateway",
  "Convex",
  "CS6400",
};



static void	print_entry	PROTO((enum system_type arch,
				       FILE *out_file,
				       HOST_MAP *map,
				       PORT *host_port));

static void	print_header	PROTO((enum system_type arch,
				       FILE *out_file,
				       PORT *host_port));
/*
 *	XXX-KLUDGE no proto for comment since it has a variable number of
 *		arguments.
 */
static void	comment();
static void	usage		PROTO((VOID));

static char	*date_string;
static Boolean	print_comments = TRUE;
static Boolean	use_dot_address = FALSE;

static char *hostname_to_string		PROTO((char *hostname));


main(argc, argv)
int argc;
char **argv;
{
  FILE			*conf_file;
  FILE			*err_file = stderr;
  FILE			*out_file = stdout;
  char			*cfn = NULL;
  char			*what = NULL;
  char			*host = NULL;
  PORT			*host_port = NULL;
  HOST_MAP		*map;
  enum system_type	arch;
  time_t		tloc;
  
  int			c;
  extern int		optind;
  extern char		*optarg;
  int			errflg = 0;


  while ( (c = getopt(argc, argv, "c:Civ")) != EOF)
    switch(c) {

    case 'c':
      cfn = optarg;
      break;

    case 'C':
      print_comments = FALSE;
      break;

    case 'i':
      use_dot_address = TRUE;
      break;

    case 'v':
      print_version_info();
      exit(0);

    default:
      errflg++;
    }

  if (argc - optind < 1) {
    errflg++;

  } else {
    
    what = argv[optind++];
    
    arch = parse_token(what, tokens);

    if (arch == TOKEN_NOT_FOUND) {
      fprintf(stderr, "Unknown type %s.\n", what);
      errflg++;
    }
    
    if (argc - optind != 0)
      host = argv[optind++];
    
    if (argc - optind != 0)
      fprintf(stderr, "%s: Ignoring extra arguments.\n",
	      argv[0]);
  }

  /*	Do we need host name?	*/
  if ((arch == ARCH_CRAY) && (host == NULL)) {
    fprintf(stderr, "Hostname required for Cray architecture.\n\n");
    errflg++;

  } else {

    /* If not silently ignore it. */
    host = NULL;
  }

  if (errflg) {
    usage();
    exit(1);
  }

  /**	Build date string
   **/
  tloc = time(NULL);
  date_string = ctime(&tloc);
  
  read_config(cfn, &cfn);

  if (host) {
    host_port = find_port_by_name(host, FIND_SUBSTRING);

    if(host_port == NULL)	{
      fprintf(stderr, "Host \"%s\" not found.\n", host);
      exit(1);
    }
  }

  print_header(arch, out_file, host_port);
  
  /*
   * Do for all addresses
   */
  FOR_ALL_HOST_MAPS(map)
    print_entry(arch, out_file, map, host_port);
  
  exit(0);
}


static void
comment(arch, out_file, comment, arg1, arg2, arg3, arg4, arg5, arg6)
     enum system_type	arch;
     FILE		*out_file;
     char		*comment;
     char		*arg1, *arg2, *arg3, *arg4, *arg5, *arg6;
{
  if (print_comments == FALSE)
    return;

  fprintf(out_file, "#");
  fprintf(out_file, comment, arg1, arg2, arg3, arg4, arg5, arg6);
}


static void
print_header(arch, out_file, host_port)
     enum system_type	arch;
     FILE		*out_file;
     PORT		*host_port;
{
  comment(arch, out_file, "\n");

  switch(arch) {
  case ARCH_CRAY:
    comment(arch, out_file, "\tARP table for Cray: %s\n",
	    host_port->host_name);
    break;

  case ARCH_HOSTS:
    comment(arch, out_file, "\tHIPPI ARP Table (/etc/hosts format)\n");
    break;

  default:
    comment(arch, out_file, "\tARP table for %s\n",
	    arches[arch]);
  }
  
  comment(arch, out_file, "\n");
  comment(arch, out_file, "\tCreated: %s", date_string);
  comment(arch, out_file, "\n");

  switch(arch) {
  case ARCH_CRAY:
    comment(arch, out_file,
	    "\n");
    comment(arch, out_file,
	    "\tFile: /etc/hycf.hsx0\n");
    comment(arch, out_file,
	    "\n");
    comment(arch, out_file,
	    "\t\t\t\t\t\tMinor number\n");
    comment(arch, out_file,
	    "\t Hostname\t\t\t\tI-field    In   Out   MTU\n");
    comment(arch, out_file,
	    "      ------------\t\t\t\t--------  ----  ----  -----\n");
    break;
    
  case ARCH_SGI:
    comment(arch, out_file,
	    "\n");
    comment(arch, out_file,
	    "\tFile: /usr/etc/hippi.imap\n");
    comment(arch, out_file,
	    "\n");
    comment(arch, out_file,
	    " Hostname\t\t\t I-field\tHIPPI ULA (optional)\n");
    comment(arch, out_file,
	    "----------\t\t\t----------\t--------------------\n");
    break;
		
  case ARCH_CONVEX:
    comment(arch, out_file,
	    "\n");
    comment(arch, out_file,
	    "\tFile: /etc/hippi.arp\n");
    comment(arch, out_file,
	    "\n");
    comment(arch, out_file,
	    "\tFormat:\n");
    comment(arch, out_file,
	    " route addhippi host <dest> <gateway> <metric> <mtu> <ifield>\n");
    comment(arch, out_file,
	    "\n");
    break;

  case ARCH_CS6400:
    comment(arch, out_file,
	    "\n");
    comment(arch, out_file,
	    "\tFile: /etc/opt/CYRShippi/hippi.art0\n");
    comment(arch, out_file,
	    "\n");
    comment(arch, out_file,
	    " Inet\t\t\t\tULA\t\t\tI-field\n");
    comment(arch, out_file,
	    "----------\t\t\t-------\t\t\t----------\n");
    break;
  }
}



static void
print_entry(arch, out_file, map, local_host_port)
     enum system_type	arch;
     FILE		*out_file;
     HOST_MAP		*map;
     PORT		*local_host_port;
{
  Logaddr		logaddr = map->logaddr;
  PORT			*host_port = map->port;
  SWITCH		*sw = host_port->swp_switch;
  char			*hostname = map->hostname;
  
  switch(arch) {
  case ARCH_CRAY:
    if (map->netaddr == NETADDR_NULL)
      return;

    if (use_dot_address)
      comment(arch, out_file, " %s\n", hostname_to_fullname(hostname));
    fprintf(out_file, "direct %-40s", hostname_to_string(hostname));
    fprintf(out_file, "\t%8.8x", logical_to_ifield(logaddr));
    fprintf(out_file, "  %4.4x  %4.4x  %d ;\n",
	    local_host_port->host_idev, local_host_port->host_odev,
	    host_port->host_mtu);
    break;

  case ARCH_SGI:
    if (map->netaddr == NETADDR_NULL)
      return;

    if (use_dot_address)
      comment(arch, out_file, " %s\n", hostname_to_fullname(hostname));
    
    fprintf(out_file, "%-24s", hostname_to_string(hostname));
    fprintf(out_file, "\t%#x\n", logical_to_ifield(logaddr));
    break;

  case ARCH_HOSTS:
    {
      struct hostent *hinfo;
      char **aliases, *fullname;
      
      comment(arch, out_file, "\n");
      comment(arch, out_file, " %-38s\tAddress: %03x\n",
	      hostname, logaddr);
      comment(arch, out_file, " Switch: %-30s\tPort: %-3d\n",
	      sw->sw_name, host_port->swp_num);
      
      if (map->netaddr == NETADDR_NULL)
	return;
      
      if ((hinfo = gethostbyname(hostname)) != NULL) {
	fullname = hinfo->h_name;
	aliases = hinfo->h_aliases;
      } else {
	fullname = "";
	aliases = NULL;
      }

      fprintf(out_file, "%s\t%s",
	      netaddr_to_ascii(map->netaddr), fullname);
      
      if (aliases != NULL) {
	int index = 0;

	while (aliases[index] != NULL) {
	  fprintf(out_file, " %s", aliases[index]);
	  index++;
	}
      }
      
      fprintf(out_file, " %s\n", hostname);
      
    }
    break;
    
  case ARCH_DXE:
    if (map->netaddr == NETADDR_NULL)
      return;
    
    fprintf(out_file, "define host %s hippi %#x\t# %s\n",
	    netaddr_to_ascii(map->netaddr),
	    logical_to_ifield(logaddr), hostname);
    break;
    
  case ARCH_CONVEX:
    if (map->netaddr == NETADDR_NULL)
      return;
    
    if (use_dot_address)
      comment(arch, out_file, " %s\n", hostname_to_fullname(hostname));

    fprintf(out_file, "route addhippi host %s %s 0 65280 %#x\n",
	    hostname_to_string(hostname), hostname_to_string(hostname),
	    logical_to_ifield(logaddr));
    break;
    
  case ARCH_CS6400:
    if (map->netaddr == NETADDR_NULL)
      return;

    if (use_dot_address)
      comment(arch, out_file, " %s\n", hostname_to_fullname(hostname));

    fprintf(out_file, "%-24s\t0:0:0:0:0:0\t\t%#x\n",
	    hostname_to_string(hostname),
	    logical_to_ifield(logaddr));
    break;
  }
}

static void
usage()
{
  fprintf(stderr, "usage: harp <options> [<architecture>|hosts] [<hostname>]\n\n");
  fprintf(stderr, "  Current architectures are: cray, sgi, convex\n");
  fprintf(stderr, "  Options:\n");
  fprintf(stderr, "\t-c <config file>   Specify configuration file to use.\n");
  fprintf(stderr, "\t-C                 Terse mode, don't print comments.\n");
  fprintf(stderr, "\t-i                 Use dot addresses instead of hostnames.\n");
  fprintf(stderr, "\t-v                 Print version information and quit.\n");
}


/*
 *	Convert a hostname to either a full hostname or a dot-address
 *	string depending on use_dot_address.
 */
static char *
hostname_to_string(hostname)
     char		*hostname;
{
  if (use_dot_address)
    return netaddr_to_ascii(hostname_to_netaddr(hostname));
  else
    return hostname_to_fullname(hostname);
}
    
