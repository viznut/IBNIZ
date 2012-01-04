#ifndef VM_H

#define MEMSIZE 0x100000

GLOBAL struct
{
    char  *ip;
   int32_t*stack;
  uint32_t sp;
  uint32_t stackmask;

  uint32_t*rstack;
  uint32_t rsp;
  uint32_t rstackmask;

   int32_t*costack;
  uint32_t cosp;
  uint32_t costackmask;
  
  uint32_t*corstack;
  uint32_t corsp;
  uint32_t corstackmask;

  char     mediacontext; /* 0=video, 1=audio, 2=stdio */
  char     videomode;    /* 0=txy, 1=t */
  char     audiomode;    /* 0=mono, 1=stereo */
  char     preferredmediacontext;
  char     visiblepage;
  char     stopped;
  uint32_t videotime;
  uint32_t audiotime;
  
  uint32_t prevsp[2];
  uint32_t prevstackval[2];
  int      currentwcount[2];

  int16_t  spchange[2];
  int      wcount[2];

  int      codelgt;
  int      datalgt;
  int      dataptr;
  uint32_t userinput;

  int32_t  mem[MEMSIZE];
} vm;

#endif