/*
 * Copyright (c) 1988, 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 *	$Id: telnet.c,v 1.1 1995/02/28 23:17:22 vwelch Exp $
 */

#include <arpa/telnet.h>
#include <malloc.h>
#include "telnet.h"

#define	DATA	0
#define	SB_IAC	1


static struct tcntl	{
	int fd;			/* Socket file descriptor */
	unsigned char sentdo[256];
	unsigned char sentwill[256];
	char unsigned buf[1024];
	int x;
	int cnt;
} *tcbase = 0;
static int ntc = 0;

static unsigned char lm_do[]   = { IAC, DO, 0 };
static unsigned char lm_dont[] = { IAC, DONT, 0 };
static unsigned char lm_wont[] = { IAC, WONT, 0 };
static unsigned char lm_will[] = { IAC, WILL, 0 };

static void dooption __((struct tcntl *, int));
static void dontoption __((struct tcntl *, int));
static void willoption __((struct tcntl *, int));
static void wontoption __((struct tcntl *, int));

void
telnet_init(fd)
int fd;
{
	struct tcntl *tc;
	
	/*
	 * If first call, allocate initial table entry.
	 */
	if(ntc == 0)	{
		tcbase = (struct tcntl *) malloc(sizeof(struct tcntl));
		if((int) tcbase <= 0)	{
			perror("telnet can't malloc table space");
			exit(1);
		}
		ntc = 1;
		tc = tcbase;
		bzero((char *) tc, sizeof(struct tcntl));
		tc->fd = fd;
		return;
	}

	/*
	 * Not first call:  If file is in the table, 
	 * initialize it.
	 */
	for(tc=tcbase; tc < &tcbase[ntc]; tc++)	{
		if(tc->fd == fd)	{
			bzero((char *) tc, sizeof(struct tcntl));
			tc->fd = fd;
			return;
		}
	}
	
	/*
	 * File not in table: allocate a new entry.
	 */
	ntc++;
	if((tcbase = (struct tcntl *)
	   realloc((char *) tcbase, ntc*sizeof(struct tcntl))) == 0)	{
		perror("telnet can't realloc table space");
		exit(1);
	}	
	tc = &tcbase[ntc-1];
	bzero((char *) tc, sizeof(struct tcntl));
	tc->fd = fd;
}

/*
 * Read in data from the network.  Either return the next valid
 * data byte, or:
 *	TELNET_EOF		EOF
 *	TELNET_EOB		End of Buffer
 *	TELNET_OVERFLOW		Buffer overflow
 */

int
telnet(fd, state)
int fd;
int *state;
{
	struct tcntl *tc;
	int c;

	for(tc=tcbase; tc < &tcbase[ntc]; tc++)	{
		if(tc->fd == fd)	{
			break;
		}
	}

	if(tc == &tcbase[ntc])	{
		return(TELNET_EOF);
	}

	if (!tc->cnt) {
		tc->x = 0;
		if ((tc->cnt = read(tc->fd, tc->buf, sizeof(tc->buf))) <= 0) {
			return(TELNET_EOF);
		}
	} else if (tc->x >= tc->cnt) {
		/*
		 * If the last byte in the buffer was a data byte
		 * and we get called again, say we have finished the
		 * buffer so the caller will use select() again.
		 */
		tc->x = tc->cnt = 0;
		return(TELNET_EOB);
	}
	
	while (tc->x < tc->cnt) {
		if(tc->x >= 1023)	{
			tc->x = 0;
			return(TELNET_OVERFLOW);
		}
		c = tc->buf[tc->x++];

		switch (*state) {
		case DATA:
			if (c == IAC)
				*state = IAC;
			else
				return(c);
			break;
		case IAC:
			switch (c) {
			case DO:   case DONT:
			case WILL: case WONT:
				*state = c;
				break;
			case SB:
				*state = SB;
				break;
			case AYT:
				write(tc->fd, "\r\n[YES]\r\n", 9);
				*state = DATA;
				break;
			default:
				*state = DATA;
				break;
			}
			break;
		case SB:
			if (c == IAC)
				*state = SB_IAC;
			break;
		case SB_IAC:
			if (c == IAC) {
				*state = SB;
				break;
			} 
			*state = DATA;
			break;
		case DO:
			dooption(tc, c);
			*state = DATA;
			break;
		case DONT:
			dontoption(tc, c);
			*state = DATA;
			break;
		case WILL:
			willoption(tc, c);
			*state = DATA;
			break;
		case WONT:
			wontoption(tc, c);
			*state = DATA;
			break;
		}
	}
	tc->x = tc->cnt = 0;
	return(TELNET_EOB);
}

static void
dooption(tc, opt)
struct tcntl *tc;
int opt;
{
	if (tc->sentwill[opt] == 0) {
		lm_wont[2] = opt;
		net_write(tc->fd, lm_wont, sizeof(lm_wont));
	} else	{
		--tc->sentwill[opt];
	}
}

static void
willoption(tc, opt)
struct tcntl *tc;
int opt;
{
	if (tc->sentdo[opt] == 0) {
		lm_dont[2] = opt;
		net_write(tc->fd, lm_dont, sizeof(lm_dont));
	} else	{
		--tc->sentdo[opt];
	}
}

static void
dontoption(tc, opt)
struct tcntl *tc;
int opt;
{
	if (tc->sentwill[opt] == 0) {
		lm_wont[2] = opt;
		net_write(tc->fd, lm_wont, sizeof(lm_wont));
	} else	{
		--tc->sentwill[opt];
	}
}

static void
wontoption(tc, opt)
struct tcntl *tc;
int opt;
{
	if (tc->sentdo[opt] == 0) {
		lm_dont[2] = opt;
		net_write(tc->fd, lm_dont, sizeof(lm_dont));
	} else	{
		--tc->sentdo[opt];
	}
}

/*
 * The following functions are required for the telnet/common.a library
 */
int
net_write(fd, str, len)
int fd;
unsigned char *str;
int len;
{
	len = write(fd, (char *)str, len);
	return(len);
}
