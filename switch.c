/*
 *	switch.c
 *
 *	Functions dealing with the switch structure.
 */

#include "basic_defines.h"
#include "switch.h"


/*
 * Prompts for different switch types.
 */
static char *nsc_prompts[] = {
    "System SMS>",		/* For SMS 1.0			*/
    "SMS2 > ",			/* For SMS 2.0			*/
    NULL
};

static char *essential_prompts[] = {
    "ES-1> ",
    "new code: ",		/* For download prompt	*/
    "-MORE-",			/* For page prompt		*/
    NULL
};

static char *iosc_prompts[] = {
    "IOSC>",
    NULL
};

static char *clusterswitch_prompts[] = {
    "OPER: >",
    NULL
};



/*
 * Default attributes for switches
 */
static struct switch_attributes attributes[] = {
	/* Type, # ports, Smart, has default, bits shifted	*/
	{ HIPPISW_P8, 8, FALSE, FALSE, 4, nsc_prompts },
	{ HIPPISW_PS4, 4, TRUE, FALSE, 2, nsc_prompts }, 
	{ HIPPISW_PS8, 8, TRUE, FALSE, 4, nsc_prompts },
	{ HIPPISW_PS32, 32, TRUE, FALSE, 5, nsc_prompts },
	{ HIPPISW_IOSC4, 4, TRUE, TRUE, 2, iosc_prompts },
	{ HIPPISW_IOSC8, 8, TRUE, TRUE, 3, iosc_prompts },
	{ HIPPISW_NETSTAR, 16, TRUE, TRUE, 4, clusterswitch_prompts },
	{ HIPPISW_ES1, 16, TRUE, TRUE, 4, essential_prompts },
	{ HIPPISW_VIRT, 32, TRUE, TRUE, 0, NULL }
};


/*
 *	Given a switch type return it's attributes
 */
struct switch_attributes *
get_sw_attributes(type)
	SW_TYPE			type;
{
	int		index = 0;

	while (attributes[index].type != type)
		index++;

	return &(attributes[index]);
}


/*
 *	Given a switch type and version number, return it's default prompt.
 */
char *
get_sw_prompt(type, version)
	SW_TYPE		type;
	int			version;
{
	char 		*prompt;

	switch(type) {

	case HIPPISW_P8:
	case HIPPISW_PS4:
	case HIPPISW_PS8:
	case HIPPISW_PS32:
		switch(version) {
		case 1:
			prompt = "SMS>";
			break;

		case 2:
			prompt = "SMS2 > ";
			break;
		}
		break;

	case HIPPISW_IOSC4:
	case HIPPISW_IOSC8:
		prompt = "IOSC>";
		break;

	case HIPPISW_NETSTAR:
		prompt = "OPER: > ";
		break;

	case HIPPISW_ES1:
		prompt = "ES-1>";
		break;
	}

	return prompt;
}

  



