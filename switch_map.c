/*
 *	switch_map.c
 *
 *	Routines for determining paths between switches.
 */

#include "basic_defines.h"
#include "path.h"
#include "switch_map.h"
#include "find.h"
#include "switch.h"
#include "port.h"
#include "portlist.h"


/*	An array of pointers to paths.			*/
typedef PATH **		PATH_ARRAY;

/*	An array of arrays				*/
typedef	PATH_ARRAY *	SWITCH_MAP;

static SWITCH_MAP	switch_map = NULL;


static void		build_switch_map	PROTO((VOID));
static void		find_paths		PROTO((int from_switch_num));
static void		find_paths_recursive	PROTO((SWITCH *current_switch,
						       int from_switch_num,
						       PORT *initial_port,
						       int cost,
						       Boolean *visited));

static int		map_size;

#define FOR_ALL_SWITCH_NUMS(switchnum)		\
	for ((switchnum) = 1; (switchnum) < map_size; (switchnum)++)



/*
 *	Build a 2x2 array of paths between switches. The map is
 *	referenced using the switch numbers.
 */
static void
build_switch_map()
{
  int	from_switch;
  int	to_switch;


  /* Add one for easy reference.
   */
  map_size = number_of_switches() + 1;

  /*
   *	Allocate the whole thing.
   */
  switch_map = (SWITCH_MAP) malloc(sizeof(PATH_ARRAY) * map_size);

  if (switch_map == NULL) {
    perror("malloc");
    exit(1);
  }

  FOR_ALL_SWITCH_NUMS(from_switch) {
    
    switch_map[from_switch] = (PATH_ARRAY) malloc(sizeof(PATH *) * map_size);

    if (switch_map[from_switch] == NULL) {
      perror("malloc");
      exit(1);
    }

    FOR_ALL_SWITCH_NUMS(to_switch)
      switch_map[from_switch][to_switch] = malloc_path();
      
  }

  /*
   *	Now we walk through all switch pairs and fill in the mappings.
   */

  FOR_ALL_SWITCH_NUMS(from_switch)
    find_paths(from_switch);

}   


/*
 *	Find the paths between one switch and all others.
 */
static void
find_paths(from_switch_num)
     int		from_switch_num;
{
  SWITCH		*from_switch;
  PORT			*port;
  int			port_num;
  int			switch_num;

  static Boolean	*visited = NULL;


  /*	Set up table of switches already visited.
   */
  if (visited == NULL) {
    visited = (Boolean *) malloc(sizeof(Boolean) * map_size);

    if (visited == NULL) {
      perror("malloc()");
      exit(1);
    }
  }

  FOR_ALL_SWITCH_NUMS(switch_num)
    visited[switch_num] = FALSE;

  visited[from_switch_num] = TRUE;

  from_switch = find_switch_by_number(from_switch_num, FIND_DEFAULT);

#ifdef DEBUG_SWMAP
  fprintf(stderr, "Building map for %s\n", from_switch->sw_name);
#endif

  /*	Start exploring
   */
  FOR_EACH_PORT(from_switch->sw_ports, port, port_num) {
    if (port->swp_type != HIPPI_LINK)
      continue;

    find_paths_recursive(port->link_sw,
			 from_switch_num,
			 port,
			 port->link_metric,
			 visited);
  }
}

static void
find_paths_recursive(current_switch, from_switch_num, initial_port, 
		     cost, visited)
     SWITCH		*current_switch;
     int		from_switch_num;
     PORT		*initial_port;
     int		cost;
     Boolean		*visited;
{
  PATH		*path;
  PORT		*port;
  int		port_num;

  visited[current_switch->sw_num] = TRUE;

#ifdef DEBUG_SWMAP
  fprintf(stderr, "  Transversed to %s\n", current_switch->sw_name);
#endif

  path = switch_map[from_switch_num][current_switch->sw_num];

  /*
   *	Have we found a better route to the current switch?
   */
  if (path->cost == cost) {
    add_port_to_path(path, initial_port);

#ifdef DEBUG_SWMAP
    fprintf(stderr, "    Adding port %d as a route to %s.\n",
	    initial_port->swp_num, current_switch->sw_name);
#endif
  }

  if (path->cost > cost) {
    better_path(path, initial_port, cost);

#ifdef DEBUG_SWMAP
    fprintf(stderr, "    Making port %d the route of choice to %s.\n",
	    initial_port->swp_num, current_switch->sw_name);
#endif
  }

  /*
   *	If the number of the current switch is less than than the
   *	original switch then the current switch has already been mapped.
   *	out and we can use it's tables.
   */
  if (current_switch->sw_num < from_switch_num) {
    PATH	*current_path;
    int		to_switch_num;

#ifdef DEBUG_SWMAP
    fprintf(stderr, "    Using %s's lookup table.\n", current_switch->sw_name);
#endif

    FOR_ALL_SWITCH_NUMS(to_switch_num) {
      path = switch_map[current_switch->sw_num][to_switch_num];
      current_path = switch_map[from_switch_num][to_switch_num];

      if ((path->cost + cost) == current_path->cost) {
	add_port_to_path(current_path, initial_port);
	
#ifdef DEBUG_SWMAP
	fprintf(stderr, "    Adding port %d as a route to switch d.\n",
		initial_port->swp_num, to_switch_num);
#endif
      }

	if ((path->cost + cost) < current_path->cost) {
	  better_path(current_path, initial_port, path->cost + cost);

#ifdef DEBUG_SWMAP
	  fprintf(stderr, "    Making port %d the route of choice to %s.\n",
		  initial_port->swp_num, current_switch->sw_name);
#endif
	} 
      }

    return;
  }

  /*
   *	Otherwise we must keep transversing links.
   */
  FOR_EACH_PORT(current_switch->sw_ports, port, port_num) {
    if (port->swp_type != HIPPI_LINK)
      continue;

    if (visited[port->link_sw->sw_num])
      continue;

    find_paths_recursive(port->link_sw,
			 from_switch_num,
			 initial_port,
			 cost + port->link_metric,
			 visited);
  }

  visited[current_switch->sw_num] = FALSE;

#ifdef DEBUG_SWMAP
  fprintf(stderr, "  Done with %s\n", current_switch->sw_name);
#endif
}


PATH *
find_path(from, to)
     SWITCH		*from;
     SWITCH		*to;
{
  if (switch_map == NULL)
    build_switch_map();

  if (from == to)		/* Undefined */
    return NULL;

  return switch_map[from->sw_num][to->sw_num];
}
