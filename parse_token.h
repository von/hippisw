/*
 *	parse_token.h
 *
 *	Definitions for token parsing routines.
 */

#ifndef _PARSE_TOKEN_H
#define _PARSE_TOKEN_H


#define	TOKEN_NOT_FOUND		-1

struct token_mapping {
  char		*token;
  int		value;
};

int parse_token		PROTO((char *token,
			       struct token_mapping *mappings));

#endif /* _PARSE_TOKEN_H */
