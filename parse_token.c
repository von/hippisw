/*
 *	parse_token.c
 *
 *	Routines for parsing tokens.
 */

#include "basic_defines.h"
#include "parse_token.h"

#include <string.h>


/*
 *	Parse a token returning it's value from a table of mappings.
 */
int
parse_token(token, mappings)
	char					*token;
	struct token_mapping	*mappings;
{
	int						index = 0;

	while (mappings[index].token != NULL) {
		if (strcasecmp(token, mappings[index].token) == 0)
			return mappings[index].value;

		index++;
	}

	return TOKEN_NOT_FOUND;
}
