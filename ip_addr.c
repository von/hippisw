/*
 *      ip_addr.c
 *
 *	Routines for handling IP addresses (Netaddrs) and for mapping
 *	IP addresses to logical addresses.
 *
 */

#include "basic_defines.h"
#include "ip_addr.h"
#include "address_config.h"

#include <malloc.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


/*
 *	Convert an IP address to an ascii string of type xx.xx.xx.xx
 *
 *	Returns a pointer to a static buffer.
 */
char *
netaddr_to_ascii(netaddr)
     Netaddr		netaddr;
{
  static char str[HNAMELEN];
  
  sprintf(str, "%u.%u.%u.%u",
	  (netaddr >> 24) & 0xff,
	  (netaddr >> 16) & 0xff,
	  (netaddr >> 8) & 0xff,
	  (netaddr >> 0) & 0xff);
  
  return str;
}

/*
 *	Convert an IP address to a full hostname.
 *
 *	Returns a pointer to a static buffer.
 */
char *
netaddr_to_fullname(netaddr)
     Netaddr		netaddr;
{
  struct hostent	*hinfo;
  static char		str[HNAMELEN];

  hinfo = gethostbyaddr((char *) &netaddr, sizeof(netaddr), AF_INET);

  if (hinfo == NULL) {	/* Punt */
    sprintf(str, "%u.%u.%u.%u",
	    (netaddr >> 24) & 0xff,
	    (netaddr >> 16) & 0xff,
	    (netaddr >> 8) & 0xff,
	    (netaddr >> 0) & 0xff);
  } else {
    strncpy(str, hinfo->h_name, HNAMELEN);
  }

  return str;
}

/*
 *	Convert a hostname to a IP address.
 *
 *	Handles "hippi-xxx" format names.
 *
 *	Returns NETADDR_NULL if an IP address could not be determied.
 */
Netaddr
hostname_to_netaddr(name)
     char		*name;
{
  Netaddr		addr;
  struct hostent	*hinfo;
  
  if((addr = inet_addr(name)) == -1)	{
    if (hinfo = gethostbyname(name))	{
      bcopy(hinfo->h_addr, &addr, hinfo->h_length);
    
    } else {
      addr = NETADDR_NULL;
    }
  }

  return addr;
}

/*
 *	Convert a hostname to a full hostname if possible.
 *
 *	Returns a pointer to a static buffer.
 */
char *
hostname_to_fullname(name)
     char		*name;
{
  static char		fullname[HNAMELEN];
  struct hostent	*hinfo;

  if ((hinfo = gethostbyname(name)) != NULL)
    strncpy(fullname, hinfo->h_name, HNAMELEN);

  else
    strncpy(fullname, name, HNAMELEN);

  return fullname;
}


/*
 *	Check to see if a string is a temporary "hippi-xx" address.
 *
 *	If it is fill netaddr with the network addres if it is or
 *	NETADDR_NULL otherwise and replace hostname with dot address.
 *
 *	Returns ERROR if the hippi address value is illegal.
 */
int
handle_hippi_addr(name, netaddr)
     char		*name;
     Netaddr		*netaddr;
{
  Netaddr		addr;

  if (sscanf(name, "hippi-%d", &addr) == 1) {

    /* Check for illegal value. */
    if ((addr < 1) || (addr > 254))
      return ERROR;

    *netaddr = addr + addr_config.hippi_network;

    strcpy(name, netaddr_to_ascii(*netaddr));

  } else {

    netaddr = NETADDR_NULL;
  }

  return NO_ERROR;
}

