/*
 *	parse_file.h
 *
 */

#ifndef _PARSE_FILE_H
#define _PARSE_FILE_H



int parse_file		PROTO((char *name));
void close_parse	PROTO((VOID));
char *parsed_filename	PROTO((VOID));
int parsed_linenumber	PROTO((VOID));
char *read_keyword	PROTO((VOID));
char *read_option	PROTO((VOID));
char *read_line		PROTO((VOID));
void next_line		PROTO((VOID));
void next_keyword	PROTO((VOID));

#endif /* _PARSE_FILE_H */
