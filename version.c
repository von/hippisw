/*
 *	version.c
 */

#include "basic_defines.h"

static char *date = "$Date: 1996/07/02 18:52:18 $";


int
print_version_info()
{
  printf("hippisw release %s dated %s.\n", HIPPISW_VERSION, date);
}
