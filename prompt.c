/*
 *	prompt.c
 *
 *	Prompt identification routines.
 */

#include "basic_defines.h"
#include "prompt.h"
#include "switch.h"

#include <string.h>


Boolean
is_prompt(sw, string)
	SWITCH			*sw;
	char			*string;
{
    char **prompt;

    prompt = sw->sw_prompts;

    while (*prompt != NULL) {
		if (is_prompt_string(string, *prompt))
			return TRUE;

		prompt++;
    }

    return FALSE;
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
    int	start_point = strlen(string) - strlen(prompt);
    
    return (strcmp(&string[start_point], prompt) == 0);
}	
