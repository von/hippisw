/*
 *	version.c
 */

static char *version = "Beta 0.2";
static char *date = "$Date: 1995/03/27 16:51:11 $";


int
print_version_info()
{
  printf("hippisw release %s dated %s.\n", version, date);
}
