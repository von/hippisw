/*
 *	prompt.h
 */

#ifndef _PROMPT_H
#define _PROMPT_H

#include "basic_defines.h"
#include "switch.h"

#define PASSWORD_PROMPT		"Password: "

/*	Last char of password prompt		*/
#define	PASSWD_PROMPT_CHAR	':'


Boolean is_prompt			PROTO((SWITCH *conn,
								   char *string));
Boolean is_password_prompt	PROTO((char *string));
Boolean is_prompt_string	PROTO((char *string,
								   char *prompt));

#endif /* _PROMPT_H */
