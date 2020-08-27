
#include "apmapbin.h"

/*!
* copied from the error.c file of Metis project which follows Apache licence
\author George
\date 1/1/2007
\version\verbatim $Id: error.c 10711 2011-08-31 22:23:04Z karypis $ \endverbatim
*/
void errexit(char *f_str,...)
{
  va_list argp;

  va_start(argp, f_str);
  vfprintf(stderr, f_str, argp);
  va_end(argp);

  if (strlen(f_str) == 0 || f_str[strlen(f_str)-1] != '\n')
        fprintf(stderr,"\n");
  fflush(stderr);
  exit(-2);
}
