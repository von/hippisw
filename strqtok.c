/*
 *	AUTHOR:		William Deich
 *			will@surya.caltech.edu
 *
 */

#include <ctype.h>
#include <string.h>

/* If STRQTOK_DEBUG is defined, then  "int strqtok_debug"  is defined
 * and initialized to 0.  If another routine sets strqtok_debug !0,
 * then the input arguments are printed at the beginning of each call
 * to strqtok() and the to-be returned token value is printed at the end of
 * the routine.
 * If STRQTOK_DEBUG is undefined, then strqtok() is compiled without
 * strqtok_debug and without any calls to stdio routines.
 */
#define STRQTOK_DEBUG

#ifdef STRQTOK_DEBUG
#include <stdio.h>
int strqtok_debug=0;
#endif

#define translate_octal(cc, s)					\
{								\
    cc = 0; 							\
    if (isdigit(*s) && *s != '8' && *s != '9') {		\
	cc = *s++ - '0';					\
	if (isdigit(*s) && *s != '8' && *s != '9') {		\
	    cc <<= 3;						\
	    cc |= *s++ - '0';					\
	    if (isdigit(*s) && *s != '8' && *s != '9') {	\
		cc <<= 3;					\
		cc |= *s++ - '0';				\
	    }							\
	}							\
    }								\
}

#define translate_hex(cc, s)					\
    for (cc = 0; isxdigit(*s); s++) {				\
	cc <<= 4;						\
	cc |= isdigit(*s) ? *s - '0' :				\
		islower(*s) ? *s + 10 - 'a' : *s + 10 - 'A';	\
    }

/* The pointer into the string, indicating where to start next token search.
 * The purpose of making it extern is that the caller can stash away
 * a copy of strq_start, process another string, then restore the pointer
 * and finish processing the first string.
 */
char *strq_start=NULL;

/* strqtok(s, delim, quotemarks, commentchars, flags)
 * ...is same as strtok(s, delim), except that
 *	o  quotemarks: tokens are quoted by pairs of these characters,
 *		allowing characters that are normally
 *		token delimiters to be part of a token.
 *	o  commentchars: if one of these characters occurs outside a
 *		quoted token, it ends the line as if it were '\0'.
 *	o  flags:
 *		flags & 01 ->	quotemarks are stripped from tokens.
 *				Default: quotemarks are left in returned
 *				tokens.
 *		flags & 02 ->	use ANSI-C rules for interpreting backslash
 *				sequences.
 *				Default: no ANSI C rules used.
 *		flags & 04 ->	don't stop at first delimiter; write the null,
 *				then skip any additional delimiters before the
 *				next word; upon return, the internal pointer
 *				is left pointing to the next token or the
 *				final NULL.
 *				Default: processing stops at first delimiter;
 *				the internal pointer is left pointing to
 *				the character following the delimiter.
 *		flags & 010 ->	ANSI-C trigraphs are interpreted and replaced
 *				with their corresponding characters.  Implies
 *				bit 02 (backslash sequences).
 *	
 * See man page for details.
 */

char *
strqtok(s, delim, quotemarks, commentchars, flags)
    char *s;		/* String to tokenize.  NULL to continue same str */
    char *delim;	/* Token delimiters.  Can be changed w/ each call. */
    char *quotemarks;	/* Quotation marks.  Can be changed w/ each call. */
    char *commentchars;	/* Comment characters.  Can be changed w/ each call. */ 
    unsigned int flags;	/*	flags&01 ->	strip quotes;
			 *	flags&02 ->	enable backslash escapes;
			 *	flags&04 ->	skip all delims before return;
			 *	flags&010 ->	trigraphs (also implies 02);
			 */
{
    register char *p, *q;
    char c;
    char leftquote;
    char *token;
    char *Cchar();
    int backslashed, inquote, intok;

    int stripquote = flags & 01;	/* strip quotemarks from tokens */
    int backslash = (flags & 02) | (flags & 010); /* backslash sequences */
    int skipdelim = flags & 04;		/* skip seq of delims at end of token */
    int trigraph = flags & 010;		/* trigraphs */
    /* New string? */
    if (s)
	strq_start = s;

    if (!strq_start)
	return (char *) NULL;
    
    /* Skip leading delimiters */
    for (p=strq_start; *p && strchr(delim, *p); p++)
	;

    if (!(*p) || strchr(commentchars, *p))
	return (char *) NULL;

#ifdef STRQTOK_DEBUG
    if (strqtok_debug) {
	(void) fprintf(stderr, "@@@ strqtok: start with `%s'\n", p);
	(void) fprintf(stderr, "@@@ strqtok: delim = `%s'\n", delim);
	(void) fprintf(stderr, "@@@ strqtok: quotemarks = `%s'\n", quotemarks);
	(void) fprintf(stderr, "@@@ strqtok: commentchars = `%s'\n",
								commentchars);
	(void) fprintf(stderr, "@@@ strqtok: flags = %#o\n", flags);
    }
#endif

    /* Set `token' to point to returned string.
     * Use p and q to walk through the user's string:
     *    p will follow input characters;
     *    q will overwrite w/ outputted characters, minus possibly-stripped
     *		quotes and including nulls after each token.
     */
    token = q = p;
    inquote = 0;
    intok = 1;
    if (backslash) {
	while (intok && (p=Cchar(p, &c, trigraph, &backslashed))) {
	    if (backslashed) {
		*q++ = c;		/* treat as plain character */
	    } else if (!inquote && *delim && strchr(delim, c)) {
		*q = '\0';		/* Reached end of token */
		intok = 0;
	    } else if (!inquote && *commentchars && strchr(commentchars, c)) {
		*q = '\0';		/* Reached end of token */
		*p = '\0';		/* make it act like end of string */
		intok = 0;
	    } else if (!inquote && *quotemarks && strchr(quotemarks, c)) {
		inquote = 1;		/* Beginning a quoted segment */
		leftquote = c;		/* Save quote char for matching with */
		if (!stripquote) *q++ = c;
	    } else if (inquote && leftquote == c) {
		inquote = 0;		/* Ending a quoted segment */
		if (!stripquote) *q++ = c;
	    } else {
		*q++ = c;		/* Ordinary character */
	    }
	}
	strq_start = p;			/* Where to start next search */
	*q = '\0';
    } else {
	while (intok && *p) {
	    if (!inquote && *delim && strchr(delim, *p)) {
		*q = '\0';		/* Reached end of token */
		p++;			/* advance p for next token */
		intok = 0;
	    } else if (!inquote && *commentchars && strchr(commentchars, *p)) {
		*q = '\0';		/* Reached end of token */
		*p = '\0';		/* make it act like end of string */
		intok = 0;
	    } else if (!inquote && *quotemarks && strchr(quotemarks, *p)) {
		inquote = 1;		/* Beginning a quoted segment */
		leftquote = *p++;	/* Save quote char for matching with */
		if (!stripquote) *q++ = leftquote;
	    } else if (inquote && leftquote == *p) {
		inquote = 0;		/* Ending a quoted segment */
		p++;
		if (!stripquote) *q++ = leftquote;
	    } else {
		*q++ = *p++;
	    }
	}
	strq_start = p;			/* Where to start next search */
	*q = '\0';
    }
#ifdef STRQTOK_DEBUG
    if (strqtok_debug)
	(void) fprintf(stderr, "@@@ strqtok: token is `%s'\n", token);
#endif

    if (skipdelim && strq_start) {
	/* Skip trailing delimiters */
	while (*strq_start && strchr(delim, *strq_start))
	    strq_start++;
    }
    return token;
}

char *
Cchar(s, c, do_trigraph, backslashed)
    register char *s;	/* string from which to collect character */
    register char *c;	/* Return character here */
    int do_trigraph;	/* If !0, interpret trigraphs according to ANSI rules */
    int *backslashed;	/* Return !0 if character was started w/ backslash */

	/* Gets the leading ANSI C character from a string.  Here, "ANSI C
	 * character" means that the string's leading characters are
	 * interpreted according to ANSI C rules; i.e. backslash escapes
	 * are properly interpreted.  However, ANSI trigraphs are treated
	 * as plain characters unless do_trigraph !=0.
	 * Return value is ptr to next character in s; when at end of string,
	 * returned ptr is NULL.
	 */
{
    register char ch;

    if ((*backslashed = (*s == '\\'))) {
	switch (*++s) {
	case 'a':
	    *c = '\a';
	    break;
	case 'b':
	    *c = '\b';
	    break;
	case 'f':
	    *c = '\f';
	    break;
	case 'n':
	    *c = '\n';
	    break;
	case 'r':
	    *c = '\r';
	    break;
	case 't':
	    *c = '\t';
	    break;
	case 'v':
	    *c = '\v';
	    break;
	case '\\':
	    *c = '\\';
	    break;
	case '^':
	    *c = '^';
	    break;
	case '\'':
	    *c = '\'';
	    break;
	case '"':
	    *c = '"';
	    break;
	case '?':
	    *c = '?';
	    break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	    translate_octal(ch, s);
	    s--;
	    *c = ch;
	    break;
	case 'x':
	    s++;
	    translate_hex(ch, s);
	    s--;
	    *c = ch;
	    break;
	default:
	    *c = *s;
	    break;
	}
    } else if (do_trigraph && (*s == '?') && (*(s+1) == '?')) {
	switch (*(s+2)) {
	case '=':
	    *c = '#';
	    s += 2;
	    break;
	case '(':
	    *c = '[';
	    s += 2;
	    break;
	case '/':
	    *c = '\\';
	    s += 2;
	    break;
	case ')':
	    *c = ']';
	    s += 2;
	    break;
	case '\'':
	    *c = '^';
	    s += 2;
	    break;
	case '<':
	    *c = '{';
	    s += 2;
	    break;
	case '!':
	    *c = '|';
	    s += 2;
	    break;
	case '>':
	    *c = '}';
	    s += 2;
	    break;
	case '-':
	    *c = '~';
	    s += 2;
	    break;
	default:
	    /* Not a trigraph sequence */
	    *c = *s;
	}
	*c = *s;
    } else {
	*c = *s;
    }
    return (*s) ? s+1 : NULL;
}

#ifdef TEST

#ifndef STRQTOK_DEBUG
#include <stdio.h>
#endif

#include <stdlib.h>
main()
{
    char delim[1000];
    char quotes[1000];
    char comments[1000];
    char buf[1000];
    unsigned int flags;
    char *s;

    #ifdef ASCII
	printf("(System uses ASCII representations of 0-9, a-z, A-Z)\n");
    #else
	printf("(System doesn't use ASCII representations of 0-9, a-z, A-Z)\n");
    #endif
    printf("Enter token delimiters: ");
    gets(delim);
    printf("Enter quote marks: ");
    gets(quotes);
    printf("Enter comment marks: ");
    gets(comments);
    printf("Enter flags:\n");
    printf("    01 -> strip quotemarks from tokens\n");
    printf("    02 -> interpret backslash sequences\n");
    printf("    04 -> skip delimiters before returning\n");
    printf("   010 -> interpret trigraphs (implies 02)\n");
    gets(buf);
    flags = strtoul(buf, NULL, 0);
    printf("flags = %#o\n", flags);

    while (1) {
	printf("Enter string to parse: ");
	gets(buf);
	for (s = strqtok(buf, delim, quotes, comments, flags); s;
			s = strqtok(NULL, delim, quotes, comments, flags))
		printf("\t`%s'\n", s);
    }
}
#endif
