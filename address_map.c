/*
 *	address_map.c
 *
 *	Routines for handling IP/Logical Address/Port mappings.
 */

#include "basic_defines.h"
#include "address_map.h"
#include "ip_addr.h"

#include <string.h>


static ADDRESS_MAP	*map = NULL;

/*
 *	Add a mapping to the list.
 *
 *	Keeps list sorted by increasing logical address.
 *
 *	Returns NO_ERROR on success, negitive otherwise (see
 *	address_map.h for a list of errors).
 */
int
add_address_map(hostname, logaddr, port, comment)
	char		*hostname;
	Logaddr		logaddr;
	PORT		*port;
	char		*comment;
{
	ADDRESS_MAP	*entry = map, *add_point = NULL, *new_entry;
	Netaddr		netaddr = NETADDR_NULL;
	Boolean		duplicate_ip = FALSE, duplicate_logaddr = FALSE;


	if (legal_logical_addr(logaddr) == FALSE)
		return ADD_ADDRESS_LA_ILLEGAL;

	if (hostname == NULL) {
		hostname = port->swp_name;

	} else {
		netaddr = hostname_to_netaddr(hostname);

		if (netaddr == NETADDR_NULL)
			return ADD_ADDRESS_HOST_UNKNOWN;
	}

	if (comment == NULL) {
		comment = port->swp_name;
	}

	/*
	 *	Transverse list looking for duplicate IP or logical address.
	 *	add_point will be left pointing at entry that the new_entry
	 *	should follow.
	 */
	while (entry != NULL) {
		if (logaddr >= entry->logaddr)
			add_point = entry;

		/*	If it's the same logical address pointing at different ports
		 *	then it's like completely bogus.
		 *	If it's the same logical addres point at the same port, mark
		 *	it as a duplicate.
		 */
		if (logaddr == entry->logaddr) {
			if (port == entry->port)
				duplicate_logaddr = TRUE;
			else
				return ADD_ADDRESS_LA_DUP;
		}

		/* If it's the same IP address pointing at different logical
		 * addresses then it's bad.
		 * If it's the same IP address and same logical address, then
		 * see if the hostnames are identical. If they are then ignore
		 * the new entry, otherwise add it but mark it as a funcitional
		 * duplicate.
		 */
		if ((netaddr != NETADDR_NULL) && (netaddr == entry->netaddr)) {
			if (logaddr != entry->logaddr)
				return ADD_ADDRESS_IP_DUP;

			/* Is it a complete duplicate? */
			if (strcmp(hostname, entry->hostname) == 0)
				return NO_ERROR;
			else
				duplicate_ip = TRUE;
		}

		entry = entry->next_map;
	}

	/*
	 * At this point we know we have a legal mapping, so add it.
	 */
	new_entry = (ADDRESS_MAP *) malloc(sizeof(*new_entry));

	if (new_entry == NULL)
		return ADD_ADDRESS_MALLOC_ERROR;

	strncpy(new_entry->hostname, hostname, HNAMELEN);
	strncpy(new_entry->comment, comment, COMMENTLEN);
	new_entry->netaddr = netaddr;
	new_entry->logaddr = logaddr;
	new_entry->port = port;
	new_entry->duplicate_ip = duplicate_ip;
	new_entry->duplicate_logaddr = duplicate_logaddr;

	if (add_point == NULL) {
		new_entry->next_map = map;
		map = new_entry;
	} else {
		new_entry->next_map = add_point->next_map;
		add_point->next_map = new_entry;
	}

	return NO_ERROR;
}


/*
 *	Return a mapping for a given logical address.
 */
LOGICAL_MAP *
find_map_by_logaddr(logaddr)
	Logaddr		logaddr;
{
	ADDRESS_MAP		*entry = map;

	while (entry != NULL) {
		if (entry->logaddr == logaddr)
			return entry;

		entry = entry->next_map;
	}

	return NULL;
}



/*
 *	Return a mapping for a given IP address.
 */
HOST_MAP *
find_map_by_netaddr(netaddr)
	Netaddr		netaddr;
{
	ADDRESS_MAP		*entry = map;

	while (entry != NULL) {
		if (entry->netaddr == netaddr)
			return entry;

		entry = entry->next_map;
	}

	return NULL;
}



/*
 *	Return maps with an IP to logical address mapping.
 */

static ADDRESS_MAP *current_host_map = NULL;

HOST_MAP *
first_host_map()
{
	current_host_map = map;

	return next_host_map();
}

HOST_MAP *
next_host_map()
{
	HOST_MAP	*entry;

	if (current_host_map == NULL)
		return NULL;

	while (current_host_map->netaddr == NETADDR_NULL) {
		current_host_map = current_host_map->next_map;

		if (current_host_map == NULL)
			return NULL;
	}

	entry = current_host_map;

	current_host_map = current_host_map->next_map;

	return entry;
}


/*
 *	Return maps with a logical address to port mapping.
 *
 *	If port is non-NULL, return only mappings to that port.
 */

static ADDRESS_MAP *current_logical_map = NULL;
static PORT *current_port = NULL;

LOGICAL_MAP *
first_logical_map(port)
	PORT		*port;
{
	current_logical_map = map;
	current_port = port;

	return next_logical_map();
}

LOGICAL_MAP *
next_logical_map()
{
	LOGICAL_MAP *entry;

	if (current_logical_map == NULL)
		return NULL;

	if (current_port != NULL)
		while (current_logical_map->port != current_port) {
			current_logical_map = current_logical_map->next_map;

			if (current_logical_map == NULL)
				return NULL;
		}

	entry = current_logical_map;

	current_logical_map = current_logical_map->next_map;

	return entry;
}
