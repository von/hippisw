/*
 *	address_conf.c
 *
 *	Read addressing (IP and logical) configuration.
 */

#include "basic_defines.h"
#include "address_config.h"
#include "parse_file.h"
#include "parse_token.h"
#include "switch.h"
#include "port.h"

/*
 *	Keywords
 */
enum keyword_values {
	KEYWD_NETWORK,
	KEYWD_LOOPBACK,
	KEYWD_DEFAULT_TYPE,
	KEYWD_MODE_BITS,
	KEYWD_COMMENT
};

static struct token_mapping keywords[] = {
	{ "network",		KEYWD_NETWORK },
	{ "loopback",		KEYWD_LOOPBACK },
	{ "default_type",	KEYWD_DEFAULT_TYPE },
	{ "mode_bits",		KEYWD_MODE_BITS },
	{ "#",				KEYWD_COMMENT },
	{ NULL, 0 }
};

static struct token_mapping default_types[] = {
	{ "none",			NO_DEFAULT_LA },
	{ "1374",			DO_LA_1374 },
	{ "ip8",			DO_LA_IP_8 },
	{ "ip12",			DO_LA_IP_12 },
	{ NULL, 0 }
};


/*
 *	Read the address configurtion.
 */
void 
address_conf()
{
	char		*keyword;
	char		*argument;
	int			token;

	Logaddr	logaddr;


	while ((keyword = read_option()) != NULL) {

		if ((token = parse_token(keyword, keywords)) == TOKEN_NOT_FOUND) {
			config_error("\"%s\" is not recognized. Skipping.\n", keyword);
			continue;
		}

		/*	Currently all options take arguments
		 */
		argument = read_option();

		if (argument == NULL) {
			config_error("Argument required for %s option.\n", keyword);
			continue;
		}

		switch(token) {
		case KEYWD_NETWORK:
			addr_config.hippi_network = hostname_to_netaddr(argument);
			break;

		case KEYWD_LOOPBACK:
			logaddr = str_to_log(argument);

			if (legal_logical_addr(logaddr) == FALSE)
				config_error("Loopback address %#x is not a legal value.\n",
							 logaddr);
			else
				addr_config.loopback_addr = logaddr;

			break;

		case KEYWD_DEFAULT_TYPE:
			if ((token = parse_token(argument, default_types)) == TOKEN_NOT_FOUND)
				config_error("\"%s\" is an unrecognized default type. Ignoring.\n",
							 argument);
			else
				addr_config.default_type = token;
      
			break;

		case KEYWD_MODE_BITS:
			logaddr = str_to_log(argument);

			if (legal_mode_bits(logaddr) == FALSE)
				config_error("Mode bits value %#x is not legal.\n", logaddr);
			else
				addr_config.mode_bits = logaddr << MODE_BITS_SHIFT;

			break;
		}
	}
}


/*
 *	Return a default logical address given a port
 *
 *	Returns LOGADDR_NULL if none.
 */
Logaddr
get_default_logical(port)
	PORT		*port;
{
	Logaddr logaddr;

	if (port->swp_type == HIPPI_LINK)
		return LOGADDR_NULL;

	switch(addr_config.default_type) {
	case NO_DEFAULT_LA:
		logaddr = LOGADDR_NULL;
		break;

	case DO_LA_1374:
		logaddr = (port->swp_switch->sw_num << 5) + port->swp_num;
		break;

	case DO_LA_IP_8:
		if (HAS_IP(port))
			logaddr = port->host_addr & 0xff;
		else
			logaddr = LOGADDR_NULL;
		break;

	case DO_LA_IP_12:
		if (HAS_IP(port))
			logaddr = port->host_addr & 0xfff;
		else
			logaddr = LOGADDR_NULL;
		break;
	}

	return logaddr;
}


/*
 *	Return the address to be used for tester loopback to a given switch
 */
Logaddr
get_tester_address(sw)
	SWITCH		*sw;
{
	return (addr_config.tester_addr + sw->sw_num);
}
