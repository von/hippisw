/*
 *	ifield	Generate ifields
 */

#include "basic_defines.h"
#include "ifield.h"
#include "switch.h"
#include "port.h"
#include "find.h"
#include "logical_addr.h"
#include "portlist.h"
#include "switch_map.h"
#include "path.h"
#include "address_map.h"

#include <stdio.h>

static void usage				PROTO((VOID));
static int print_logical		PROTO((char *name));
static int get_logical_address	PROTO((char *name, Ifield *ifield));
static int find_route			PROTO((char *from, char *to, PORTLIST **hop));
static int portlist_to_ifield	PROTO((PORTLIST *portlist, Ifield *ifield));
static int print_location		PROTO((char *device));
static int print_device			PROTO((char *switch_name, char *port_string));


/*
 *	Return values for find_route()
 */
#define NO_ERROR			0
#define	NO_PATH				-1
#define UNKNOWN_SOURCE		-2
#define UNKNOWN_DEST		-3


/*
 *	Modes
 */
#define	MODE_SRC_ROUTE			0
#define MODE_LOGICAL_ADDRESS	1
#define	MODE_PRINT_LOCATION		2
#define MODE_PRINT_DEVICE		3



main(argc, argv)
	int			argc;
	char		**argv;
{
	FILE 		*out_file = stdout;

	char		*config_file = NULL;

	extern int 	optind;
	extern char	*optarg;

	int			arg;

	PORTLIST	*path = NULL, *hop;

	Ifield		ifield;

	Boolean		loopback = FALSE;
	Boolean		print_path = FALSE;
	int			mode = MODE_SRC_ROUTE;


	while ((arg = getopt(argc, argv, "c:lpvDL")) != EOF)
		switch(arg)	{
		case 'c':
			config_file = optarg;
			break;

		case 'l':
			loopback = TRUE;
			break;

		case 'p':
			print_path = TRUE;
			break;

		case 'v':
			print_version_info();
			exit(0);
      
		case 'D':
			mode = MODE_PRINT_DEVICE;
			break;

		case 'L':
			mode = MODE_PRINT_LOCATION;
			break;
		}

	if (optind == argc) {
		usage();
		exit(1);
	}

	read_config(config_file, &config_file);


	/*	If only one arg, return it's logical address.
	 */
	if ((mode == MODE_SRC_ROUTE) && (argc - optind == 1))
		mode = MODE_LOGICAL_ADDRESS;

	switch(mode) {
	case MODE_LOGICAL_ADDRESS:
		if (get_logical_address(argv[optind], &ifield) == ERROR)
			exit(1);
		break;

	case MODE_SRC_ROUTE:
		/*	Handle source route
		 */
		while ((argc - optind) > 1) {
			switch (find_route(argv[optind], argv[optind + 1], &hop)) {
			case NO_ERROR:
				path = append_portlist(path, hop);
				disperse_portlist(hop);
				optind++;
				break;

			case NO_PATH:
	
				fprintf(stderr,
						"Couldn't find a path between %s and %s. Exiting.\n",
						argv[optind], argv[optind + 1]);
				exit(1);

			case UNKNOWN_SOURCE:
				fprintf(stderr, "Unknown device: \"%s\".\n", argv[optind]);
				exit(1);

			case UNKNOWN_DEST:
				fprintf(stderr, "Unknown device: \"%s\".\n", argv[optind + 1]);
				exit(1);
			}
		}
    
		if (loopback) {
			while (optind > argc ) {
				switch (find_route(argv[optind], argv[optind - 1], &hop)) {
				case NO_ERROR:
					path = append_portlist(path, hop);
					disperse_portlist(hop);
					optind--;
					break;

				case NO_PATH:
	
					fprintf(stderr,
							"Couldn't find a path between %s and %s. Exiting.\n",
							argv[optind], argv[optind + 1]);
					exit(1);

				case UNKNOWN_SOURCE:
					fprintf(stderr, "Unknown device: \"%s\".\n",
							argv[optind]);
					exit(1);

				case UNKNOWN_DEST:
					fprintf(stderr, "Unknown device: \"%s\".\n",
							argv[optind - 1]);
					exit(1);
				}
			}
		}
    
		if (portlist_to_ifield(path, &ifield) != NO_ERROR)
			exit(1);

		if (print_path) {
			PORT		*port;
			int		port_num;

			fprintf(out_file, "Path:\n");

			FOR_EACH_PORT_IN_PATH(path, port, port_num)
				fprintf(out_file, "\tSwitch %s port %d (%s)\n",
						port->swp_switch->sw_name,
						port->swp_num,
						port->swp_name);

			fprintf(out_file, "\n");
		}
		break;

	case MODE_PRINT_LOCATION:	
		exit(print_location(argv[optind])); 

	case MODE_PRINT_DEVICE:
		if (argc - optind < 2) {
			fprintf(stderr, "Switch and port number required.\n\n");
			usage();
			exit(1);
		}
		exit(print_device(argv[optind], argv[optind + 1]));
	}

	fprintf(out_file, "Ifield: 0x%08x\n", ifield);
}


/*
 *	Find a path between two points.
 */
static int
find_route(from, to, route)
	char			*from;
	char			*to;
	PORTLIST		**route;
{
	SWITCH			*from_sw, *to_sw;

	PORT			*to_port, *from_port, *port;

	PATH			*hop;

	PORTLIST		*path = NULL;


	*route = NULL;

	/* Find source	*/
	if ((from_sw = find_switch_by_name(from, FIND_DEFAULT)) == NULL) {
		if ((from_port = find_port_by_name(from, FIND_SUBSTRING)) == NULL) {
			return UNKNOWN_SOURCE;
		}

		from_sw = from_port->swp_switch;

	} else {     
		from_port = NULL;
	}


	/* Find destination	*/
	if ((to_sw = find_switch_by_name(to, FIND_DEFAULT)) == NULL) {
		if ((to_port = find_port_by_name(to, FIND_SUBSTRING)) == NULL) {
			return UNKNOWN_DEST;
		}

		to_sw = to_port->swp_switch;

	} else {
		to_port = NULL;
	}
 
	/*	Transverse switches		*/
	while (from_sw != to_sw) {
		hop = find_path(from_sw, to_sw);

		if (hop->num_ports == 0)
			return NO_PATH;

		port = FIRST_PORT(hop);

		path = add_to_portlist(path, port);

		from_sw = port->link_sw;
	}

	/*	Transverse to final port if necessary	*/
	if (to_port != NULL)
		path = add_to_portlist(path, to_port);

	*route = path;

	return NO_ERROR;
}
 


static int
get_logical_address(name, ifield)
	char			*name;
	Ifield			*ifield;
{
	ADDRESS_MAP		*map;

	if ((map = find_addr_map_by_name(name, FIND_SUBSTRING)) == NOT_FOUND) {
		fprintf(stderr, "Couldn't find device \"%s\".\n", name);
		return ERROR;
	}

	*ifield = logical_to_ifield(map->logaddr);

	return NO_ERROR;
}


static int
portlist_to_ifield(portlist, ifield)
	PORTLIST	*portlist;
	Ifield		*ifield;
{
	PORT		*port;
	int			portnum;
	int			num_bits = 0;
	int			bits_shifted;
  
	(*ifield) = 0;

	if (portlist == NULL)
		return NO_ERROR;

	FOR_EACH_PORT_REVERSED(portlist, port, portnum) {
		bits_shifted = port->swp_switch->sw_bits_shifted;
		(*ifield) <<= bits_shifted;
		num_bits += bits_shifted;
		(*ifield) += port->swp_num;
	}

	if (num_bits > MAX_SRC_ROUTE_BITS) {
		fprintf(stderr, "Source route requires too many bits (%d).\n",
				num_bits);
		return ERROR;
	}

	(*ifield) |= CAMPON;

	return NO_ERROR;
}


static void
usage()
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  ifield <options> <host or device>\n");
	fprintf(stderr, "\tReturn a logical address.\n\n");
	fprintf(stderr, "  ifield [options] <host1> <host2> [<host3>] [<host4>]...\n");
	fprintf(stderr, "\tCalculate a source route.\n");
	fprintf(stderr, "  ifield -L <device>\n");
	fprintf(stderr, "\tReturn the physical location of a device.\n");
	fprintf(stderr, "\nOptions:\n");
	fprintf(stderr, "\t-c <config file>   Specify configuration file to use.\n");
	fprintf(stderr, "\t-l	                For source route - loopback path to source.\n");
	fprintf(stderr, "\t-p                 For source route - print hop-by-hop path.\n");
	fprintf(stderr, "\t-v                 Print version information and quit.\n");
}


static int
print_location(device)
	char		*device;
{
	PORT		*port;

	if ((port = find_port_by_name(device, FIND_SUBSTRING)) == NOT_FOUND) {
		fprintf(stderr, "\"%s\" not found.\n", device);
		return ERROR;
	}

	printf("%s %d\n", port->swp_switch->sw_name, port->swp_num);

	return NO_ERROR;
}


static int
print_device(switch_name, port_string)
	char		*switch_name;
	char		*port_string;
{
	SWITCH		*sw;
	PORT		*port;
	int			rc = NO_ERROR;

	if ((sw = find_switch_by_name(switch_name, FIND_SUBSTRING)) == NOT_FOUND) {
		if (is_numeric(switch_name)) {
			if ((sw = find_switch_by_number(atoi(switch_name), FIND_SUBSTRING))
				== NOT_FOUND) {
				fprintf(stderr, "Unknown switch \"%s\"\n", switch_name);
				exit(1);
			}
		}
	}
  
	if (is_numeric(port_string) == FALSE) {
		fprintf(stderr, "Port string \"%s\" illegal.\n", port_string);
		exit(1);
	}

	if ((port = find_port_by_number(sw->sw_ports, atoi(port_string))) == NULL) {
		printf("Unknown\n");
		rc = ERROR;

	} else
		printf("%s\n", port->swp_name);

	return rc;
}
