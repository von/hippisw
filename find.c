/*
 *	find.c
 *
 *	Routines for finding ports or switches.
 */

#include "basic_defines.h"
#include "find.h"
#include "switchlist.h"
#include "portlist.h"
#include "port.h"
#include "switch.h"
#include "logical_addr.h"
#include "address_map.h"

#include <string.h>

static Boolean string_match	PROTO((char *string1, 
				       char *string2, 
				       int flags));


/*
 *	Find a switch by name.
 *
 *	Returns pointer to switch or NOT_FOUND.
 */
SWITCH *
find_switch_by_name(name, flags)
     char		*name;
     int		flags;
{
  SWITCH	*sw;

  FOR_ALL_SWITCHES(sw)
    if (string_match(name, sw->sw_name, flags))
      return sw;

  return NOT_FOUND;
}


/*
 *	Find a switch by number.
 *
 *	Returns a pointer to the switch or NOT_FOUND.
 */
SWITCH *
find_switch_by_number(number, flags)
     int		number;
     int		flags;
{
  SWITCH	*sw;
  int		portnum;

  FOR_ALL_SWITCHES(sw)
    if (sw->sw_num == number)
      return sw;

  return NOT_FOUND;
}



/*
 *	Find a port by name.
 *
 *	Returns a pointer to the port or NOT_FOUND.
 */
PORT *
find_port_by_name(name, flags)
     char		*name;
     int		flags;
{
  SWITCH	*sw;
  PORT		*port;
  int		portnum;

  FOR_ALL_SWITCHES(sw)
    FOR_EACH_PORT(sw->sw_ports, port, portnum)
      if (string_match(name, port->swp_name, flags))
	return port;

  return NOT_FOUND;
}


/*
 *	Find an address mapping by name.
 *
 *	Returns a pointer to the mapping or NOT_FOUND
 */
ADDRESS_MAP *
find_addr_map_by_name(name, flags)
     char		*name;
     int		flags;
{
  ADDRESS_MAP		*map;

  FOR_ALL_LOGICAL_MAPS(map)
    if (string_match(name, map->hostname, flags))
      return map;

  return NOT_FOUND;
}



/*
 *	Do a pair of strings match? Perform a case-insensitive comparison.
 *
 *	Flag are used as follows:
 *
 *	if FIND_SUBSTRING is set then string1 need only match up to
 *	it's length of string2.
 *
 *	Returns TRUE on match, false otherwise.
 */
static Boolean
string_match(string1, string2, flags)
     char		*string1;
     char		*string2;
     int		flags;
{
  if (flags & FIND_SUBSTRING)
    return (strncasecmp(string1, string2, strlen(string1)) == 0);
  else
    return (strcasecmp(string1, string2) == 0);
}
    

