/*
 *	switchlist.c
 *
 *	These routines maintain one list of switches internally.
 *
 */

#include "basic_defines.h"
#include "switchlist.h"

#include <malloc.h>


static SWITCH	*switchlist = NULL;
static int		number_switches = 0;

/*
 *	Add a switch to the list.
 *
 *	Returns number of switches in list or ERROR on error.
 */
int
add_switch(sw)
	SWITCH		*sw;
{
	number_switches++;

	sw->sw_next = NULL;

	if (switchlist == NULL)
		switchlist = sw;
	else {
		SWITCH      *sw_entry = switchlist;

		/*	Find end of list
		 */
		while (sw_entry->sw_next != NULL)
			sw_entry = sw_entry->sw_next;

		sw_entry->sw_next = sw;
	}
  
#ifdef DEBUG_SWITCHLIST
	fprintf(stderr, "Added %s as switch #%d.\n", sw->sw_name, number_switches);
#endif

	return number_switches;
}

/*
 *	Return the number of switches in the switch list.
 */
int
number_of_switches()
{
	return number_switches;
}


/*
 *	Free the whole switch list.
 */
void
free_switchlist()
{
	SWITCH	*sw = switchlist, *next;

	while (sw != NULL) {
		next = sw->sw_next;
		free(sw);
		sw = next;
	}
}


/*
 *	These routines are used to walk through the switch list.
 *
 */

SWITCH *
first_switch()
{
	return switchlist;
}

SWITCH *
next_switch(sw)
	SWITCH		*sw;
{
	if (sw == NULL)
		return NULL;

	return sw->sw_next;
}

