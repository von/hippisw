/*
 *	port_conf.c
 *
 *	Read a port configuration.
 *
 */

#include "basic_defines.h"
#include "read_config.h"
#include "parse_file.h"
#include "parse_token.h"
#include "port.h"
#include "switch.h"
#include "portlist.h"
#include "logical_addr.h"
#include "ip_addr.h"
#include "address_map.h"


#include <string.h>


static int scan_host			PROTO((PORT *port));
static int scan_null			PROTO((PORT *port));
static int scan_link			PROTO((PORT *port));
static int scan_device			PROTO((PORT *port));

static Boolean handle_port_options	PROTO((PORT *port,
					       char *option));
static void handle_addr			PROTO((PORT *port));
static void handle_addhost		PROTO((PORT *port));
static void handle_default		PROTO((PORT *port));

static int handle_add_map		PROTO((char *hostname,
					       Logaddr logaddr,
					       PORT *port,
					       char *comment));

#define	handle_logical_port(logaddr, port, comment)	\
	handle_add_map(NULL, logaddr, port, comment)

#define handle_ip_logical(hostname, logaddr, port)	\
	handle_add_map(hostname, logaddr, port, NULL)


/*
 *	Keywords
 */
enum option_values {
  OPTION_ADDR,
  OPTION_ADDHOST,
  OPTION_MTU,
  OPTION_COMMENT,
  OPTION_DEFAULT,
  OPTION_NEED_DISABLED,
  OPTION_TESTER,
  HOSTOPT_DEV,
  LINKOPT_DEFAULT,
  LINKOPT_METRIC
};

static struct token_mapping port_types[] = {
  { "null",		HIPPI_NULL },
  { "device",		HIPPI_DEVICE },
  { "link",		HIPPI_LINK },
  { "host",		HIPPI_HOST },
  { "dx",		HIPPI_DX },
  { "cntfddi",		HIPPI_CNT },
  { "tester",		HIPPI_TESTER },
  { NULL, 		0 }
};

static struct token_mapping port_options[] = {
  { "comment",		OPTION_COMMENT },
  { "needdisabled",	OPTION_NEED_DISABLED },
  { NULL,		0 }
};

static struct token_mapping non_link_options[] = {
   { "addr",		OPTION_ADDR },
   { "tester",		OPTION_TESTER },
   { "default",		OPTION_DEFAULT},
   { NULL,		0}
 };

static struct token_mapping host_options[] = {
  { "addhost",		OPTION_ADDHOST },
  { "mtu",		OPTION_MTU },
  { "dev",		HOSTOPT_DEV },
  { NULL,		0 }
};

static struct token_mapping null_options[] = {
  { NULL,		0}
};

static struct token_mapping link_options[] = {
  { "metric",		LINKOPT_METRIC },
  { NULL,		0 }
};

static struct token_mapping device_options[] = {
  { NULL,		0 }
};


/*
 *	Read a port configuration
 */
void
port_conf(sw, width)
     SWITCH		*sw;
     int		width;
{
  PORT		*port;
  PORT		*duplicate_port;

  char		*port_type;
  char		*number_str;
  int		type;

  int		rc;


  number_str = read_option();
  port_type = read_option();

  if ((number_str == NULL) || (port_type == NULL)) {
    config_error("Port number and type required for port. Skipping.\n");
    next_keyword();
    return;
  }

  if (is_numeric(number_str) == FALSE) {
    config_error("Port number \"%s\" is not a legal port number. Skipping.\n",
		 number_str);
    next_keyword();
    return;
  }

  if ((type = parse_token(port_type, port_types)) == TOKEN_NOT_FOUND) {
    config_error("Port type \"%s\" not recognized.\n", port_type);
    type = HIPPI_NULL;
  }

  port = (PORT *) malloc(sizeof(*port));

  if (port == NULL) {
    config_error("malloc() failed. Exiting.\n");
    exit(1);
  }

  /*
   *	Tester is just a device
   */
  if (type == HIPPI_TESTER) {
    type = HIPPI_DEVICE;	
    port->swp_tester = TRUE;
  } else {
    port->swp_tester = FALSE;
  }

  port->swp_switch = sw;
  port->swp_num = str_to_int(number_str);
  port->swp_width = width;
  port->swp_linenum = parsed_linenumber();
  port->swp_type = type;
  NULL_STRING(port->swp_comment);
  port->swp_default_logaddr = LOGADDR_NOT_ASSIGNED;
  port->swp_need_disabled = FALSE;


  switch(type) {
  case HIPPI_NULL:
    rc = scan_null(port);
    break;

  case HIPPI_DEVICE:
    rc = scan_device(port);
    break;

  case HIPPI_LINK:
    rc = scan_link(port);
    break;

  case HIPPI_HOST:
  case HIPPI_DX:
  case HIPPI_CNT:
    rc = scan_host(port);
    break;
  }

  if (rc == ERROR) {
    free(port);
    return;
  }
  
  /*
   * Check for duplicate port number.
   */
  duplicate_port = find_port_by_number(sw->sw_ports, port->swp_num);

  if (duplicate_port != NULL) {
    config_error("Port number %d on switch %s already assigned to %s on line %d. Ignoring.\n",
		 port->swp_num, port->swp_switch,
		 duplicate_port->swp_name, duplicate_port->swp_linenum);
    free(port);
    return;
  }

  sw->sw_ports = add_to_portlist(sw->sw_ports, port);
 
  if (port->swp_default_logaddr == LOGADDR_NOT_ASSIGNED)
    port->swp_default_logaddr = get_default_logical(port);

  if (port->swp_default_logaddr != LOGADDR_NULL) {	
    if (HAS_IP(port))
      handle_ip_logical(port->swp_name, port->swp_default_logaddr, port);
    else
      handle_logical_port(port->swp_default_logaddr, port, port->swp_name);
  }

  return;
}
    

/*
 *	Port type specific parsers.
 *
 *	All return ERROR if an error occurs and the port entry should
 *	be ignored. NO_ERROR otherwise.
 */

/*
 *	Read a host or DX box configuration.
 */
static int
scan_host(port)
     PORT		*port;
{
  char		*option, *name;
  char		*argument, *argument2;

  int		token;

  Netaddr	netaddr;

  port->swp_name = port->host_name;
  
  name = read_option();

  if (name == NULL) {
    config_error("Host has no name. Ignoring.\n");
    return ERROR;
  }

  strncpy(port->swp_name, name, HNAMELEN);
  
  if ((netaddr = hostname_to_netaddr(port->swp_name)) == NETADDR_NULL)
    config_error("Warning: Can't get net address for \"%s\".\n",
		 port->swp_name);
  
  port->host_addr = netaddr;	


  /*
   *	Set Defaults
   */
  port->host_idev = NO_DEVICE_NUMBER;
  port->host_odev = NO_DEVICE_NUMBER;
  port->host_mtu = HIPPI_MTU;
  NULL_STRING(port->host_ifname);

  /*
   *	Read options
   */
  while ((option = read_option()) != NULL) {

    if (handle_port_options(port, option) == TRUE)
      continue;

    if ((token = parse_token(option, host_options)) == TOKEN_NOT_FOUND) {
      config_error("\"%s\" is not a recognized option. Skipping.\n", option);
      continue;
    }

    switch(token) {
    case OPTION_ADDHOST:
      handle_addhost(port);
      break;

    case OPTION_MTU:
      if ((argument = read_option()) == NULL) {
	config_error("Argument missing for MTU options.\n");
	break;
      }
      
      if (is_numeric(argument) == FALSE) {
	config_error("Numeric argument required for MTU option.\n");
	break;
      }

      port->host_mtu = str_to_int(argument);
      break;

    case HOSTOPT_DEV:
      argument = read_option();
      argument2 = read_option();

      if (port->swp_type != HIPPI_HOST) {
	config_error("Dev option only valid for HIPPI hosts.\n");
	break;
      }

      if ((argument == NULL) || (argument2 == NULL)) {
	config_error("Input and output device numbers required for dev option.\n");
	break;
      }

      if (is_numeric(argument) == FALSE) {
	config_error("\"%s\" is not a valid numeric argument.\n", argument);
	break;
      }

      if (is_numeric(argument2) == FALSE) {
	config_error("\"%s\" is not a valid numeric argument.\n", argument2);
	break;
      }
      
      port->host_idev = str_to_int(argument);
      port->host_odev = str_to_int(argument2);
      break;
    }
  }

  return NO_ERROR;
}
     
  
/*
 *	Scan a null entry.
 *
 *	Basically this is just a placemarker of some sort. Ignore all options
 *	and just save the comment.
 */
static int
scan_null(port)
     PORT		*port;
{
  char		*argument;
  int		token;
 

  port->swp_name = "NULL_PORT";

  while((argument = read_option()) != NULL) {

    if (handle_port_options(port, argument) == TRUE)
      continue;

    if ((token = parse_token(argument, null_options)) == TOKEN_NOT_FOUND)
      continue;

    switch(token) {
      /* No unique options	*/
    }
  }

  return NO_ERROR;
}


/*
 *	Read a link configuration.
 */
static int
scan_link(port)
     PORT		*port;
{
  char		*argument;
  int		token;

  /*	Read destination switch
   */
  argument = read_option();

  if (argument == NULL) {
    config_error("Destination switch required for link. Ignoring.\n");
    return ERROR;
  }

  /*	We can't check the switch name now, since we don't have a complete
   *	list of switches yet.
   */

  strncpy(port->link_swname, argument, SWNAMELEN);
  port->link_sw = NULL;
  port->link_default = FALSE;
  port->link_metric = DEFAULT_LINK_METRIC;
  
  /* Create link name
   */
  {
    char buffer[BUFFERLEN];
    
    sprintf(buffer, "Link to %s", port->link_swname);

    port->swp_name = strdup(buffer);
  }

  while ((argument = read_option()) != NULL) {

    if (handle_port_options(port, argument) == TRUE)
      continue;

    if ((token = parse_token(argument, link_options)) == TOKEN_NOT_FOUND) {
      config_error("\"%s\" is not recognized. Skipping.\n", argument);
      continue;
    }

    switch(token) {
    case LINKOPT_DEFAULT:
      port->link_default = TRUE;
      break;

    case LINKOPT_METRIC:
      if ((argument = read_option()) == NULL) {
	config_error("Argument required for metric option. Ignoring.\n");
	break;
      }

      if (is_numeric(argument) == FALSE) {
	config_error("\"%s\" is not a valid metric. Ignoring.\n", argument);
	break;
      }

      port->link_metric = str_to_int(argument);
      break;
    }
  }

  return NO_ERROR;
}



/*
 *	Read a device configuration.
 */
static int
scan_device(port)
     PORT		*port;
{
  char		*argument;
  int		token;


  /*	Read name
   */
  if ((argument = read_option()) == NULL) {
    argument = "Unnamed";
  }

  port->swp_name = port->dev_name;

  strncpy(port->swp_name, argument, HNAMELEN);

  while((argument = read_option()) != NULL) {

    if (handle_port_options(port, argument) == TRUE)
      continue;

    if ((token = parse_token(argument, device_options)) == TOKEN_NOT_FOUND) {
      config_error("\"%s\" is not recognized. Skipping.\n", argument);
      continue;
    }

    switch(token) {
      /* No unique options	*/
    }
  }

  return NO_ERROR;
}


/*
 *	Handle general port options.
 *
 *	Returns TRUE if the option was parsed, FALSE if it was not.
 */
static Boolean
handle_port_options(port, option)
     PORT		*port;
     char		*option;
{
  int		token;
  char		*arg;

  if ((token = parse_token(option, port_options)) != TOKEN_NOT_FOUND) {
 
    switch(token) {
    case OPTION_COMMENT:
      arg = read_option();

      if (arg == NULL)
	config_error("Warning: null comment.\n");
      else
	strncpy(port->swp_comment, arg, COMMENTLEN);
      break;

    case OPTION_NEED_DISABLED:
      port->swp_need_disabled = TRUE;
      break;
    }
    
    return TRUE;
  }

  if (port->swp_type == HIPPI_LINK)
    return FALSE;
 
  if ((token = parse_token(option, non_link_options)) != TOKEN_NOT_FOUND) {
 
    switch(token) {
    case OPTION_ADDR:
      handle_addr(port);
      break;

    case OPTION_TESTER:
      port->swp_tester = TRUE;
      break;

    case OPTION_DEFAULT:
      handle_default(port);
      break;
    }
    
    return TRUE;
  }


  return FALSE;
}

  


/*
 *	Handle the "addr" option.
 *
 *	It's format is: addr [comment] <address>
 *
 */
static void
handle_addr(port)
     PORT		*port;
{
  char		*argument;
  char		*comment = port->swp_name;
  Logaddr	logaddr;

  if ((argument = read_option()) == NULL) {
    config_error("Address required for addr option.\n");
    return;
  }
 
  /*	If this argument is numeric assume it's the address, otherwise
   *	assume it's a comment.
   */
  if (is_numeric(argument) == FALSE) {
    comment = argument;

    if ((argument = read_option()) == NULL) {
      config_error("\"%s\" is not a valid address.\n", comment);
      return;
    }
  }

  logaddr = str_to_log(argument);

  handle_logical_port(logaddr, port, comment);
}




/*
 *	Handle the "addhost" option.
 *
 *	It's format is: addhost <hostname> <address>
 */
static void
handle_addhost(port)
     PORT		*port;
{
  char		*hostname;
  char		*address;

  Logaddr	logaddr;

  LOGICAL_MAP	*map;
  PORT		*duplicate;


  hostname = read_option();
  address = read_option();

  if ((hostname == NULL) || (address == NULL)) {
    config_error("Hostname and logical address required for addhost option.\n");
    return;
  }

  logaddr = str_to_log(address);

  /*	Check to be sure the logical address is not already in use.
   */
  if ((map = find_map_by_logaddr(logaddr)) != NULL) {
    duplicate = map->port;
    config_error("Logical address %#x is already used by %s on line %d.\n",
		 logaddr, duplicate->swp_name, duplicate->swp_linenum);
    return;
  }

  handle_ip_logical(hostname, logaddr, port);
}       
				   
  
/*
 *	Handle the default option
 */
static void
handle_default(port)
     PORT		*port;
{
  Logaddr		logaddr;

  char			*addr_str;

  if ((addr_str = read_option()) == NULL) {
    config_error("Address required for default option.\n");
    return;
  }

  if (strcasecmp(addr_str, "none") == 0)
    logaddr = LOGADDR_NULL;
  else
    logaddr = str_to_log(addr_str);

  port->swp_default_logaddr = logaddr;
}




/*
 *	Handle the adding of a logical address to port mapping.
 */
static int
handle_add_map(hostname, logaddr, port, comment)
     char		*hostname;
     Logaddr		logaddr;
     PORT		*port;
     char		*comment;
{
  int		rc = NO_ERROR;
  LOGICAL_MAP	*lmap;
  LOGICAL_MAP	*hmap;
  PORT		*duplicate;
  Netaddr	netaddr;


  switch(add_address_map(hostname, logaddr, port, comment)) {
  case NO_ERROR:
    break;

  case ADD_ADDRESS_MALLOC_ERROR:
    config_error("malloc() error. Exiting.\n");
    exit(1);

  case ADD_ADDRESS_LA_DUP:
    lmap = find_map_by_logaddr(logaddr);

    duplicate = lmap->port;

    /* if (duplicate == port) then this mapping has already taken place
     */
    if (duplicate != port) {
      config_error("Address %#x is already in use by %s (line %d).\n",
		   logaddr, duplicate->swp_name, duplicate->swp_linenum);
    
      rc = ERROR;
    }
    break;

  case ADD_ADDRESS_LA_ILLEGAL:
    config_error("Address %#x is not a legal value.\n", logaddr);
    rc = ERROR;
    break;

  case ADD_ADDRESS_IP_DUP:
    hmap = find_map_by_netaddr(hostname_to_netaddr(hostname));
    
    netaddr = hostname_to_netaddr(hostname);

    config_error("%s (%s) is already mapped to logical address 0x%x.\n",
		 hostname, netaddr_to_ascii(netaddr), hmap->logaddr);
    rc = ERROR;
    break;
    

   case ADD_ADDRESS_HOST_UNKNOWN:
    config_error("Could resolve hostname \"%s\".\n", hostname);
    rc = ERROR;
    break;
  }

  return rc;
}


 
				     
				    
