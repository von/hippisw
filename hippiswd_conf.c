/*
 *	hippiswd_conf.c
 *
 *	Read hippiswd configuration.
 */

#include "basic_defines.h"
#include "daemon_config.h"
#include "parse_file.h"
#include "parse_token.h"


/*
 *	Keywords
 */
enum keyword_values {
  KEYWD_HOST,
  KEYWD_PORT,
  KEYWD_DIR,
  KEYWD_MAGIC,
  KEYWD_PASSWDFILE,
  KEYWD_LOGFILE,
  KEYWD_LOGCMD,
  KEYWD_COMMENT
};

static struct token_mapping keywords[] = {
  { "host",		KEYWD_HOST },
  { "port",		KEYWD_PORT },
  { "dir",		KEYWD_DIR },
  { "magic",		KEYWD_MAGIC },
  { "passwdfile",	KEYWD_PASSWDFILE },
  { "logfile",		KEYWD_LOGFILE },
  /* For backwards compatibility */
  { "mailcmd",		KEYWD_LOGCMD },
  { "logcmd",		KEYWD_LOGCMD },
  { "#",		KEYWD_COMMENT },
  { NULL, 0 }
};

/*
 *	Read the hippiswd configuration.
 */
void
hippiswd_conf()
{
  char		*keyword;
  char		*argument;
  int		token;

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
    case KEYWD_HOST:
      strncpy(daemon_config.hostname, argument, HNAMELEN);
      break;

    case KEYWD_PORT:
      daemon_config.daemon_port = str_to_int(argument);
      break;

    case KEYWD_DIR:
      strncpy(daemon_config.working_dir, argument, PATHLEN);
      break;

    case KEYWD_MAGIC:
      strncpy(daemon_config.magic_string, argument, MAGICLEN);
      break;

    case KEYWD_PASSWDFILE:
      strncpy(daemon_config.password_file, argument, PATHLEN);
      break;

    case KEYWD_LOGFILE:
      strncpy(daemon_config.log_file, argument, PATHLEN);
      break;

    case KEYWD_LOGCMD:
      strncpy(daemon_config.log_command, argument, PATHLEN);
      break;

    case KEYWD_COMMENT:
      read_line();
      break;
    }
  }
}
