/*
 *	Daemon configuration structure
 */

#ifndef _DAEMON_CONFIG_H
#define	_DAEMON_CONFIG_H

#include "basic_defines.h"


struct daemon_config_struct {
  char		hostname[HNAMELEN];
  int		daemon_port;
  char		working_dir[PATHLEN];
  char		binary_path[PATHLEN];
  char		magic_string[MAGICLEN];
  char		password_file[PATHLEN];
  char		log_file[PATHLEN];
  char		log_command[PATHLEN];
};

#ifdef BUILD_CONFIG_STRUCTURES

struct daemon_config_struct daemon_config =
	{ "", 0, "", "bin/hippiswd", MAGIC_STRING, "config.hippi.password", 
	    "hippiswd.log", "logcmd" };

#else /* BUILD_CONFIG_STRUCTURES */

extern struct daemon_config_struct daemon_config;

#endif /* BUILD_CONFIG_STRUCTURES */

#endif /* _DAEMON_CONFIG_H */
