#include <stdio.h>
#include <stdlib.h>
#define IBNIZ_MAIN
#include "ibniz.h"
#include "gen.h"

int main(int argc,char**argv)
{
  int rc;

  if(argc<=1)
  {
    fprintf(stderr,"usage: %s 'ibnizcode'\n",argv[0]);
    return 1;
  }
  compiler_parse(argv[1]);
  rc=compiler_compile();
  printf("// compiler returned with %d\n",rc);
  return 0;
}
