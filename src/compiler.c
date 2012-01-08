#include "ibniz.h"

#if defined(X86) || defined(AMD64)
#  include "gen.h"
#  include "gen_x86.c"
#elsif defined(IBNIZ2C)
#  include "gen.h"
#  include "gen_c.c"
#else
#  define NONATIVECODE
#endif

void compiler_parse(char*src)
{
  char*d=vm.parsed_code;
  uint32_t*hd=vm.parsed_hints;
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
    vm.parsed_data[0]=0;
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
             vm.parsed_data[vm.datalgt>>5]|=a<<s;
             vm.parsed_data[(vm.datalgt>>5)+1]=0;
           }
           else
           {
             vm.parsed_data[vm.datalgt>>5]|=a>>(0-s);
             vm.parsed_data[(vm.datalgt>>5)+1]=a<<(32+s);
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
        vm.parsed_data[vm.datalgt>>5]|=vm.parsed_data[0]>>i;
        i*=2;
      }
    }
    if(!pad) vm.parsed_data[(vm.datalgt>>5)+1]=vm.parsed_data[0];
    else
    {
      vm.parsed_data[(vm.datalgt>>5)+1]=
        (vm.parsed_data[0]<<(32-pad)) |
        (vm.parsed_data[1]>>pad);
    }
    }
  }

  /* precalculate skip points */
  vm.codelgt=d-vm.parsed_code;
  for(i=0;;i++)
  {
    int j=i+1,seek0=0,seek1=0,seek2=0;
    char a=vm.parsed_code[i];
    if(a=='\0') { seek0='M'; j=0; }
    if(a=='M') seek0='M';
    else if(a=='?') { seek0=';'; seek1=':'; }
    else if(a==':') seek0=';';
    else if(a=='{') { seek0='}'; }
    if(seek0)
    {
      for(;;j++)
      {
        int a=vm.parsed_code[j];
        if(a=='\0' || a==seek0 || a==seek1)
        {
          if(i==j || a==0) vm.parsed_hints[i]=0;
              else vm.parsed_hints[i]=j+1;
          break;
        }
      }
    }
    if(a=='\0') break;
  }
}

/** utility functions for code generator **/

#ifndef NONATIVECODE

#define GSV_REG 0
#define GSV_ABS 1
#define GSV_JMP 2
#define GENSTACKDEPTH (NUMREGS*3)

#define IVAR_T 0
#define IVAR_Y 1
#define IVAR_X 2

struct {
  int gsp;
  struct gsv_t {
    char type;
    uint32_t val;
  } gs[GENSTACKDEPTH];
  uint32_t usedregs;
//  uint32_t usedregswithtyx;
//  int treg,yreg,xreg;
//  int treg0,yreg0,xreg0;
} gen;

void freereg(int reg)
{
  gen.usedregs&=~(1<<reg);
}

void checkregusage()
{
  int i=0;
  gen.usedregs=0;
  for(;i<=gen.gsp;i++)
    if(gen.gs[i].type==GSV_REG)
      gen.usedregs|=1<<i;
//  gen.usedregswithtyx=gen.usedregs;
//  if(gen.treg>=0) gen.usedregswithtyx|=1<<gen.treg;
//  if(gen.yreg>=0) gen.usedregswithtyx|=1<<gen.yreg;
//  if(gen.xreg>=0) gen.usedregswithtyx|=1<<gen.xreg;
}

void flushstackbottom()
{
  if(gen.gsp>=0)
  {
    int i;
    if(gen.gs[0].type==GSV_REG)
      gen_push_reg(gen.gs[0].val);
    else
      gen_push_imm(gen.gs[0].val);
    for(i=0;i<gen.gsp;i++)
      gen.gs[i]=gen.gs[i+1];
    gen.gsp--;
    checkregusage();
  }
}

int allocreg()
{
  int i;
  while(gen.usedregs==(1<<NUMREGS)-1)
    flushstackbottom();
  /*
  if(gen.usedregswithtyx==(1<<NUMREGS)-1)
  {
    int tyxregs=gen.usedregs^gen.usedregswithtyx;
    if(gen.treg>0 && (tyxregs&(1<<gen.treg)))
    {
      i=gen.treg;
      gen.treg=-1;
      checkregusage();
      return i;
    }
    if(gen.yreg>0 && (tyxregs&(1<<gen.yreg)))
    {
      i=gen.yreg;
      gen.yreg=-1;
      checkregusage();
      return i;
    }
    if(gen.xreg>0 && (tyxregs&(1<<gen.xreg)))
    {
      i=gen.xreg;
      gen.xreg=-1;
      checkregusage();
      return i;
    }
  }
  */
  for(i=0;i<NUMREGS;i++)
  {
    if(!(gen.usedregs&(1<<i)))
    {
      gen.usedregs|=1<<i;
      return i;
    }
  }
  exit(1); // we have a fatal error if this happens
}

void gen_flushpartialstack(int howmany)
{
  for(;howmany>0;howmany--)
    flushstackbottom();
}

void growstack(int type,uint32_t val)
{
  gen.gsp++;
  if(gen.gsp>=GENSTACKDEPTH)
    gen_flushpartialstack(3);
  gen.gs[gen.gsp].type=type;
  gen.gs[gen.gsp].val=val;
}

void growstackri(int r,int32_t i)
{
  if(r<0) growstack(GSV_ABS,i);
     else growstack(GSV_REG,r);
}

int popintoreg()
{
  int r;
  if(gen.gsp>=0) exit(1); // should precheck this
  r=allocreg();
  growstack(GSV_REG,r);
  gen_pop_reg(r);
  return r;
}

void stateinit()
{
  gen.gsp=-1;
  //gen.xreg=gen.yreg=gen.treg=-1;
  gen.usedregs=0; //gen.usedregswithtyx=0;
}

int popstackval(int32_t*i)
{
  int r;
  if(gen.gsp<0-depth)
    r=popintoreg();
  else
  {
    if(gen.gs[gen.gsp].type!=GSV_REG)
    {
      r=-1;
      if(i) *i=gen.gs[gen.gsp].val;
    } else
      r=gen.gs[gen.gsp].val;
    gen.gsp--;
  }
  return r;
}

/*** user-callable ***/

gen_flushstack()
{
  gen_flushpartialstack(gen.gsp+1);
}

/* stack ops */

gen_pop()
{
  if(gen.gsp>=0)
    gen.gsp--;
  else
    gen_pop_noreg();
}

gen_dup()
{
  if(gen.gsp>=0)
  {
    growstack(gen.gs[gen.gsp].type,gen.gs[gen.gsp].val);
  } else
  {
    int r=allocreg();
    growstack(GSV_REG,r);
    gen_dup_reg(r);
  }
}

gen_swap()
{
  int r0,r1;
  int32_t i0,i1;
  r0=popintoreg(&i0);
  r1=popintoreg(&i1);
  growstackri(r0,i0);
  growstackri(r1,i1);
}

gen_trirot()
{
  int r0,r1,r2;
  int32_t i0,i1,i2;
  r0=popintoreg(&i0);
  r1=popintoreg(&i1);
  r2=popintoreg(&i2);
  growstackri(r1,i1);
  growstackri(r0,i0);
  growstackri(r2,i2);
}

gen_pick()
{
}

gen_bury()
{
}

/* loadimm */

gen_loadimm(int val)
{
  growstack(GSV_ABS,val);
}

/* arithmetic */


#define BINOP(name,immimm) \
gen_##name ()
{ \
  int r0,r1; \
  int32_t i0,i1; \
  r0=popstackval(&i0); \
  r1=popstackval(&i1); \
  if(r0<0 && r1<0) \
  { \
    growstack(GSV_ABS,immimm); \
  } \
  else \
  if(r0>=0 && r1>=0) \
  { \
    growstack(GSV_REG,r0); \
    gen_##name##_reg_reg (r0,r1); \
  } \
  else \
  { \
    growstack(GSV_REG,r0>=0?r0:r1); \
    gen_##name##_reg_imm (r0>=0?r0:r1,r0>=0?i1:i0); \
  } \
}

BINOP(add,i1+i0)
BINOP(sub,i1-i0)
BINOP(and,i1&i0)
BINOP(mul,IBNIZ_MUL(i1,i0))
BINOP(div,IBNIZ_DIV(i1,i0))
BINOP(and,i1&i0)
BINOP(xor,i1^i0)
BINOP(or,i1|i0)

gen_neg()
{
  int32_t i;
  r=popstackval(&i);
  if(r<0)
    growstack(GSV_ABS,~i);
  else
  {
    growstack(GSV_REG,r);
    gen_neg_reg(r);
  }
}

// gen_load_reg_imm(int r,uint32_t imm)
// gen_add_reg_reg(int r,int r)
// gen_add_reg_imm(int r,uint32_t imm)


gen_whereami()
{
  // todo should also update ivars according to sp
  int t=allocreg();
  int y=allocreg();
  int x=allocreg();
  growstack(GSV_REG,t);
  growstack(GSV_REG,y);
  growstack(GSV_REG,x);
  gen_mov_reg_ivar(t,IVAR_T);
  gen_mov_reg_ivar(y,IVAR_Y);
  gen_mov_reg_ivar(x,IVAR_X);
}

gen_tyxloop_init()
{
  gen.gsp=-1;
  // store treg0 yreg0 xreg0 to ensure loopability
  /*
  gen.treg0=gen.treg=allocreg();
  gen.yreg0=gen.yreg=allocreg();
  gen.xreg0=gen.xreg=allocreg();
  growstack(GSV_REG,gen.treg);
  growstack(GSV_REG,gen.yreg);
  growstack(GSV_REG,gen.xreg);
  gen_mov_reg_ivar(gen.treg,IVAR_T);
  gen_mov_reg_ivar(gen.yreg,IVAR_Y);
  gen_mov_reg_ivar(gen.xreg,IVAR_X);
  */
  gen_nativeinit();
  gen_whereami();
  gen_label(0);
}

// real whereami can use any regs

gen_tyxloop_iterator()
{
  int t,y,x;
  gen_flushstack();
  t=allocreg();
  y=allocreg();
  x=allocreg();
  growstack(GSV_REG,t);
  growstack(GSV_REG,y);
  growstack(GSV_REG,x);
  gen_mov_reg_ivar(t,IVAR_T);
  gen_mov_reg_ivar(y,IVAR_Y);
  gen_mov_reg_ivar(x,IVAR_X);
  gen_add_reg_imm(x,2);
  gen_mov_ivar_reg(IVAR_X,x);
  gen_cmpjne_reg_inc_imm_lab(x,0x00010000,0);
}

gen_finish()
{
  gen_flushstack();
  gen_tyxloop_iterator();
  gen_nativefinish();
}
#endif

int compiler_compile()
{
#ifndef NONATIVECODE
  int i;
  gen_tyxloop_init();
  for(i=0;i<vm.codelgt;i++)
  {
    char a=vm.parsed_code[i];
    switch(a)
    {
      case(OP_LOADIMM):
        gen_loadimm(vm.parsed_hints[i]);
        break;
      
      case('d'):
        gen_dup();
        break;
      case('p'):
        gen_pop();
        break;
      case('x'):
        gen_swap();
        break;
      case('v'):
        gen_trirot();
        break;
        
      case('+'):
        gen_add();
        break;
      case('-'):
        gen_sub();
        break;
      case('*'):
        gen_mul();
        break;
      case('/'):
        gen_div();
        break;
      case('%'):
        gen_mod();
        break;
      case('&'):
        gen_and();
        break;
      case('|'):
        gen_or();
        break;
      case('^'):
        gen_xor();
        break;
      case('~'):
        gen_neg();
        break;
    }
  }
  gen_finish();
  return 0;
#else
  return -1;
#endif
}
