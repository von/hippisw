/*
 *	version.c
 */

static char *version = "Beta 0.1";
static char *date = "$Date: 1995/02/28 23:17:28 $";


int
print_version_info()
{
  printf("hippisw release %s dated %s.\n", version, date);
}
