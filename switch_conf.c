/*
 *	switch_conf.c
 *
 *	Read a switch configuration.
 *
 */

#include "basic_defines.h"
#include "switch.h"
#include "parse_file.h"
#include "parse_token.h"
#include "ip_addr.h"
#include "switchlist.h"

/*
 *	Keywords
 */
enum keyword_values {
  KEYWD_SIZE,
  KEYWD_ADDRESS,
  KEYWD_PORT,
  KEYWD_PROMPT,
  KEYWD_SMS,
  KEYWD_COMMENT
};

static struct token_mapping keywords[] = {
  { "address",	KEYWD_ADDRESS },
  { "port", 	KEYWD_PORT },
  { "prompt",	KEYWD_PROMPT },
  { "size",	KEYWD_SIZE },
  { "sms",	KEYWD_SMS },
  { "comment",	KEYWD_COMMENT },
  { NULL, 0 }
};

static struct token_mapping switch_keywords[] = {
  { "p8",	HIPPISW_P8 },
  { "ps4",	HIPPISW_PS4 },
  { "ps8",	HIPPISW_PS8 },
  { "ps32",	HIPPISW_PS32 },
  { "ios4",	HIPPISW_IOSC4 },
  { "iosc4",	HIPPISW_IOSC4 },
  { "ios8",	HIPPISW_IOSC8 },
  { "iosc8",	HIPPISW_IOSC8 },
  { "netstar",	HIPPISW_NETSTAR },
  { "giga",	HIPPISW_NETSTAR },
  { "es1",	HIPPISW_ES1 },
  { "virtual",	HIPPISW_VIRT },
  { NULL,	0}
};

/*
 *	Read a switch configuration
 */
SWITCH *
switch_conf()
{
  char		*keyword;
  char		*argument;
  int		token;

  SWITCH	*sw;

  char		*name;

  struct switch_attributes	*attributes;


  sw = (SWITCH *) malloc(sizeof(SWITCH));

  if (sw == NULL) {
    config_error("malloc() failed.\n");
    exit(1);
  }

  name = read_option();		/* Read name			*/
  
  argument = read_option();	/* Read type			*/

  if ((name == NULL) || (argument == NULL)) {
    config_error("Name and type required for switch. Skipping.\n");
    next_keyword();
    return NULL;
  }

  sw->sw_type = parse_token(argument, switch_keywords);

  if (sw->sw_type == TOKEN_NOT_FOUND) {
    config_error("Switch type \"%s\" unknown. Assuming PS32.\n", argument);
    sw->sw_type = HIPPISW_PS32;
  }

  /*
   *	Fill in defaults
   */
  strncpy(sw->sw_name, name, SWNAMELEN);
  sw->sw_version = 1;

  /*	Set a bunch of stuff here in one wave of the hand	*/
  attributes = get_sw_attributes(sw->sw_type);
  bcopy(attributes, &(sw->sw_attributes), sizeof(sw->sw_attributes));
     
  sw->sw_ports = NULL;
  NULL_STRING(sw->sw_hostname);
  sw->sw_tport = TELNET_PORT;
  NULL_STRING(sw->sw_prompt);
  sw->sw_linenum = parsed_linenumber();
  NULL_STRING(sw->sw_comment);
  sw->sw_virt_attached  = NULL;
  NULL_STRING(sw->sw_password);
  
  /*
   *	Read rest of options
   */
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
    case KEYWD_SIZE:
      if (is_numeric(argument) == FALSE) {
	config_error("\"%s\" is not a legal numeric argument. Skipping.\n",
		     argument);
	continue;
      }
      sw->sw_num_ports = str_to_int(argument);

      break;

    case KEYWD_ADDRESS:
      if (hostname_to_netaddr(argument) == NETADDR_NULL)
	config_error("Warning: can't resolve net address for \"%s\".\n",
		     argument);
      
      strncpy(sw->sw_hostname, argument, HNAMELEN);
      break;

    case KEYWD_PORT:
      if (is_numeric(argument) == FALSE) {
	config_error("\"%s\" is not a legal numeric argument. Skipping.\n",
		     argument);
	continue;
      }
      sw->sw_tport = str_to_int(argument);
      break;

    case KEYWD_PROMPT:
      strncpy(sw->sw_prompt, argument, PROMPTLEN);
      break;

    case KEYWD_SMS:
      if (is_numeric(argument) == FALSE) {
	config_error("\"%s\" is not a legal numeric argument. Skipping.\n",
		     argument);
	continue;
      }
      sw->sw_version = str_to_int(argument);
      break;

    case KEYWD_COMMENT:
      strncpy(sw->sw_comment, argument, COMMENTLEN);
      break;
    }
  }

  /* If switch prompt was not set, set it to the default
   */
  if (strlen(sw->sw_prompt) == 0)
    strcpy(sw->sw_prompt, get_sw_prompt(sw->sw_type, sw->sw_version));

  if ((sw->sw_num = add_switch(sw)) == ERROR) {
    config_error("Error adding switch to list.\n");
    exit(1);
  }

  return sw;
}

      
      
      
  

