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


	switch(sw->sw_type) {

	case HIPPISW_ES1:
		log("Sending init string to %s.\n", sw->sw_name);		
		sprintf(buffer, "set parameter page 0\n");
		if (write_to_switch(conn, buffer, strlen(buffer)) == ERROR)
			log("Error writing to %s.\n", sw->sw_name);
		break;
	}
}
