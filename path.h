/*
 *	path.h
 *
 *	Definitions for the path structure.
 *
 *	These lists are used to hold paths between switches.
 */

#ifndef _PATH_H
#define _PATH_H


#include "basic_defines.h"
#include "port.h"


#define MAX_PORTS_PER_PATH		4


struct path {
  int		num_ports;
  PORT		*ports[MAX_PORTS_PER_PATH];
  int		cost;
};

typedef struct path	PATH;

struct pathlist {
  int		num_hops;
  PATH		**hop;
};

typedef struct pathlist	PATHLIST;


#define	INFINITE_COST		9999999

PATH *malloc_path		PROTO((VOID));
PATH *better_path		PROTO((PATH *path, PORT *port, int cost));
PATH *add_port_to_path		PROTO((PATH *path, PORT *port));
Boolean path_empty		PROTO((PATH *path));

PATHLIST *add_path_to_pathlist	PROTO((PATHLIST *pathlist, PATH *path));
void disperse_pathlist		PROTO((PATHLIST *pathlist));
PATH *first_path		PROTO((PATHLIST *pathlist, int *hop));
PATH *next_path			PROTO((PATHLIST *pathlist, int *hop));

#define FOR_EACH_PORT_IN_PATH(path, port, port_num)		\
	for ((port_num) = 0, (port) = (path)->ports[0];		\
	     (port_num) < (path)->num_ports;			\
	     (port_num)++, (port) = (path)->ports[port_num])



#define FOR_EACH_PATH_IN_PATHLIST(pathlist, path, count)	\
	for ((path) = first_path(pathlist, &(count));		\
	     (path) != NULL;					\
	     (path) = next_path(pathlist, &(count))


#define FIRST_PORT(path)				\
	     ((path)->ports[0])

/*
 *	Do a round robin module thing for switches than don't support
 *	something similar.
 */
#define PRIMARY_PORT(path, logaddr)			\
	     ((path)->ports[logaddr % ((path)->num_ports)])

#endif /* _PATH_H */
