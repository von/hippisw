/*
 *	parse_file.c
 *
 *	Read and tokenize a file.
 */

#include <stdio.h>

#include "basic_defines.h"
#include "parse_file.h"

#define BUFFER_SIZE		240

static	FILE		*file;
static	char		*filename;
static	int			linenumber;
static	Boolean		need_new_line;    	/* Need to read next line	*/
static	char		current_line[BUFFER_SIZE];

static	char		*read_token	PROTO((Boolean stop_at_cr));

/* In strqtok.c */
extern char *strqtok	PROTO((char *string,
							   char *delim,
							   char *quotemarks,
							   char *commentchars,
							   unsigned int flags));


/*	Options for strqtok()
 */
static char 	*sep = " \t\n";
static char 	*quotemarkers = "\"'";
static char 	*commentchars = "#";
static int 		strqtok_flags = 01;		/* Strip quote markers		*/


/*
 *	Open a file for parsing
 */
int
parse_file(name)
	char		*name;
{
	file = fopen(name, "r");

	if (file == NULL)
		return ERROR;

	filename = name;
	linenumber = 0;
	need_new_line = TRUE;

	return NO_ERROR;
}


/*
 *	Close file
 */
void
close_parse()
{
	fclose(file);

	filename = NULL;
	linenumber = 0;
}


/*
 *	Return name of file being parsed
 */
char *
parsed_filename()
{
	return filename;
}


/*
 *	Return linenumber of file being parsed
 */
int
parsed_linenumber()
{
	return linenumber;
}


/*
 * Read token returning NULL if an unescaped \n is encountered.
 */

char *
read_option()
{
 	return read_token(TRUE);
}


/*
 * Read until token is encountered. Returns NULL if EOF is reached.
 */

char *
read_keyword()
{
  	return read_token(FALSE);
}

/*
 *	Return next token from file.
 *
 *	Returns NULL when a new line without preceding whitespace is read.
 *
 *	Will return NULL twice in a row at EOF.
 */
static char *
read_token(stop_at_cr)
	Boolean		stop_at_cr;
{
	char		*token = NULL;


	if (file == NULL)
		return NULL;

	while (token == NULL) {

		/* Read new line if needed */
		if (need_new_line) {
			if (fgets(current_line, BUFFER_SIZE, file) == NULL)	/* EOF */
				return NULL;

			linenumber++;
			need_new_line = FALSE;

			token = strqtok(current_line, sep, quotemarkers, commentchars,
							strqtok_flags);
    
		} else {	/* Read token from current line */

			token = strqtok(NULL, sep, quotemarkers, commentchars,
							strqtok_flags);

		}

		if (token == NULL) {	/* Unescaped CR */

			if (stop_at_cr)
				return NULL;
 
			need_new_line = TRUE;
			continue;
		}

		if (*token == '\\') {
			token = strqtok(NULL, sep, quotemarkers, commentchars,
							strqtok_flags);

			if (token == NULL) {	/* Escaped CR */
				need_new_line = TRUE;
				continue;
			}
		}
	}

	return token;
}

      
/*
 * Read Remainder of line
 */
char *read_line()
{
	return(strqtok(NULL, "\n", quotemarkers, commentchars, strqtok_flags));
}
	

/*
 * Skip to next line
 */
void
next_line()
{
	need_new_line++;
}

/*
 * Skip to next keyword (past next unescaped CR)
 */
void
next_keyword()
{
	while(read_option() != NULL)
		{}
}
      



