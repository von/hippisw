/*
 *	Password information structure
 */

#ifndef _PASSWORD_CONFIG_H
#define _PASSWORD_CONFIG_H

#include "basic_defines.h"


struct password_config_struct {
  char	default_sw_password[PASSWDLEN];
  char	daemon_password[PASSWDLEN];
};


void passwd_conf		PROTO((char **passwd_file));


#ifdef BUILD_CONFIG_STRUCTURES

struct password_config_struct password_config =
	{ "", "" };

#else /* BUILD_CONFIG_STRUCTURES */

extern struct password_config_struct password_config;

#endif /* BUILD_CONFIG_STRUCTURES */

#endif /* _PASSWORD_CONFIG_H */
