/*
 *	path.c
 *
 *	Routines for determining paths between switches.
 */

#include "basic_defines.h"
#include "path.h"
#include "port.h"



/*
 *	Malloc a path structure.
 */
PATH *
malloc_path()
{
	PATH	*path;

	path = (PATH *) malloc(sizeof(PATH));

	if (path == NULL) {
		perror("malloc()");
		exit(1);
	}

	path->num_ports = 0;
	path->cost = INFINITE_COST;

	return path;
}


/*
 *	Replace a current path with a better one.
 */
PATH *
better_path(path, port, cost)
	PATH	*path;
	PORT	*port;
	int		cost;
{
	path->num_ports = 1;
	path->ports[0] = port;
	path->cost = cost;

	return path;
}


/*
 *	Add a port to a path.
 */
PATH *
add_port_to_path(path, port)
	PATH		*path;
	PORT		*port;
{
	if (path->num_ports < MAX_PORTS_PER_PATH) {
		int		port_num;

		/*	Check to see if port is already in list...
		 */
		for (port_num = 0; port_num < path->num_ports; port_num++)
			if (path->ports[port_num] == port)
				return path;

		path->ports[path->num_ports] = port;
		(path->num_ports)++;
	}

	return path;
}


/*
 *	Is a path empty?
 */
Boolean
path_empty(path)
	PATH		*path;
{
	if (path == NULL)
		return TRUE;

	if ((path->cost == INFINITE_COST) || (path->num_ports == 0))
		return TRUE;

	return FALSE;
}


/*
 *	Add a path to a pathlist.
 */
PATHLIST *
add_path_to_pathlist(pathlist, path)
	PATHLIST		*pathlist;
	PATH			*path;
{
	if (pathlist == NULL) {
		pathlist = (PATHLIST *) malloc(sizeof(*pathlist));

		if (pathlist == NULL) {
			perror("malloc()");
			exit(1);
		}

		pathlist->num_hops = 0;
	}

	pathlist->hop[pathlist->num_hops] = path;
	pathlist->num_hops++;

	return pathlist;
}

/*
 *	Disperse a pathlist without freeing any paths.
 */
void
disperse_pathlist(pathlist)
	PATHLIST		*pathlist;
{
	if (pathlist != NULL)
		free(pathlist);
}

/*
 *	Walk through a pathlist.
 */
PATH *
first_path(pathlist, hop)
	PATHLIST		*pathlist;
	int				*hop;
{
	*hop = 0;
  
	if (pathlist == NULL)
		return NULL;

	return pathlist->hop[0];
}

PATH *
next_path(pathlist, hop)
	PATHLIST		*pathlist;
	int				*hop;
{
	if (pathlist == NULL)
		return NULL;

	*hop++;

	if ((*hop) < pathlist->num_hops)
		return pathlist->hop[*hop];

	return NULL;
}

