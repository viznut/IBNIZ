#include <stdint.h>

#ifndef IBNIZ_H
#define IBNIZ_H

#ifdef IBNIZ_MAIN
#  define GLOBAL
#else
#  define GLOBAL extern
#endif

#include "vm.h"

/* i/o stuff used by vm */

uint32_t gettimevalue();
int getuserchar();
void waitfortimechange();

/* vm-specific */

void vm_compile(char*src);
void vm_init();
int vm_run();
void switchmediacontext();

/* compiler */

void compiler_parse(char*src);
int compiler_compile();

/* clipboard */

GLOBAL char*clipboard;
int clipboard_load();
void clipboard_store();

#endif
