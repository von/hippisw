/*
 *	portlist.c
 *
 *	Routines for the portlist structure.
 *
 */

#include "basic_defines.h"
#include "portlist.h"

#include <malloc.h>

/*
 *	Add a port to a port list. Maintains list in order of addition.
 *
 *	If list is NULL a new list is created.
 *
 *	Returns a pointer to the list or NULL on error.
 */
PORTLIST *
add_to_portlist(list, port)
     PORTLIST		*list;
     PORT		*port;
{
  if (list == NULL) {
    list = (PORTLIST *) malloc(sizeof(*list));

    if (list == NULL)
      return NULL;
    
    list->num_ports = 0;
    list->ports = (PORT **) malloc(sizeof(PORT *));

  } else {

    list->ports = (PORT **) realloc(list->ports,
				    sizeof(PORT *) * (list->num_ports + 1));
  }

  if (list->ports == NULL)
    return NULL;
  
  list->ports[list->num_ports] = port;
  list->num_ports++;
  
#ifdef DEBUG_PORTLIST
  fprintf(stderr, "\tAdded %s to port list (%d total).\n",
	  port->swp_name, list->num_ports);
#endif

  return list;
}
      

/*
 *	Sort a list into increasing order by port number.
 *
 */

PORTLIST *
sort_portlist(list)
     PORTLIST		*list;
{
  int		port1, port2;
  PORT		*port;

  if (list == NULL)
    return NULL;

  for (port1 = 0; port1 < list->num_ports; port1++)
    for (port2 = port1 + 1; port2 < list->num_ports; port2++)
      if (list->ports[port2]->swp_num < list->ports[port1]->swp_num) {
	port = list->ports[port2];
	list->ports[port2] = list->ports[port1];
	list->ports[port1] = port;
      }

  return list;
}


/*
 *	Append a portlist to another portlist.
 */

PORTLIST *
append_portlist(list1, list2)
     PORTLIST		*list1;
     PORTLIST		*list2;
{
  int new_length, port_num;

  if (list1 == NULL) {
    list1 = (PORTLIST *) malloc(sizeof(*list1));

    if (list1 == NULL)
      return NULL;

    list1->num_ports = 0;
    list1->ports = NULL;
  }

  if (list2 == NULL)
    return list1;
  
  new_length = list1->num_ports + list2->num_ports;

  if (list1->ports == NULL)
    list1->ports = (PORT **) malloc(sizeof(PORT *) * new_length);
  else
    list1->ports = (PORT **) realloc(list1->ports,
				     sizeof(PORT *) * new_length);

  for (port_num = list1->num_ports; port_num < new_length; port_num++)
    list1->ports[port_num] = list2->ports[port_num - list1->num_ports];

  list1->num_ports = new_length;

  return list1;
}



/*
 *	Free all ports in a list.
 */
void
free_portlist(list)
     PORTLIST		*list;
{
  int		port;

  for (port = 0; port < list->num_ports; port++)
    free(list->ports[port]);

  free(list->ports);
  free(list);
}


/*
 *	Disperse a portlist, but don't deallocate the ports.
 */
void
disperse_portlist(list)
     PORTLIST		*list;
{
  free(list->ports);
  free(list);
}

    

/*
 *	Routines used to walk through a port list.
 */

PORT *
first_port(portlist, portnum)
     PORTLIST		*portlist;
     int		*portnum;
{
  *portnum = 0;

  if (portlist == NULL)
    return NULL;

  if (portlist->num_ports == 0)
    return NULL;

  return portlist->ports[0];
}

PORT *
last_port(portlist, portnum)
     PORTLIST		*portlist;
     int		*portnum;
{
  if (portlist == NULL)
    return NULL;

  if (portlist->num_ports == 0)
    return NULL;

  *portnum = portlist->num_ports - 1;

  return portlist->ports[*portnum];
}

PORT *
next_port(portlist, portnum)
     PORTLIST		*portlist;
     int		*portnum;
{
  if (portlist == NULL)
    return NULL;

  (*portnum)++;

  if ((*portnum) == portlist->num_ports)
    return NULL;

  return (portlist->ports[*portnum]);
}

PORT *
prev_port(portlist, portnum)
     PORTLIST		*portlist;
     int		*portnum;
{
  if (portlist == NULL)
    return NULL;

  (*portnum)--;

  if ((*portnum) < 0)
    return NULL;

  return (portlist->ports[*portnum]);
}


/*
 *	Find a port by number in a portlist.
 *
 *	Returns a pointer to the port or NULL if not found.
 */
PORT *
find_port_by_number(list, portnum)
     PORTLIST		*list;
     int		portnum;
{
  PORT			*port;
  int			num;

  FOR_EACH_PORT(list, port, num)
    if (port->swp_num == portnum)
      return port;

  return NULL;
}
