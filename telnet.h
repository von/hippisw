/*
 *	telnet.h
 *
 *	$Id: telnet.h,v 1.1 1995/02/28 23:17:27 vwelch Exp $
 */

#include <stdio.h>
#ifdef	__
#undef	__
#endif

#ifdef	__STDC__
#define	__(x)	x
#else
#define	__(x)	()
#endif

int telnet __((int, int *));
void telnet_init __((int));
void send_do __((int));
void send_will __((int));

int net_write __((int, unsigned char *, int));
void net_encrypt __((void));
int telnet_spin __((void));


#define	TELNET_EOF		-1
#define TELNET_EOB		-2
#define TELNET_OVERFLOW		-3
