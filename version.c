/*
 *	version.c
 */

static char *version = "0.2.3";
static char *date = "$Date: 1996/03/05 20:32:43 $";


int
print_version_info()
{
  printf("hippisw release %s dated %s.\n", version, date);
}
