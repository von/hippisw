/*
 *	sw_output
 *
 *	Routines that create a command for a given type of switch.
 *
 *	All routines a pointer to a static string containing the command.
 *	The string may be zero-length if the command is not supported by
 *	the switch.
 *
 */

#include "basic_defines.h"
#include "sw_output.h"
#include "switch.h"
#include "path.h"
#include "port.h"
#include "logical_addr.h"

#include <stdio.h>

/*
 *	Set by set_output_switch_type() to the switch the commands are
 *	are being creared for.
 */

static struct sw *output_sw = NULL;


/*
 *	Set by set_output_swtich_stream(). If non-NULL all commands are
 *	fprintf()-ed to this stream.
 */

static FILE *sw_stream = NULL;

#define PRINT_CMD()							\
	if ((*command_str != '\0') && (sw_stream != NULL)) { 		\
		  fprintf(sw_stream, command_str);			\
	 }


/*
 *	Static buffer used to return command.
 */

#define MAX_SW_COMMAND_LENGTH	256

static char command_str[MAX_SW_COMMAND_LENGTH];


/*
 * Handy defines
 */

#define SW_TYPE		(output_sw->sw_type)

#define SMS_VERSION	(output_sw->sw_version)		/* For NSC switches */


/*
 *	XXX KLUDGE - no proto for sprinfcat due to the fact it has a
 *		variable number of arguments.
 */
static char *sprintfcat();



/*
 * Comment control
 */
static Boolean do_comments = TRUE;



/*
 * Set the type of switch commands should be created for.
 *
 * Returns NULL if the switch type is not supported.
 *
 */
char *
set_output_switch(sw)
	struct sw *sw;
{
	char *return_string = command_str;
  
  
	switch(sw->sw_type) {
    
	case HIPPISW_PS4:
	case HIPPISW_PS8:
	case HIPPISW_PS32:
		switch (sw->sw_version) {

		case 1:
		case 2:
			break;

		default:
			fprintf(stderr, "%s: Unsupported SMS version %d.\n",
					sw->sw_name, sw->sw_version);
			return_string = NULL;
		}
		break;
    
	case HIPPISW_IOSC4:
	case HIPPISW_IOSC8:
	case HIPPISW_NETSTAR:
	case HIPPISW_ES1:
		if (sw->sw_version != 1) {
			fprintf(stderr, "%s: Unsupported version %d.\n",
					sw->sw_name, sw->sw_version);
			return_string = NULL;
		}
		break;

	case HIPPISW_P8:
		fprintf(stderr,
				"%s: P8 switch doesn't support commands\n",
				sw->sw_name);
		return_string = NULL;
		break;
    
	default:
		fprintf(stderr, "%s: Unknown switch type (%d)\n",
				sw->sw_name, sw->sw_type);
		return_string = NULL;
		break;
	}
  
	if (return_string != NULL) {
		output_sw = sw;
		sprintf(command_str, "switch %s\n", output_sw->sw_name);
	} else {
		NULL_STRING(command_str);
	}
  
	PRINT_CMD();
  
	return return_string;
}


/*
 *	Set the output stream for the commands.
 *
 *	Returns the previous stream.
 */
FILE *
set_switch_output_stream(stream)
	FILE *stream;
{
	FILE *old_stream = sw_stream;

	sw_stream = stream;

	return old_stream;
}


/*
 *	Turn comments off
 */
void
sw_no_comments()
{
	do_comments = FALSE;
}


/*
 * Comment
 *
 */
char *
sw_comment(comment, arg1, arg2, arg3, arg4, arg5, arg6)
	char *comment;
	char *arg1, *arg2, *arg3, *arg4, *arg5, *arg6;
{
	char tmp_buffer[MAX_SW_COMMAND_LENGTH];


	if (do_comments == FALSE) 
		NULL_STRING(command_str);

	else {
		sprintf(tmp_buffer,
				comment, arg1, arg2, arg3, arg4, arg5, arg6);
		sprintf(command_str, "# %s", tmp_buffer);
	}

	PRINT_CMD();

	return command_str;
}


/*
 * Disable all ports on current switch that need disabling.
 */
char *
disable_all_ports()
{
	PORT		*port;
	int			portnum;


	NULL_STRING(command_str);

	switch(SW_TYPE) {
    
	case HIPPISW_PS8:
	case HIPPISW_PS4:
	case HIPPISW_PS32:
    
    
		FOR_EACH_PORT(output_sw->sw_ports, port, portnum) {

			if (port->swp_need_disabled == FALSE)
				continue;

			switch(SMS_VERSION) {
			case 1:
				sprintfcat(command_str, "disable port %d\n", port->swp_num);
				break;

			case 2:
				sprintf(command_str, "fset dpi\\n%d\n", port->swp_num);
				break;
			}
		}	
		break;

	default:
		/** Not necessary **/
		NULL_STRING(command_str);
		break;
	}

	PRINT_CMD();

	return command_str;
}


/*
 * Enable all Ports
 */
char *
enable_all_ports()
{
	switch(SW_TYPE) {
	
	case HIPPISW_PS8:
	case HIPPISW_PS4:
	case HIPPISW_PS32:
		switch(SMS_VERSION) {
		case 1:
			sprintf(command_str, "enable port all\n");
			break;

		case 2:
			sprintf(command_str, "fset epa\n");
			break;
		}
		break;

	default:
		/** Not necessary **/
		NULL_STRING(command_str);
		break;
	}

	PRINT_CMD();

	return command_str;
}


/*
 * Clear all pathways/logical addresses
 */
char *
clear_all_pathways()
{
	switch(SW_TYPE) {
	
	case HIPPISW_PS8:
	case HIPPISW_PS4:
	case HIPPISW_PS32:
		switch(SMS_VERSION) {
		case 1:
			sprintf(command_str, "clear pathway all\n");
			break;

		case 2:
			sprintf(command_str, "fset cpa\n");
			break;
		}
		break;

	case HIPPISW_NETSTAR:
		sprintf(command_str, "ifielde all = clear\n");
		break;

	case HIPPISW_IOSC4:
	case HIPPISW_IOSC8:
		sprintf(command_str, "clear path all\n");
		break;

	case HIPPISW_ES1:
		sprintf(command_str, "set destination all invalid\n");
		break;
	}

	PRINT_CMD();

	return command_str;
}


/*
 * Save all pathways/logical addresses
 */
char *
save_all_pathways()
{
	switch(SW_TYPE) {
	
	case HIPPISW_PS8:
	case HIPPISW_PS4:
	case HIPPISW_PS32:
		switch(SMS_VERSION) {
		case 1:
			sprintf(command_str, "save pathway all\n");
			break;

		case 2:
			sprintf(command_str, "fset sapa\n");
			break;
		}
		break;

	case HIPPISW_NETSTAR:
		sprintf(command_str, "ifieldw both\n");
		break;

	case HIPPISW_IOSC4:
	case HIPPISW_IOSC8:
		sprintf(command_str, "save path all\n");
		break;

	case HIPPISW_ES1:
		sprintf(command_str, "save destination\n");
		break;
	}

	PRINT_CMD();

	return command_str;
}


/*
 * Set a pathway from all input ports
 */
char *
set_pathway_all(address, portlist)
	Logaddr		address;
	PATH		*portlist;
{
	PORT		*port;
	int			portnum;
  
  
	switch(SW_TYPE) {
    
	case HIPPISW_PS8:
	case HIPPISW_PS4:
	case HIPPISW_PS32:
		switch(SMS_VERSION) {
		case 1:
			sprintf(command_str,
					"set pathway all  d %3.3x",
					address, port);
      
			FOR_EACH_PORT_IN_PATH(portlist, port, portnum)
				sprintfcat(command_str, " o %2d", port->swp_num);
      
			sprintfcat(command_str, "\n");

			break;

		case 2:
			sprintf(command_str,
					"fset spa %3.3x", address);

			FOR_EACH_PORT_IN_PATH(portlist, port, portnum)
				sprintfcat(command_str, " %2d", port->swp_num);

			sprintfcat(command_str, "\n");
			break;
		}
		break;
    
	case HIPPISW_NETSTAR:
		sprintf(command_str, "ifielde all %4d =", address);
    
		FOR_EACH_PORT_IN_PATH(portlist, port, portnum)
			sprintfcat(command_str, " %2d", port->swp_num);
    
		sprintfcat(command_str, "\n");
		break;
    
	case HIPPISW_IOSC4:
	case HIPPISW_IOSC8:
		sprintf(command_str,
				"set path all d %3.3x", address);
    
		FOR_EACH_PORT_IN_PATH(portlist, port, portnum)
			sprintfcat(command_str, " o %2d", port->swp_num);
    
		sprintfcat(command_str, "\n");
		break;
    
	case HIPPISW_ES1:
		sprintf(command_str, "set destination %#3x to %2d allow\n",
				address, PRIMARY_PORT(portlist, address)->swp_num);
		break;
	}
  
	PRINT_CMD();
  
	return command_str;
}


/*
 * Set a pathway from a particular input port
 */
char *
set_pathway(in_port, address, portlist)
	int			in_port;
	Logaddr		address;
	PATH		*portlist;
{
	PORT		*port;
	int			portnum;
  
  
	switch(SW_TYPE) {
    
	case HIPPISW_PS8:
	case HIPPISW_PS4:
	case HIPPISW_PS32:
		switch(SMS_VERSION) {
		case 1:
			sprintf(command_str,
					"set pathway i %2d d %3.3x",
					in_port, address);

			FOR_EACH_PORT_IN_PATH(portlist, port, portnum)
				sprintfcat(command_str, " o %2d", port->swp_num);

			sprintfcat(command_str, "\n");

			break;

		case 2:
			sprintf(command_str, "fset spd %2d %3.3x",
					in_port, address);

			FOR_EACH_PORT_IN_PATH(portlist, port, portnum)
				sprintfcat(command_str, " %2d", port->swp_num);

			sprintfcat(command_str, "\n");

			break;
		}
		break;

	case HIPPISW_NETSTAR:
		sprintf(command_str, "ifielde %3d %4d =",
				in_port, address);
    
		FOR_EACH_PORT_IN_PATH(portlist, port, portnum)
			sprintfcat(command_str, " %2d", port->swp_num);
    
		sprintfcat(command_str, "\n");
		break;
    
	case HIPPISW_IOSC4:
	case HIPPISW_IOSC8:
		sprintf(command_str, "set path i %2d d %3.3x",
				in_port, address);
    
		FOR_EACH_PORT_IN_PATH(portlist, port, portnum)
			sprintfcat(command_str, " o %2d", port->swp_num);
    
		sprintfcat(command_str, "\n");
    
		break;
    
	case HIPPISW_ES1:
		/* Not supported ?!? */
		NULL_STRING(command_str);
		break;
	}
  
	PRINT_CMD();
  
	return command_str;
}


/*
 * Display the status of a port
 */
char *
display_port_stats(port)
	int port;
{
	switch(SW_TYPE) {
	case HIPPISW_PS8:
	case HIPPISW_PS4:
	case HIPPISW_PS32:
		switch(SMS_VERSION) {
		case 1:
			sprintf(command_str, "d s i %2d\n", port);
			break;

		case 2:
			sprintf(command_str, "fd si \\n%d\n", port);
			break;
		}
		break;

	case HIPPISW_NETSTAR:
		sprintf(command_str, "counters %d\n", port);
		break;

	case HIPPISW_IOSC4:
	case HIPPISW_IOSC8:
		/* Not supported?!? */
		NULL_STRING(command_str);
		break;

	case HIPPISW_ES1:
		sprintf(command_str, "show counters %2d\n", port);
		break;
	}

	PRINT_CMD();

	return command_str;
}


/*
 * Display all stats on a switch
 */
char *
display_switch_stats()
{
	switch(SW_TYPE) {
	case HIPPISW_PS8:
	case HIPPISW_PS4:
	case HIPPISW_PS32:
		switch(SMS_VERSION) {
		case 1:
			sprintf(command_str, "d s a\n");
			break;

		case 2:
			sprintf(command_str, "fd sa \n");
			break;
		}
		break;


	case HIPPISW_NETSTAR:
		sprintf(command_str, "counters all\n");
		break;

	case HIPPISW_IOSC4:
	case HIPPISW_IOSC8:
		/* Not supported?!? */
		NULL_STRING(command_str);
		break;

	case HIPPISW_ES1:
		sprintf(command_str, "show counters all\n");
		break;

	}

	PRINT_CMD();

	return command_str;
}


/*
 * Clear the status of a port
 */
char *
clear_port_stats(port)
	int port;
{
	switch(SW_TYPE) {
	case HIPPISW_PS8:
	case HIPPISW_PS4:
	case HIPPISW_PS32:
		switch(SMS_VERSION) {
		case 1:
			sprintf(command_str, "c s i %2d\n", port);
			break;

		case 2:
			sprintf(command_str, "fs csi %d\n", port);
			break;
		}
		break;

	case HIPPISW_NETSTAR:
		/* Anyway to clear without displaying? */
		sprintf(command_str, "counters %d r\n", port);
		break;

	case HIPPISW_ES1:
		sprintf(command_str, "set counters 0 %d\n", port);
		break;
    
	case HIPPISW_IOSC4:
	case HIPPISW_IOSC8:
		/* Not supported */
		NULL_STRING(command_str);
		break;

	}
  
	PRINT_CMD();
  
	return command_str;
}


/*
 * Display all stats on a switch
 */
char *
clear_switch_stats()
{
	switch(SW_TYPE) {
	case HIPPISW_PS8:
	case HIPPISW_PS4:
	case HIPPISW_PS32:
		switch(SMS_VERSION) {
		case 1:
			sprintf(command_str, "c s a\n");
			break;

		case 2:
			sprintf(command_str, "fs csa \n");
			break;
		}
		break;

	case HIPPISW_NETSTAR:
		sprintf(command_str, "counters all r\n");
		break;

	case HIPPISW_ES1:
		sprintf(command_str, "set counters 0 all\n");
		break;
    
	case HIPPISW_IOSC4:
	case HIPPISW_IOSC8:
		/* Not supported */
		NULL_STRING(command_str);
		break;

	}

	PRINT_CMD();

	return command_str;
}


/*
 * Concatenate stuff onto the end of a string
 */
static char *
sprintfcat(string, format, arg)
	char *string;
	char *format;
	char *arg;
{
	char buffer[BUFFERLEN];

	sprintf(buffer, format, arg);

	strcat(string, buffer);

	return string;
}







