/*
 *	port.h
 *
 *	Definitions for a hippisw switch port
 *
 */

#ifndef _PORT_H
#define _PORT_H

#include "basic_defines.h"
#include "ip_addr.h"
#include "logical_addr.h"


#define	SINGLE_WIDE	1
#define	DOUBLE_WIDE	2


/* Things that can be connected to a port */
enum	swp_type	{
	HIPPI_NULL,			/* Nothing connected			*/
	HIPPI_DEVICE,		/* A non-network device			*/
	HIPPI_LINK,			/* A link to another switch		*/
	HIPPI_HOST,			/* A network host				*/
	HIPPI_DX,			/* A DX (HIPPI gateway)			*/
	HIPPI_CNT,			/* A CNT FDDI gateway			*/
	HIPPI_TESTER		/* Hippi tester port			*/
};

#ifdef BUILD_TYPE_STRINGS
char *port_types[] = {
	"NULL",
	"device",
	"link",
	"host",
	"dx",
	"cnt"
};
#endif /* BUILD_TYPE_STRINGS */

/*	A plain hippi device
*/
struct	swp_dev	{
	char		d_name[HNAMELEN];	/* Device name		*/
};

/*	A connection to another switch
 */
struct	swp_link	{
	char			l_swname[SWNAMELEN];	/* Remote switch name	*/
	struct sw    	*l_sw;					/* Remote switch		*/
	int				l_metric;				/* Metric for link		*/
	Boolean			l_default;				/* Is default link?		*/
};

#define DEFAULT_LINK_METRIC		1

/*
 *	Host specific info
 */
struct swp_cray {
	int		idev;			/* Input minor device number	*/
	int		odev;			/* Output minor device number	*/
};

struct swp_giga {
	int		board_num;		/* HIPPI board port #			*/
};

union swp_hi {
	struct swp_cray		cray;
	struct swp_giga		giga;
};

/*	An IP host or gateway
 */
struct	swp_host	{
	char			h_name[HNAMELEN];	/* Internet host name		*/
	int				h_mtu;				/* MTU in bytes				*/
	char			h_ifname[16];		/* Interface name			*/
	Netaddr			h_netaddr; 			/* Internet address			*/
	union swp_hi	h_info;				/* Host-specific info		*/
};

union	swp_u {
	struct swp_dev	d;		/* Device structure		*/
	struct swp_link	l;		/* Link structure		*/
	struct swp_host	h;		/* Host structure		*/
};

/*
 * nicknames for union variables
 */
#define dev_name		swp_thing.d.d_name
#define link_swname		swp_thing.l.l_swname
#define link_sw			swp_thing.l.l_sw
#define	link_default	swp_thing.l.l_default
#define link_metric		swp_thing.l.l_metric
#define host_name		swp_thing.h.h_name
#define host_ifname		swp_thing.h.h_ifname
#define host_mtu		swp_thing.h.h_mtu
#define host_addr		swp_thing.h.h_netaddr

#define cray_idev		swp_thing.h.h_info.cray.idev
#define cray_odev		swp_thing.h.h_info.cray.odev

#define giga_board_num	swp_thing.h.h_info.giga.board_num


struct sw_port	{		/* Port description structure		*/
	struct sw     *swp_switch;		/* Switch attached to					*/
	int				swp_num;		/* Port number							*/
	int				swp_width;		/* DOUBLE_WIDE or SINGLE_WIDE			*/
	enum swp_type	swp_type;		/* Kind of thing connected				*/
	union swp_u 	swp_thing;		/* Appropriate structure				*/
	int				swp_linenum;	/* config file line number				*/
	char	       	*swp_name;		/* Pointer to the name if it has one	*/
	char			swp_comment[COMMENTLEN];
									/* Comment from config file				*/
	Logaddr			swp_default_logaddr;
									/* default logical address				*/
	Boolean			swp_need_disabled;	
									/* Needs to be disabled to set paths?	*/
	Boolean			swp_tester;		/* Is tester port?						*/
};

typedef	struct sw_port	PORT;


/* Does a port do IP? */
#define	HAS_IP(sp)	\
	((((sp)->swp_type == HIPPI_HOST) ||	\
	  ((sp)->swp_type == HIPPI_DX) ||	\
	  ((sp)->swp_type == HIPPI_CNT) ) && 	\
	  ((sp)->host_addr != NETADDR_NULL))
	  
/* Do we do loopback on a port? */
#define DO_LOOPBACK(sp)	\
	((sp)->swp_type != HIPPI_LINK)

/* Values for device numbers */
#define NO_DEVICE_NUMBER	-1


#endif /* _PORT_H */

