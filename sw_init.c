/*
 *	sw_init
 *
 *	Routine called when first connected to a switch to initialize it
 *	to a proper state so that the daemon can talk to it smoothly.
 */

#include "basic_defines.h"
#include "sw_init.h"
#include "connections.h"
#include "logger.h"

void
sw_init(conn)
	Connection	*conn;
{
	SWITCH		*sw = conn->sw;
	char		buffer[BUFFERLEN];


	log("Sending init string to %s.\n", sw->sw_name);		

	switch(sw->sw_type) {

	case HIPPISW_ES1:
		sprintf(buffer, "set parameter page 0\n");
		if (write_to_switch(conn, buffer, strlen(buffer)) == ERROR)
			log("Error writing to %s.\n", sw->sw_name);
		break;

	default:
		/*
		 * No init string to send, but we need to write an empty
		 * string anyways just to solicit another prompt.
		 */
		sprintf(buffer, "\n");
		if (write_to_switch(conn, buffer, strlen(buffer)) == ERROR)
			log("Error writing to %s.\n", sw->sw_name);
		break;

	}
}
