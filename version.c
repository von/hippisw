/*
 *	version.c
 */

static char *version = "Beta 0.2.1";
static char *date = "$Date: 1995/04/06 22:23:58 $";


int
print_version_info()
{
  printf("hippisw release %s dated %s.\n", version, date);
}
