#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "ibniz.h"

#define MAXCODESIZE 4096
#define MAXDATASIZE 4096

#define OP_LOADIMM '0'

#define ROL(a,s) ((((uint32_t)(a))<<(s))|(((uint32_t)(a))>>(32-(s))))
#define ROR(a,s) ((((uint32_t)(a))>>(s))|(((uint32_t)(a))<<(32-(s))))

#define MOVESP(steps) vm.sp=(vm.sp+(steps))&vm.stackmask
#define MOVERSP(steps) vm.rsp=(vm.rsp+(steps))&vm.rstackmask

char compiled_code[MAXCODESIZE];
uint32_t compiled_data[MAXDATASIZE];
uint32_t compiled_hints[MAXCODESIZE];

void pushmediavariables();

uint32_t getdatabits(int n)
{
  int s=(32-n-(vm.dataptr&31));
  uint32_t mask;
  uint32_t a;
  if(n<=0 || vm.datalgt<=0) return 0;
  mask=(1<<n)-1;
  if(s>=0) a=(compiled_data[vm.dataptr>>5]>>s)&mask;
      else a=((compiled_data[vm.dataptr>>5]<<(0-s))|
              (compiled_data[(vm.dataptr>>5)+1]>>(32+s)))&mask;
  vm.dataptr=(vm.dataptr+n)%vm.datalgt;
  return a;
}

void vm_compile(char*src)
{
  char*d=compiled_code;
  uint32_t*hd=compiled_hints;
  uint32_t num;
  char*s,nummode=0,shift=0;
  int i,j;
  s=src;

  /* parse immediates, skip comments & whitespaces */

  for(;;)
  {
    char a=*s++;
    if((!a) || (a>='!' && a<='~'))
    {
      if(a=='.' || (a>='0' && a<='9') || (a>='A' && a<='F'))
      {
        if(nummode==0)
        {
          num=0;
          shift=16;
          nummode=1;
        }
        if(a=='.')
        {
          if(nummode==2)
          {
            *d++=OP_LOADIMM;
            *hd++=num;
            num=0;
          }
          nummode=2;
          shift=12;
        } else
        {
          char digit=(a>='A'?a-'A'+10:a-'0');
          if(nummode==1) num=ROL(num,4);
          num|=digit<<shift;
          if(nummode==2) shift=(shift-4)&31;
        }
      } else
      {
        if(nummode)
        {
          *d++=OP_LOADIMM;
          *hd++=num;
          nummode=0;
        }
        if(a=='\\')
        {
          while(*s && *s!='\n') s++;
          if(!s) break;
          s++;
        }
        else
        {
          if(a!=',')
          {
            if(a=='$') a='\0';
            *d++=a;
            *hd++=0;
            if(a=='\0') break;
          }
        }
      }
    }
  }

  /* parse data */

  vm.datalgt=0;
  if(s[-1]=='$')
  {
    int digitsz=4;
    compiled_data[0]=0;
    for(;;)
    {
      int a=*s++;
      if(!a) break;
      if(a=='\\')
      {
        while(*s && *s!='\n') s++;
        if(!s) break;
        s++;
      }
      else
      switch(a)
      {
        case('b'):
          digitsz=1;
          break;
        case('q'):
          digitsz=2;
          break;
        case('o'):
          digitsz=3;
          break;
        case('h'):
          digitsz=4;
          break;
        case('A'):case('B'):case('C'):case('D'):case('E'):case('F'):
          a=a-'A'+10+'0';
        case('0'):case('1'):case('2'):case('3'):case('4'):
        case('5'):case('6'):case('7'):case('8'):case('9'):
          a-='0';
          a&=((1<<digitsz)-1);
          {int s=(32-digitsz-(vm.datalgt&31));
           if(s>=0)
           {
             compiled_data[vm.datalgt>>5]|=a<<s;
             compiled_data[(vm.datalgt>>5)+1]=0;
           }
           else
           {
             compiled_data[vm.datalgt>>5]|=a>>(0-s);
             compiled_data[(vm.datalgt>>5)+1]=a<<(32+s);
           }
           vm.datalgt+=digitsz;
          }
          break;
      }
    }
    /* fill last 2 words to ease fetch */
    {int pad=vm.datalgt&31;
    if(pad)
    {
      int i=pad;
      while(i<32)
      {
        compiled_data[vm.datalgt>>5]|=compiled_data[0]>>i;
        i*=2;
      }
    }
    if(!pad) compiled_data[(vm.datalgt>>5)+1]=compiled_data[0];
    else
    {
      compiled_data[(vm.datalgt>>5)+1]=
        (compiled_data[0]<<(32-pad)) |
        (compiled_data[1]>>pad);
    }
    }
  }

  /* precalculate skip points */
  vm.codelgt=d-compiled_code;
  for(i=0;;i++)
  {
    int j=i+1,seek0=0,seek1=0,seek2=0;
    char a=compiled_code[i];
    if(a=='\0') { seek0='M'; j=0; }
    if(a=='M') seek0='M';
    else if(a=='?') { seek0=';'; seek1=':'; }
    else if(a==':') seek0=';';
    else if(a=='{') { seek0='}'; }
    if(seek0)
    {
      for(;;j++)
      {
        int a=compiled_code[j];
        if(a=='\0' || a==seek0 || a==seek1)
        {
          if(i==j || a==0) compiled_hints[i]=0;
              else compiled_hints[i]=j+1;
          break;
        }
      }
    }
    if(a=='\0') break;
  }

  /* DEBUG: dump code */
  /*
  for(i=0;i<vm.codelgt;i++)
  {
    printf("slot %x: opcode %c, hints %x\n",
      i,compiled_code[i],compiled_hints[i]);
  }
  for(i=0;i<vm.datalgt;i+=32)
  {
    printf("datapoint %d/%d: %x\n",i,vm.datalgt,compiled_data[i>>5]);
  }
  */
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

  vm.ip=compiled_code;
  vm.mediacontext=0;
  vm.videomode=0;
  vm.audiomode=0;
  vm.visiblepage=1;
  vm.dataptr=0;
  vm.userinput=0;
  vm.stopped=0;
  vm.audiotime=vm.videotime=gettimevalue();

  vm.spchange[0]=vm.spchange[1]=0;
  vm.wcount[0]=vm.wcount[1]=0;
  vm.currentwcount[0]=vm.currentwcount[1]=0;
  vm.prevsp[0]=vm.prevsp[1]=0;

  /* zero out memory */
  if(!vm.datalgt) memset(vm.mem,0,MEMSIZE*sizeof(uint32_t));
  else
  {
    int i;
    vm.dataptr=0;
    for(i=0;i<MEMSIZE;i++) vm.mem[i]=getdatabits(32);
    vm.dataptr=0;
  }

  pushmediavariables();
}

#define SWAP(t,a,b) { t tmp=(a);(a)=(b);(b)=tmp; }

void switchmediacontext()
{
  SWAP(int32_t*,vm.stack,vm.costack);
  SWAP(uint32_t,vm.sp,vm.cosp);
  SWAP(uint32_t,vm.stackmask,vm.costackmask);
  SWAP(uint32_t*,vm.rstack,vm.corstack);
  SWAP(uint32_t,vm.rsp,vm.corsp);
  SWAP(uint32_t,vm.rstackmask,vm.corstackmask);
  vm.mediacontext=vm.preferredmediacontext;
}

void stepmediacontext(int skippoint,int at_eoc)
{
  vm.spchange[vm.mediacontext]=vm.sp-vm.prevsp[vm.mediacontext];
  vm.wcount[vm.mediacontext]=vm.currentwcount[vm.mediacontext];
  vm.currentwcount[vm.mediacontext]=0;
  vm.prevsp[vm.mediacontext]=vm.sp;

  if(vm.mediacontext==vm.preferredmediacontext)
  {
    //if(vm.rsp==0)
    vm.ip=compiled_code+skippoint;
    //         else 
    //         vm.ip=compiled_code+(vm.rstack[vm.rsp-1]%vm.codelgt);
  } else
  {
    switchmediacontext();
    //if(vm.rsp!=0) vm.ip=compiled_code+(vm.rstack[vm.rsp-1]%vm.codelgt);
    //  else
    if(at_eoc) vm.ip=compiled_code;
  }
}

void flipvideopage()
{
  vm.visiblepage=((vm.sp>>16)&1)^1;
  //vm.visiblepage^=1;
  for(;;)
  {
    uint32_t newt=gettimevalue();
    if(newt!=vm.videotime) break;
    waitfortimechange();
  }
  vm.videotime=gettimevalue();
}

void pushmediavariables()
{
  vm.currentwcount[vm.mediacontext]++;
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

int vm_run()
{
  int cycles;
  if(vm.stopped) return 0;
  for(cycles=10240;cycles;cycles--)
  {
    char op=*vm.ip++;
    int32_t*a=&vm.stack[vm.sp],*b;

    switch(op)
    {
      /*** NUMBERS ***/

      case(OP_LOADIMM):
        MOVESP(1);
        vm.stack[vm.sp]=compiled_hints[vm.ip-1-compiled_code];
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
        b=&vm.stack[vm.sp];
        {int64_t m=*a;
         m*=((int32_t)*b);
         *b=m>>16;
         }
        break;

      case('/'):	// (b a -- a/b)
        MOVESP(-1);
        b=&vm.stack[vm.sp];
        if(!*a)*b=0;
        else
        {int64_t m=*b;
         m<<=16;
         m/=((int32_t)*a);
         *b=m;}
        break;

      case('%'):	// (b a -- a%b)
        MOVESP(-1);
        b=&vm.stack[vm.sp];
        if(!*a)*b=0;
        else
        *b=(*b%*a);
        break;
    
      case('q'):	// (a -- sqrt(a), 0 if a<0)
        if(*a<0) *a=0;
        else *a=sqrt(*a/65536.0)*65536.0;
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
        b=&vm.stack[vm.sp];
        {int steps=(*a>>16)&31;
         *b=ROR(*b,steps);
        }
        break;

      case('l'):	// (b a -- b >> a)
        MOVESP(-1);
        b=&vm.stack[vm.sp];
        {int steps=(*a>>16)&63;
         uint32_t w=*b;
         if(steps<32)
         *b=(w<<steps); else *b=(w>>(steps-32));
         }
        break;

      case('~'):	// (a -- NOT a)
        *a=~*a;
        break;

      case('s'):	// (a -- sin(a))
        *a=sin(*a*(2*M_PI/65536.0))*65536.0;
        break;
      case('a'):	// (b a -- atan2(a,b))
        MOVESP(-1);
        b=&vm.stack[vm.sp];
        *b=atan2(*a,*b)*(65536.0/(2*M_PI));
        break;

      case('<'):	// (a -- a<0?a:0)
        if(*a>=0)*a=0;
        break;
      case('>'):	// (a -- a>0?a:0)
        if(*a&0x80000000)*a=0;
        break;
      case('='):	// (a -- a==0?1:0)
        if(*a)*a=0x10000;else *a=0;
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
        stepmediacontext(compiled_hints[vm.ip-compiled_code-1],0);
        pushmediavariables();
        break;

      case('\0'):	// end of code
        //vm.ip=compiled_code; // or top of rstack (don't pop it)
        stepmediacontext(compiled_hints[vm.ip-compiled_code-1],1);
        pushmediavariables();
        break;
        
      case('w'):	// whereami
        pushmediavariables();
        break;
      
      case('T'):	// terminate program
        vm.ip--;
        vm.stopped=1;
        return 10240-cycles;

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
        vm.ip=compiled_code+compiled_hints[vm.ip-compiled_code-1];
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
        vm.rstack[vm.rsp]=vm.ip-compiled_code;
        break;

      case('L'):	// loop
        {uint32_t*i=&vm.rstack[(vm.rsp-1)&vm.rstackmask];
        (*i)--;
        if(*i==0) MOVERSP(-2); else
          vm.ip=(vm.rstack[vm.rsp]%vm.codelgt)+compiled_code;
        }
        break;

      case(']'):	// while
        MOVESP(-1);
        if(*a) vm.ip=(vm.rstack[vm.rsp]%vm.codelgt)+compiled_code;
          else MOVERSP(-1);
        break;

      case('J'):	// jump
        {int point=*a%vm.codelgt; // !!! addressing will change
        MOVESP(-1);
        vm.ip=compiled_code+point;}
        break;
        
      /*** PROGRAM CONTROL: Subroutines ***/

      case('{'):	// defsub
        MOVESP(-1);
        vm.mem[ROL(*a,16)&(MEMSIZE-1)]=vm.ip-compiled_code;
        vm.ip=compiled_code+compiled_hints[vm.ip-1-compiled_code];
        break;
      case('}'):	// ret
        vm.ip=compiled_code+(vm.rstack[vm.rsp]%vm.codelgt);
        MOVERSP(-1);
        break;
      case('V'):	// visit
        MOVESP(-1);
        MOVERSP(1);
        vm.rstack[vm.rsp]=vm.ip-compiled_code;
        vm.ip=((vm.mem[ROL(*a,16)&(MEMSIZE-1)])%vm.codelgt)+compiled_code;
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
  return 10240;
}
