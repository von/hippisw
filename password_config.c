/*
 *	Read the password configuration.
 */

#include "basic_defines.h"
#include "parse_file.h"
#include "parse_token.h"
#include "read_config.h"
#include "password_config.h"
#include "daemon_config.h"
#include "switch.h"
#include "find.h"

#include <string.h>


/*
 *	Keywords
 */
enum keyword_values {
  KEYWD_DEFAULT,
  KEYWD_SWITCH,
  KEYWD_SWITCHES,
  KEYWD_HIPPISWD
};

struct token_mapping tokens[] = {
  { "default",		KEYWD_DEFAULT },
  { "switch",		KEYWD_SWITCH },
  { "switches",		KEYWD_SWITCHES },
  { "hippiswd",		KEYWD_HIPPISWD },
  { NULL,		0 }
};


static void handle_switch_option	PROTO((char *switchname));


/*
 *	Parse the password file.
 */
void
passwd_conf(passwd_file)
     char		**passwd_file;
{
  char			*keyword;
  char			*arg;
  int			token;


  if (strlen(daemon_config.password_file) == 0)
    return;

  if (parse_file(daemon_config.password_file) == ERROR) {
    perror(daemon_config.password_file);
    return;
  }

  if (passwd_file != NULL)
    *passwd_file = daemon_config.password_file;

#ifdef DEBUG_CONF
  fprintf(stderr, "Opened password file: %s\n", daemon_config.password_file);
#endif

  while ((keyword = read_keyword()) != NULL) {
    
    if ((token = parse_token(keyword, tokens)) == TOKEN_NOT_FOUND) {
      config_error("\"%s\" is not recognized. Skipping.\n", keyword);
      next_keyword();
      continue;
    }

    if ((arg = read_option()) == NULL) {
      config_error("Argument required for %s option. Ignoring.\n");
      continue;
    }

    switch(token) {
    case KEYWD_DEFAULT:
      strncpy(password_config.default_sw_password, arg, PASSWDLEN);
      strncpy(password_config.daemon_password, arg, PASSWDLEN);
      break;

    case KEYWD_SWITCHES:
      strncpy(password_config.default_sw_password, arg, PASSWDLEN);
      break;

    case KEYWD_HIPPISWD:
      strncpy(password_config.daemon_password, arg, PASSWDLEN);
      break;

    case KEYWD_SWITCH:	
      handle_switch_option(arg);
      break;
    }
  }

  close_parse();

#ifdef DEBUG_CONF
  fprintf(stderr, "Done parsing password file.\n");
#endif
}


/*
 *	Handle the switch option
 */
static void
handle_switch_option(switchname)
     char		*switchname;
{
  char		*passwd;
  SWITCH	*sw;

  passwd = read_option();

  if (passwd == NULL) {
    config_error("Password required for switch option. Ignoring.\n");
    return;
  }

  sw = find_switch_by_name(switchname, FIND_DEFAULT);

  if (sw == NULL) {
    config_error("Unknown switch \"%s\".\n", switchname);
    return;
  }

  strncpy(sw->sw_password, passwd, PASSWDLEN);
}
