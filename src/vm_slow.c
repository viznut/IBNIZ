#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "ibniz.h"

#define MOVESP(steps) vm.sp=(vm.sp+(steps))&vm.stackmask
#define MOVERSP(steps) vm.rsp=(vm.rsp+(steps))&vm.rstackmask

void pmv_setfunc();

uint32_t getdatabits(int n)
{
  int s=(32-n-(vm.dataptr&31));
  uint32_t mask;
  uint32_t a;
  if(n<=0 || vm.datalgt<=0) return 0;
  mask=(1<<n)-1;
  if(s>=0) a=(vm.parsed_data[vm.dataptr>>5]>>s)&mask;
      else a=((vm.parsed_data[vm.dataptr>>5]<<(0-s))|
              (vm.parsed_data[(vm.dataptr>>5)+1]>>(32+s)))&mask;
  vm.dataptr=(vm.dataptr+n)%vm.datalgt;
  return a;
}

void initstatecounters()
{
  vm.spchange[0]=vm.spchange[1]=0;
  vm.wcount[0]=vm.wcount[1]=0;
  vm.currentwcount[0]=vm.currentwcount[1]=0;
  vm.prevsp[0]=vm.prevsp[1]=0;
  vm.prevstackval[0]=vm.prevstackval[1]=0;
//  vm.specialcontextstep=1;
}

void vm_compile(char*src)
{
  /* no other compilation in vm_slow! */
  compiler_parse(src);
  vm.specialcontextstep=1;
//  initstatecounters();
}

void vm_init()
{
  /* video context */

  vm.stack=vm.mem+0xE0000;
  vm.stackmask=0x1ffff;
  vm.sp=0;
  
  vm.rstack=vm.mem+0xCC000;
  vm.rstackmask=0x3FFF;
  vm.rsp=0;

  /* audio context */

  vm.costack=vm.mem+0xD0000;
  vm.costackmask=0xffff;
  vm.cosp=1; // to avoid audio skipping bug at start
  
  vm.corstack=vm.mem+0xC8000;
  vm.corstackmask=0x3FFF;
  vm.corsp=0;

  /* state */

  vm.ip=vm.parsed_code;
  vm.mediacontext=0;
  vm.videomode=0;
  vm.audiomode=0;
  vm.visiblepage=1;
  vm.dataptr=0;
  vm.userinput=0;
  vm.stopped=0;
  vm.audiotime=vm.videotime=gettimevalue();
  
  initstatecounters();

  /* zero out memory */
  if(!vm.datalgt) memset(vm.mem,0,MEMSIZE*sizeof(uint32_t));
  else
  {
    int i;
    vm.dataptr=0;
    for(i=0;i<MEMSIZE;i++) vm.mem[i]=getdatabits(32);
    vm.dataptr=0;
  }
  
  pmv_setfunc();
  vm.pmv_func();
}

void switchmediacontext()
{
  SWAP(int32_t*,vm.stack,vm.costack);
  SWAP(uint32_t,vm.sp,vm.cosp);
  SWAP(uint32_t,vm.stackmask,vm.costackmask);
  SWAP(uint32_t*,vm.rstack,vm.corstack);
  SWAP(uint32_t,vm.rsp,vm.corsp);
  SWAP(uint32_t,vm.rstackmask,vm.corstackmask);
  vm.mediacontext=vm.preferredmediacontext;
  pmv_setfunc();
}

void stepmediacontext(int skippoint,int op)
{
  int16_t spc0=vm.spchange[vm.mediacontext];
  int16_t spc1=vm.sp-vm.prevsp[vm.mediacontext];
  if(abs(spc1)<1024)
  {
    vm.spchange[vm.mediacontext]=spc1;
    if(spc0==spc1)
    {
      vm.wcount[vm.mediacontext]=vm.currentwcount[vm.mediacontext];
    } else vm.wcount[vm.mediacontext]=0;
  }
  vm.currentwcount[vm.mediacontext]=0;
  vm.prevsp[vm.mediacontext]=vm.sp;
  vm.prevstackval[vm.mediacontext]=vm.stack[vm.sp];

  if(vm.mediacontext==vm.preferredmediacontext)
  {
    vm.ip=vm.parsed_code+skippoint;
    if(vm.wcount[vm.mediacontext]>0)
       vm.specialcontextstep&=~(1<<vm.mediacontext);
  } else
  {
    switchmediacontext();
    if(!op) vm.ip=vm.parsed_code;
  }
}

void flipvideopage()
{
  vm.visiblepage=((vm.sp>>16)&1)^1;
  for(;;)
  {
    uint32_t newt=gettimevalue();
    if(newt!=vm.videotime) break;
    waitfortimechange();
  }
  vm.videotime=gettimevalue();
}

void pmv_audio()
{
  vm.currentwcount[vm.mediacontext]++;
  if(!vm.sp) // todo we need something better
    vm.audiotime+=64;
  MOVESP(1);
  vm.stack[vm.sp]=vm.audiotime*65536+vm.sp*64;
}

void pmv_video_t()
{
  int p=vm.sp&65535;
  vm.currentwcount[vm.mediacontext]++;
  if(!p)
    flipvideopage();
  MOVESP(1);
  vm.stack[vm.sp]=(vm.videotime<<16)|p;
}

void pmv_video_txy()
{
  int p=vm.sp&65535;
  vm.currentwcount[vm.mediacontext]++;
  if(vm.visiblepage==(vm.sp>>16))
    flipvideopage();
  MOVESP(1);
  vm.stack[vm.sp]=vm.videotime<<16;
  MOVESP(1);
  vm.stack[vm.sp]=(p<<1)-0x10000;
  MOVESP(1);
  vm.stack[vm.sp]=((p&255)<<9)-0x10000;
}

void pmv_setfunc()
{
  if(vm.mediacontext==1)
    vm.pmv_func=pmv_audio;
  else if(vm.videomode==0)
    vm.pmv_func=pmv_video_txy;
  else
    vm.pmv_func=pmv_video_t;
}

void pushmediavariables()
{
  if(vm.mediacontext==0)
  {
    int p=vm.sp&65535;
    
    if(vm.videomode==0)
    {
      if(vm.visiblepage==(vm.sp>>16))
      {
        flipvideopage();
      }
      MOVESP(1);
      vm.stack[vm.sp]=vm.videotime<<16;
      MOVESP(1);
      vm.stack[vm.sp]=(p<<1)-0x10000;
      MOVESP(1);
      vm.stack[vm.sp]=((p&255)<<9)-0x10000;
    } else {
      if(!p)
      {
        flipvideopage();
      }
      MOVESP(1);
      vm.stack[vm.sp]=(vm.videotime<<16)|p;
    }
  } else
  {
    if(!vm.sp) // todo we need something better
    {
      vm.audiotime+=64;
    }
    MOVESP(1);
    vm.stack[vm.sp]=vm.audiotime*65536+vm.sp*64;
//    fprintf(stderr,"%x\n",vm.stack[vm.sp]);
  }
}

#define CYCLESPERRUN 10223
int vm_run()
{
  int cycles;
  if(vm.stopped) return 0;

  if(vm.mediacontext!=vm.preferredmediacontext)
     vm.specialcontextstep=3;

  pmv_setfunc();

  for(cycles=CYCLESPERRUN;cycles;cycles--)
  {
    char op=*vm.ip++;
    int32_t*a=&vm.stack[vm.sp],*b;

    switch(op)
    {
      /*** NUMBERS ***/

      case(OP_LOADIMM):
        MOVESP(1);
        vm.stack[vm.sp]=vm.parsed_hints[vm.ip-1-vm.parsed_code];
        break;
 
      /*** ARITHMETIC ***/
      
      case('+'):	// (b a -- a+b)
        MOVESP(-1);
        vm.stack[vm.sp]+=*a;
        break;

      case('-'):	// (b a -- a-b)
        MOVESP(-1);
        vm.stack[vm.sp]-=*a;
        break;

      case('*'):	// (b a -- a*b)
        MOVESP(-1);
        vm.stack[vm.sp]=IBNIZ_MUL(vm.stack[vm.sp],*a);
        /*
        b=&vm.stack[vm.sp];
        {int64_t m=*a;
         m*=((int32_t)*b);
         *b=m>>16;
         }
        */
        break;

      case('/'):	// (b a -- a/b)
        MOVESP(-1);
        vm.stack[vm.sp]=IBNIZ_DIV(vm.stack[vm.sp],*a);
        /*
        b=&vm.stack[vm.sp];
        if(!*a)*b=0;
        else
        {int64_t m=*b;
         m<<=16;
         m/=((int32_t)*a);
         *b=m;}
        */
        break;

      case('%'):	// (b a -- a%b)
        MOVESP(-1);
        vm.stack[vm.sp]=IBNIZ_MOD(vm.stack[vm.sp],*a);
        /*
        b=&vm.stack[vm.sp];
        if(!*a)*b=0;
        else
        *b=(*b%*a);
        */
        break;
    
      case('q'):	// (a -- sqrt(a), 0 if a<0)
        *a=IBNIZ_SQRT(*a);
        /*
        if(*a<0) *a=0;
        else *a=sqrt(*a/65536.0)*65536.0;
        */
        break;

      case('&'):	// (b a -- a&b)
        MOVESP(-1);
        vm.stack[vm.sp]&=*a;
        break;

      case('|'):	// (b a -- a|b)
        MOVESP(-1);
        vm.stack[vm.sp]|=*a;
        break;

      case('^'):	// (b a -- a^b)
        MOVESP(-1);
        vm.stack[vm.sp]^=*a;
        break;

      case('r'):	// (b a -- b ror a)
        MOVESP(-1);
        vm.stack[vm.sp]=IBNIZ_ROR(vm.stack[vm.sp],*a);
        /*
        b=&vm.stack[vm.sp];
        {int steps=(*a>>16)&31;
         *b=ROR(*b,steps);
        }
        */
        break;

      case('l'):	// (b a -- b >> a)
        MOVESP(-1);
        vm.stack[vm.sp]=IBNIZ_SHL(vm.stack[vm.sp],*a);
        /*
        b=&vm.stack[vm.sp];
        {int steps=(*a>>16)&63;
         uint32_t w=*b;
         if(steps<32)
         *b=(w<<steps); else *b=(w>>(steps-32));
         }
        */
        break;

      case('~'):	// (a -- NOT a)
        *a=~*a;
        break;

      case('s'):	// (a -- sin(a))
        *a=IBNIZ_SIN(*a);
        //*a=sin(*a*(2*M_PI/65536.0))*65536.0;
        break;
      case('a'):	// (b a -- atan2(a,b))
        MOVESP(-1);
        vm.stack[vm.sp]=IBNIZ_ATAN2(vm.stack[vm.sp],*a);
        //b=&vm.stack[vm.sp];
        //*b=atan2(*a,*b)*(65536.0/(2*M_PI));
        break;

      case('<'):	// (a -- a<0?a:0)
        *a=IBNIZ_ISNEG(*a);
        //if(*a>=0)*a=0;
        break;
      case('>'):	// (a -- a>0?a:0)
        *a=IBNIZ_ISPOS(*a);
        //if(*a&0x80000000)*a=0;
        break;
      case('='):	// (a -- a==0?1:0)
        *a=IBNIZ_ISZERO(*a);
        //if(*a)*a=0x10000;else *a=0;
        break;

      /*** STACK MANIPULATION ***/

      case('d'):	// (a -- a a)
        MOVESP(1);
        vm.stack[vm.sp]=*a;
        break;

      case('p'):	// (a --)
        MOVESP(-1);
        break;

      case('x'):	// (b a -- a b) // forth: SWAP
        {int32_t tmp=*a;
         b=&vm.stack[(vm.sp-1)&vm.stackmask];
         *a=*b;
         *b=tmp;}
        break;

      case('v'):	// (c b a -- b a c) // forth: ROT
        {int32_t a_v=*a,*c;
         b=&vm.stack[(vm.sp-1)&vm.stackmask];
         c=&vm.stack[(vm.sp-2)&vm.stackmask];
         *a=*c;
         *c=*b;
         *b=a_v;}
        break;

      case(')'):	// pick from STACK[top-1-i]
        *a=vm.stack[(vm.sp-1-ROL(*a,16))&vm.stackmask];
        break;
      
      case('('):	// store to STACK[top-2-i]
        MOVESP(-1);
        b=&vm.stack[vm.sp];
        MOVESP(-1);
        vm.stack[(vm.sp-ROL(*a,16))&vm.stackmask]=*b;
        break;

      case('z'):
        MOVESP(1);
        vm.stack[vm.sp]=ROL(((vm.stack+vm.sp)-vm.mem),16);
        break;

      /*** EXTERIOR LOOP ***/

      case('M'):	// media switch
      case('\0'):
        if(vm.specialcontextstep&(1<<vm.mediacontext))
           stepmediacontext(vm.parsed_hints[vm.ip-vm.parsed_code-1],op);
        else
           vm.ip=vm.parsed_code+vm.parsed_hints[vm.ip-vm.parsed_code-1];
        vm.pmv_func(); //pushmediavariables();
        break;

      /*
      case('\0'):	// end of code
        //vm.ip=vm.parsed_code; // or top of rstack (don't pop it)
        if(vm.stepmediacontext_func)
           vm.stepmediacontext_func(vm.parsed_hints[vm.ip-vm.parsed_code-1],1);
        else
           vm.ip=vm.parsed_code+vm.parsed_hints[vm.ip-vm.parsed_code-1];
        //stepmediacontext(vm.parsed_hints[vm.ip-vm.parsed_code-1],1);
        pushmediavariables();
        break;
      */

      case('w'):	// whereami
        vm.pmv_func(); //pushmediavariables();
        break;

      case('T'):	// terminate program
        vm.ip--;
        vm.stopped=1;
        return CYCLESPERRUN-cycles;

      /*** MEMORY MANIPULATION ***/

      case('@'):	// (addr -- val)
        *a=vm.mem[ROL(*a,16)&(MEMSIZE-1)];
        break;
      
      case('!'):	// (val addr --)
        MOVESP(-1);
        b=&vm.stack[vm.sp];
        MOVESP(-1);
        vm.mem[ROL(*a,16)&(MEMSIZE-1)]=*b;
        break;

      /*** PROGRAM CONTROL: Conditional execution ***/

      case('?'):	// if
        MOVESP(-1);
        if(*a!=0) break;
      case(':'):	// then
        vm.ip=vm.parsed_code+vm.parsed_hints[vm.ip-vm.parsed_code-1];
      case(';'):	// endif/nop
        break;

      /*** PROGRAM CONTROL: Loops ***/

      case('i'):	// i counter
        MOVESP(1);
        vm.stack[vm.sp]=ROL(vm.rstack[(vm.rsp-1)&vm.rstackmask],16);
        break;

      case('j'):	// j counter
        MOVESP(1);
        vm.stack[vm.sp]=ROL(vm.rstack[(vm.rsp-3)&vm.rstackmask],16);
        break;
      
      case('X'):	// times
        MOVERSP(1);
        MOVESP(-1);
        vm.rstack[vm.rsp]=ROL(*a,16);
      case('['):	// do
        MOVERSP(1);
        vm.rstack[vm.rsp]=vm.ip-vm.parsed_code;
        break;

      case('L'):	// loop
        {uint32_t*i=&vm.rstack[(vm.rsp-1)&vm.rstackmask];
        (*i)--;
        if(*i==0) MOVERSP(-2); else
          vm.ip=(vm.rstack[vm.rsp]%vm.codelgt)+vm.parsed_code;
        }
        break;

      case(']'):	// while
        MOVESP(-1);
        if(*a) vm.ip=(vm.rstack[vm.rsp]%vm.codelgt)+vm.parsed_code;
          else MOVERSP(-1);
        break;

      case('J'):	// jump
        {int point=*a%vm.codelgt; // !!! addressing will change
        MOVESP(-1);
        vm.ip=vm.parsed_code+point;}
        break;
        
      /*** PROGRAM CONTROL: Subroutines ***/

      case('{'):	// defsub
        MOVESP(-1);
        vm.mem[ROL(*a,16)&(MEMSIZE-1)]=vm.ip-vm.parsed_code;
        vm.ip=vm.parsed_code+vm.parsed_hints[vm.ip-1-vm.parsed_code];
        break;
      case('}'):	// ret
        vm.ip=vm.parsed_code+(vm.rstack[vm.rsp]%vm.codelgt);
        MOVERSP(-1);
        break;
      case('V'):	// visit
        MOVESP(-1);
        MOVERSP(1);
        vm.rstack[vm.rsp]=vm.ip-vm.parsed_code;
        vm.ip=((vm.mem[ROL(*a,16)&(MEMSIZE-1)])%vm.codelgt)+vm.parsed_code;
        break;

      /*** PROGRAM CONTROL: Rstack manipulation ***/
      
      case('R'):	// pull from rstack to mainstack
        MOVESP(1);
        vm.stack[vm.sp]=ROL(vm.rstack[vm.rsp],16);
        MOVERSP(-1);
        break;
      case('P'):	// push from stack to rstack
        MOVERSP(1);
        vm.rstack[vm.rsp]=ROL(*a,16);
        MOVESP(-1);
        break;

      /*** INPUT ***/

      case('U'):	// userinput
        MOVESP(1);
        vm.stack[vm.sp]=vm.userinput;
        vm.userinput&=0xff00ffff;
        break;

      /*** DATA SEGMENT ***/
      
      case('G'):	// getbits
        *a=ROL(getdatabits((*a>>16)&31),16);
        break;
    }
  }
  return CYCLESPERRUN;
}
