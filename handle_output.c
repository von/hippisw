/*
 *	handle_output.c
 *
 *	Routines that handle output to client and switches.
 */

#include "basic_defines.h"
#include "connections.h"
#include "client_request.h"



/*
 *	Write a string to a switch.
 */
int
write_to_switch(conn, string, len)
	Connection		*conn;
	char			*string;
	int				len;
{
	char		buffer[BUFFERLEN];
	register int	read_pos = 0, write_pos = 0;
  

	while (read_pos < len) {
		/*
		 *	Parse string handling backslash characters and converting
		 *	carriage returns.
		 */
		for ( write_pos = 0;
			  (read_pos < len) && (write_pos < BUFFERLEN); ) {
			char c = string[read_pos];

			switch(c) {
			case '\n':	/* STRIP */
			case '\r':
			case '\0':
				read_pos++;
				break;

			case '\\':
				read_pos++;
				c = string[read_pos++];
				switch(c) {
				case 'n':
					buffer[write_pos++] = '\r';
					buffer[write_pos++] = '\n';
					break;

				default:
					buffer[write_pos++] = c;
					break;
				}
				break;

			default:
				buffer[write_pos++] = string[read_pos++];
			}
		}
    
		buffer[write_pos++] = '\r';
		buffer[write_pos++] = '\n';

		/*
		 *	We dont't actually want to send the NULL.
		 */
		buffer[write_pos] = '\0';

		write(conn->sw_sock, buffer, write_pos);
	}

	return NO_ERROR;
}


/*
 *	Send a CR to a switch
 */
int
ping_switch(conn)
	Connection			*conn;
{
    return write_to_switch(conn, "\n", 1);
}


/*
 *	Write a string to the client.
 */
int
write_to_client(conn, string, len)
	Connection		*conn;
	char			*string;
	int				len;
{
    DAEMON_PACKET		packet;

    packet.flags = 0;

    if (conn->got_prompt)
		packet.flags |= DAEMON_PKT_GOT_PROMPT;

    strcpy(packet.string, string);

    return write(conn->client_sock, &packet, sizeof(packet));
}


