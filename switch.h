/*
 *	switch.h
 *
 *	Definitions for a hippisw switch
 *
 */

#ifndef _SWITCH_H
#define _SWITCH_H

#include "basic_defines.h"
#include "portlist.h"




typedef enum		{	/* Types of switches		*/
	HIPPISW_P8,			/* NSC P8					*/
	HIPPISW_PS4,		/* NSC PS4					*/
	HIPPISW_PS8,		/* NSC PS8					*/
	HIPPISW_PS32,		/* NSC PS32					*/
	HIPPISW_IOSC4,		/* IOSC 4x4					*/
	HIPPISW_IOSC8,		/* IOSC 8x8					*/
	HIPPISW_NETSTAR,	/* Netstat Cluster Switch	*/
	HIPPISW_ES1,		/* Essential ES1			*/
	HIPPISW_VIRT		/* Virtual Switch			*/
} SW_TYPE;


#ifdef BUILD_TYPE_STRINGS
char *switch_types[] = {
	"NSC P8",
	"NSC PS4",
	"NSC PS8",
	"NSC PS32",
	"IOSC 4x4",
	"IOSC 8x8",
	"Netstar Cluster Switch",
	"Essential ES-1",
	"Virtual Switch"
};

#endif /* BUILD_TYPE_STRINGS */


/*	Switch attributes
 */
struct switch_attributes {
	SW_TYPE		type;
	int			num_ports;
	Boolean		is_smart;
	Boolean		has_default;
	int			bits_shifted;
	char		**prompts;   
};
  
struct switch_attributes *get_sw_attributes	PROTO((SW_TYPE type));

struct sw	{			/* Switch description structure	*/
	char						sw_name[SWNAMELEN];
										 		/* Switch name			*/
	int							sw_num;			/* Switch number		*/
	int							sw_version;		/* Switch version		*/
	struct switch_attributes 	sw_attributes; 	
												/* Attributes			*/
	PORTLIST					*sw_ports; 		/* Pointer to port list	*/
	char						sw_hostname[HNAMELEN];
											 	/* Telnet host name		*/
	int							sw_tport;		/* Telnet port number	*/
	int							sw_linenum;		/* config file line
												   number				*/
	char						sw_comment[COMMENTLEN];
												/* Comment from config
												   file line			*/
	struct sw_port				*sw_virt_attached;
												/* For virtual switches
												   this points to the
												   switch they are
												   attached to. NULL
												   for all other
												   switches.			*/
	char						sw_password[PASSWDLEN];
												/* Password for switch	*/
	struct sw					*sw_next;		/* For the list			*/
};

typedef struct sw	SWITCH;

#define	sw_type				sw_attributes.type
#define	sw_num_ports		sw_attributes.num_ports
#define	sw_is_smart			sw_attributes.is_smart
#define	sw_has_default		sw_attributes.has_default
#define	sw_bits_shifted		sw_attributes.bits_shifted
#define sw_prompts			sw_attributes.prompts

#define FOR_EACH_SW_PORT(sw, port)	\
	FOR_EACH_PORT((sw)->sw_ports, port)

#endif /* _SWITCH_H */
