/*
 *	prompt.c
 *
 *	Prompt identification routines.
 */

#include "basic_defines.h"
#include "prompt.h"
#include "connections.h"

#include <string.h>


Boolean
is_prompt(conn, string)
     Connection			*conn;
     char			*string;
{
  return is_prompt_string(string, conn->sw->sw_prompt);
}



Boolean
is_password_prompt(string)
     char			*string;
{
  return is_prompt_string(string, PASSWORD_PROMPT);
}


Boolean
is_prompt_string(string, prompt)
     char			*string;
     char			*prompt;
{
  int				start_point;

  
  start_point = strlen(string) - strlen(prompt);

  return (strcmp(&string[start_point], prompt) == 0);
}

