/*
 *	switch.c
 *
 *	Functions dealing with the switch structure.
 */

#include "basic_defines.h"
#include "switch.h"


/*
 *	Given a switch type return it's attributes
 */

static struct switch_attributes attributes[] = {
  /* Type, # ports, Smart, has default, bits shifted	*/
  { HIPPISW_P8, 8, FALSE, FALSE, 4 },
  { HIPPISW_PS4, 4, TRUE, FALSE, 2 }, 
  { HIPPISW_PS8, 8, TRUE, FALSE, 4 },
  { HIPPISW_PS32, 32, TRUE, FALSE, 5 },
  { HIPPISW_IOSC4, 4, TRUE, TRUE, 2 },
  { HIPPISW_IOSC8, 8, TRUE, TRUE, 3 },
  { HIPPISW_NETSTAR, 16, TRUE, TRUE, 4 },
  { HIPPISW_ES1, 16, TRUE, TRUE, 4 },
  { HIPPISW_VIRT, 32, TRUE, TRUE, 0 }
};

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
     SW_TYPE			type;
     int			version;
{
  char *prompt;

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

  



