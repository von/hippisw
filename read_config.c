/*
 *	read_config.c
 *
 *	Read the configuration files.
 *
 *	Returns 0 on success, negitive on error.
 */

#include <stdio.h>

#include "basic_defines.h"
#include "parse_file.h"
#include "parse_token.h"
#include "read_config.h"
#include "switch.h"
#include "port.h"
#include "find.h"
#include "switchlist.h"

#define BUILD_CONFIG_STRUCTURES
#include "daemon_config.h"
#include "password_config.h"
#include "address_config.h"

#include <string.h>

static char *config_paths[] = {
#ifdef CONFIG_FILE
	CONFIG_FILE,
#endif
/*
 *	Add extra config files here
 */
	"/usr/local/etc/config.hippi",
	"/usr/local/etc/hippisw/config.hippi",
	NULL
};

/*
 *	Where to send error output
 */
static FILE	*error_file = stderr;

/*
 *	Keywords
 */
enum keywords_values {
	KEYWD_HIPPISWD,
	KEYWD_SWITCH,
	KEYWD_PORT,
	KEYWD_DPORT,
	KEYWD_ADDRESS,
	KEYWD_COMMENT
};

static struct token_mapping keywords[] = {
	{ "hippiswd", KEYWD_HIPPISWD },
	{ "switch", KEYWD_SWITCH },
	{ "port", KEYWD_PORT },
	{ "dport", KEYWD_DPORT },
	{ "address", KEYWD_ADDRESS },
	{ "#", KEYWD_COMMENT },
	{ NULL, 0 }
};


static void check_config		PROTO((VOID));

extern void hippiswd_conf		PROTO((VOID));
extern void address_conf		PROTO((VOID));
extern void port_conf			PROTO((SWITCH *sw,
									   int width));
extern SWITCH *switch_conf		PROTO((VOID));


/*
 *	Read the configuraton.
 *
 *	If config_file is non-NULL then only that path is attempted,
 *		otherwise the default list (config_paths) is attempted.
 *	If file_opened is non-NULL it is set to point at the path of the
 *		file that is opened.
 *	file_opened may == &config_file
 */

void
read_config(config_file, file_opened)
	char		*config_file;
	char		**file_opened;
{
	int			path_index = 0;
	char		*keyword;
	int			token;
	SWITCH		*current_switch = NULL;

	if (config_file == NULL) {
		while (config_paths[path_index] != NULL) {
			if (parse_file(config_paths[path_index]) == NO_ERROR)
				break;

			path_index++;
		}
		if (config_paths[path_index] == NULL) {
			fprintf(stderr, "Couldn't open configuration file. Exiting.\n");
			exit(1);
		}
    
		if (file_opened != NULL)
			*file_opened = config_paths[path_index];
	} else {
		if (parse_file(config_file) == ERROR) {
			perror(config_file);
     
			if (file_opened != NULL)
				*file_opened = config_file;
		}
	}

#ifdef DEBUG_CONF
	fprintf(stderr, "Opened config file: %s\n", config_paths[path_index]);
#endif

	/*
	 *	Enter loop until EOF is reached.
	 */

	while ((keyword = read_keyword()) != NULL) {

		if ((token = parse_token(keyword, keywords)) == TOKEN_NOT_FOUND) {
			config_error("\"%s\" is not recognized. Skipping.\n", keyword);
			next_keyword();
			continue;
		}

  
		switch(token) {
		case KEYWD_HIPPISWD:
			hippiswd_conf();
			break;

		case KEYWD_SWITCH:
			current_switch = switch_conf();
			break;

		case KEYWD_DPORT:
		case KEYWD_PORT:
			if (current_switch == NULL) {
				config_error("Port defined before switch. Skipping.\n");
				next_keyword();
				break;
			}

			port_conf(current_switch,
					  (token == KEYWD_PORT ? SINGLE_WIDE : DOUBLE_WIDE));
			break;

		case KEYWD_ADDRESS:
			address_conf();
			break;

		case KEYWD_COMMENT:
			next_line();
			break;
		}
	}

	check_config();

	close_parse();

#ifdef DEBUG_CONF
	fprintf(stderr, "Done parsing config file.\n");
#endif
}
      

/*
 *	Print a configuration error with filename and linenumber.
 */
void config_error(string, arg1, arg2, arg3, arg4, arg5, arg6)
	char		*string;
	char		*arg1, *arg2, *arg3, *arg4, *arg5, *arg6;
{
	fprintf(error_file,  "ERROR: %s line %d\n\t",
			parsed_filename(), parsed_linenumber());

	fprintf(error_file, string, arg1, arg2, arg3, arg4, arg5, arg6);
}

/*
 *	Print a configuration error allowing specification of line
 *	number.
 */
void config_error_l(linenumber, string, arg1, arg2, arg3, arg4, arg5, arg6)
	int			linenumber;
	char		*string;
	char		*arg1, *arg2, *arg3, *arg4, *arg5, *arg6;
{
	fprintf(error_file,  "ERROR: %s line %d\n\t",
			parsed_filename(), linenumber);

	fprintf(error_file, string, arg1, arg2, arg3, arg4, arg5, arg6);
}



/*
 *	Return true is a string represents a valid positive number
 */
Boolean
is_numeric(s)
	char		*s;
{
	if(s == NULL)
		return FALSE;
  
	if(*s == '0')   {
		s++;
		if(*s == 'x' || *s == 'X')      {
			s++;
			while(*s)       {
				if (isxdigit(*s++))
					continue;
				return FALSE;
			}
		} else  {
			while(*s)       {
				if(isdigit(*s) && *s != '8' && *s++ != '9')
					continue;
				return FALSE;
			}
		}
	} else  {
		while(*s)       {
			if(isdigit(*s++))
				continue;
			return FALSE;
		}
	}
  
	return TRUE;
}


/*
 *	Check the configuration.
 *	
 *	1) Go through and sort ports.
 *	2) Check link destinations and look for tester port.
 *	3) Check to be sure there is no conflict between the
 *		loopback address and other logical addresses.
 *     
 */
static void
check_config()
{
	SWITCH		*sw;
	PORT		*port;
	int			portnum;

	FOR_ALL_SWITCHES(sw) {
		sort_portlist(sw->sw_ports);

		FOR_EACH_PORT(sw->sw_ports, port, portnum) {

			if (port->swp_tester == TRUE) {
				if (addr_config.tester_port == NULL) {
					addr_config.tester_port = port;
				} else {
					config_error_l(port->swp_linenum,
								   "Ignoring duplicate tester port.\n");
					port->swp_tester = FALSE;
				}
			}

			if (port->swp_type != HIPPI_LINK)
				continue;

			port->link_sw = find_switch_by_name(port->link_swname, FIND_DEFAULT);

			if (port->link_sw == NOT_FOUND) {
				config_error_l(port->swp_linenum,
							   "Switch \"%s\" not found. Changing port to a NULL device.\n",
							   port->link_swname);

				port->swp_type = HIPPI_NULL;
			}
		}
	}

	/*
	 *	Check for conflicts with loopback address.
	 */
	if (addr_config.loopback_addr != LOGADDR_NULL) {
		LOGICAL_MAP *map;

		map = find_map_by_logaddr(addr_config.loopback_addr);

		if (map != NULL) {
			PORT		*port = map->port;

			config_error_l(port->swp_linenum,
						   "Logical address overrides loopback address.\n");

			addr_config.loopback_addr = LOGADDR_NULL; 
		}
	}
}
      

