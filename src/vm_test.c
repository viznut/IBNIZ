#define IBNIZ_MAIN
#include <stdio.h>
#include <stdlib.h>
#include "ibniz.h"

void waitfortimechange()
{
}

uint32_t gettimevalue()
{
  return 0;
}

int runtest(char*code,uint32_t correct_stacktop)
{
  vm_init();
  vm_compile(code);
  while(!vm.stopped) vm_run();
  if(vm.stack[vm.sp]==correct_stacktop) return 0;
  fprintf(stderr,"unit test failed with code \"%s\"\n",code);
  fprintf(stderr,"stacktop=%x (should have been %x)\n",
    vm.stack[vm.sp],correct_stacktop);
  exit(1);
}

void test_numbers()
{
  runtest("12345T",0x23450001);
  runtest("F.1234T",0xF1234);
  runtest("123456789ABCDT",0xABCD6789);
  runtest("1234.56789AT",0x9A345678);
  runtest("1.15.25+T",0x13A00);
}

int main()
{
  /* TODO: i guess we need a little bit more coverage here */

  test_numbers();  
  runtest("1,1+T",2<<16);
  runtest("6,6*T",36<<16);
  runtest("1,4X3*LT",81<<16);
  runtest("1,2,3,2)T",1<<16);
  runtest("3?5:1;T",5<<16);
  runtest("0?5:1;T",1<<16);
  return 0;
}
