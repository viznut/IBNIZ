#include "ibniz.h"
#include <math.h>

#if defined(X86) || defined(AMD64)
#  include "gen.h"
#  include "gen_x86.c"
#elif defined(IBNIZ2C)
#  include "gen.h"
#  include "gen_c.c"
#else
#  define NONATIVECODE
#endif

//#define COMPILERDEBUG

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

typedef struct
{
  char type;
  uint32_t val;
} gsv_t;

struct {
  int gsp;
  gsv_t gs[GENSTACKDEPTH];
  
  int grsp;
  gsv_t grs[GENSTACKDEPTH];

  uint32_t usedregs;

  // memory-register mapping as well
  // flushrstack();
  // flushmemory();

  uint8_t*co0;
  uint8_t*co;
  int srcidx;
  // structure for label-address mapping
  // (store labels as co0-relative!)
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
      gen.usedregs|=1<<gen.gs[i].val;
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

int allocreg_withprefs(int preferred,int forbidden)
{
  int i;
  int excluded;

#ifdef COMPILERDEBUG
  printf("// alloc: prefer %x forbid %x used %x\n",
    preferred,forbidden,gen.usedregs);
#endif

  while((gen.usedregs|forbidden)==(1<<NUMREGS)-1)
    flushstackbottom();

  excluded=gen.usedregs|forbidden|(~preferred);
  for(i=0;i<NUMREGS;i++)
  {
    if(!(excluded&(1<<i)))
    {
      gen.usedregs|=1<<i;
      return i;
    }
  }

  excluded=gen.usedregs|forbidden;
  for(i=0;i<NUMREGS;i++)
  {
    if(!(excluded&(1<<i)))
    {
      gen.usedregs|=1<<i;
      return i;
    }
  }

  fprintf(stderr,"allocreg: no free regs after gs emptied!?");
  exit(-1);
}

int allocreg()
{
  int i;
  int cands;
  return allocreg_withprefs(0,0);
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
  if(gen.gsp>=0)
  {
    fprintf(stderr,"popintoreg: gsp>=0");
    exit(-1);
  }
  r=allocreg();
  //growstack(GSV_REG,r);
  gen_pop_reg(r);
  return r;
}

void stateinit()
{
  gen.gsp=-1;
  gen.grsp=-1;
  //gen.xreg=gen.yreg=gen.treg=-1;
  gen.usedregs=0; //gen.usedregswithtyx=0;
}

int popstackval(int32_t*i)
{
  int r;
  if(gen.gsp<0)
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

void popsv(gsv_t*ret)
{
  int r;
  if(gen.gsp<0)
  {
    ret->type=GSV_REG;
    ret->val=popintoreg();
  }
  else
  {
    ret->type=gen.gs[gen.gsp].type;
    ret->val=gen.gs[gen.gsp].val;
    gen.gsp--;
  }
}

/*** user-callable ***/

flushstack()
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
  gsv_t v1,v0;
  popsv(&v0);
  popsv(&v1);
  growstack(v0.type,v0.val);
  growstack(v1.type,v1.val);
}

gen_trirot()
{
  gsv_t v2,v1,v0;
  popsv(&v0);
  popsv(&v1);
  popsv(&v2);
  growstack(v1.type,v1.val);
  growstack(v0.type,v0.val);
  growstack(v2.type,v2.val);
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

// use gen_##name##_reg_reg_reg(t,s0,s1);

void binop_getsv(gsv_t*t,gsv_t*s1,gsv_t*s0,int is_commutative)
{
  popsv(s0);
  popsv(s1);
  checkregusage();
  if(s0->type!=GSV_REG && s1->type!=GSV_REG)
  {
    t->type=GSV_ABS;
  }
  else
  {
    t->type=GSV_REG;
    if(s1->type==GSV_REG)
    {
      t->val=allocreg_withprefs(1<<(s1->val),
        (s0->type==GSV_REG)?1<<(s0->val):0);
    }
    else
    {
      if(is_commutative)
      {
        SWAP(int,s0->type,s1->type);
        SWAP(uint32_t,s0->val,s1->val);
        t->val=allocreg_withprefs(1<<(s1->val),0);
      } else
      {
        t->val=allocreg_withprefs(0,1<<(s0->val));
      }
    }
  }
}

void unop_getsv(gsv_t*t,gsv_t*s)
{
  popsv(s);
  checkregusage();
  if(s->type!=GSV_REG)
  {
    t->type=GSV_ABS;
  }
  else
  {
    t->type=GSV_REG;
    t->val=allocreg_withprefs(1<<(s->val),0);
  }
}

#define BINOP_C(name,immimm) \
gen_##name () \
{ \
  gsv_t t,s1,s0; \
  binop_getsv(&t,&s1,&s0,1); \
  if(t.type==GSV_ABS) \
  { \
    uint32_t i1=s1.val,i0=s0.val; \
    growstack(GSV_ABS,immimm); \
  } \
  else \
  { \
    growstack(GSV_REG,t.val); \
    if(s0.type==GSV_REG) \
      gen_##name##_reg_reg_reg(t.val,s1.val,s0.val); \
    else \
      gen_##name##_reg_reg_imm(t.val,s1.val,s0.val); \
  } \
}

#define BINOP_NC(name,immimm) \
gen_##name () \
{ \
  gsv_t t,s1,s0; \
  binop_getsv(&t,&s1,&s0,0); \
  if(t.type==GSV_ABS) \
  { \
    uint32_t i1=s1.val,i0=s0.val; \
    growstack(GSV_ABS,immimm); \
  } \
  else \
  { \
    growstack(GSV_REG,t.val); \
    if(s1.type==GSV_REG) \
    { \
      if(s0.type==GSV_REG) \
        gen_##name##_reg_reg_reg(t.val,s1.val,s0.val); \
      else \
        gen_##name##_reg_reg_imm(t.val,s1.val,s0.val); \
    } \
    else \
    { \
      gen_mov_reg_imm(t.val,s1.val); \
      gen_##name##_reg_reg_reg(t.val,t.val,s0.val); \
    } \
  } \
}

BINOP_C(add,i1+i0)
BINOP_NC(sub,i1-i0)
BINOP_C(mul,IBNIZ_MUL(i1,i0))
BINOP_NC(div,IBNIZ_DIV(i1,i0))
BINOP_NC(mod,IBNIZ_MOD(i1,i0))
BINOP_C(and,i1&i0)
BINOP_C(xor,i1^i0)
BINOP_C(or,i1|i0)
BINOP_NC(ror,IBNIZ_ROR(i1,i0))
BINOP_NC(shl,IBNIZ_SHL(i1,i0))
BINOP_NC(atan2,IBNIZ_ATAN2(i1,i0))

#define UNOP(name,imm) \
gen_##name () \
{ \
  gsv_t t,s; \
  unop_getsv(&t,&s); \
  if(t.type==GSV_ABS) { \
    uint32_t i=s.val; \
    growstack(GSV_ABS,imm); \
  } \
  else \
  { \
    growstack(GSV_REG,t.val); \
    gen_##name##_reg_reg(t.val,s.val); \
  } \
}

UNOP(neg,~i)
UNOP(sin,IBNIZ_SIN(i))
UNOP(sqrt,IBNIZ_SQRT(i))
UNOP(isneg,IBNIZ_ISNEG(i));
UNOP(ispos,IBNIZ_ISPOS(i));
UNOP(iszero,IBNIZ_ISZERO(i));

/*
// we must harmonize post-if and post-else gs structure (and post-a and post-b as well)
// cond IF  a ELSE b ENDIF c
//        0  1    0 1     1
// cond IF a ENDIF
//        0 0     0 
// trivial: just flushstack() at these points (before if-jump, before else-jump, before endif-label)
*/

/*
// when looping: harmonize stack state in both ends
// a [ b ] c
//    0 0
*/

/*
// when jumping to subroutines:
// A { xxx }
*/

gen_if(int skipto)
{
  gsv_t cond;
  popsv(&cond);
  flushstack();
  if(cond.type==GSV_REG)
  {
    gen_beq_reg_lab(cond.val,skipto);
  } else
  {
    if(cond.val==0) gen_jmp_lab(skipto);
  }
}

gen_else(int skipto)
{
  flushstack();
  gen_jmp_lab(skipto);
  gen_label(gen.srcidx+1);
}

gen_endif()
{
  flushstack();
  gen_label(gen.srcidx+1);
}

gen_load()
{
  int r;
  gsv_t address;
  popsv(&address);
  r=allocreg();
  growstack(GSV_REG,r);
  if(address.type==GSV_REG)
    gen_load_reg_reg(r,address.val);
  else
    gen_load_reg_imm(r,address.val);
}

gen_store()
{
  int r;
  gsv_t address;
  gsv_t value;
  popsv(&address);
  popsv(&value);
  if(address.type==GSV_REG)
  {
    if(value.type==GSV_REG)
      gen_store_reg_reg(address.val,value.val);
    else
      gen_store_reg_imm(address.val,value.val);
  } else
  {
    if(value.type==GSV_REG)
      gen_store_imm_reg(address.val,value.val);
    else
      gen_store_imm_imm(address.val,value.val);
  }
}

gen_do()
{
  flushstack();
  gen_rpush_lab(gen.srcidx+1);
  gen_label(gen.srcidx+1); 

  // later: use temp storage for loop address, check loop sanity
  // analyzer will unroll small immediate loops
}

gen_times()
{
  gen_rpush();
  gen_do();
}

gen_while()
{
  gsv_t cond;
  popsv(&cond);
  if(cond.type==GSV_REG)
  {
    flushstack();
    gen_bne_reg_rstack(cond.val);
    gen_rpop_noreg();
  } else
  {
    if(cond.val)
    {
      flushstack();
      gen_jmp_rstack();
    } else
    {
      gen_rpop_noreg();
    }
  }
}

gen_loop()
{
  /*
  in c:
  if(--(rstack[(rsp-1)])) goto l0;

  in x86:
  dec dword [rsp-4]
  jne .l0
  */
}

gen_rpush()
{
  gsv_t v;
  popsv(&v);
  if(v.type==GSV_REG)
    gen_rpush_reg(v.val);
  else
    gen_rpush_imm(v.val);
}

gen_defsub()
{
  /*
  in c:
  mem[X]=((&&l7)-main);
  goto l6; // use skip to seek it
  // (compiler: store current gs and clear gs)
  l2:
  */
}

gen_return()
{
  /*
  flushstack();
  gen_jmp_rpop();
  compiler: restore stored gs
  l6:
  */
}

gen_rpop()
{
  int r=allocreg();
  growstack(GSV_REG,r);
  gen_rpop_reg(r);
}

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
  gen_label(0);
  gen_whereami();
}

// real whereami can use any regs

gen_tyxloop_iterator()
{
  int t,y,x;
  flushstack();
  t=allocreg();
  y=allocreg();
  x=allocreg();
  growstack(GSV_REG,t);
  growstack(GSV_REG,y);
  growstack(GSV_REG,x);
  gen_mov_reg_ivar(t,IVAR_T);
  gen_mov_reg_ivar(y,IVAR_Y);
  gen_mov_reg_ivar(x,IVAR_X);
  gen_add_reg_reg_imm(x,x,2);
  gen_mov_ivar_reg(IVAR_X,x);
  //gen_cmpjne_reg_inc_imm_lab(x,0x00010000,0);
}


gen_finish()
{
  flushstack();
  //gen_tyxloop_iterator();
  gen_nativefinish();
}
#endif

int compiler_compile()
{
#ifndef NONATIVECODE
  int i,j;
  gen_tyxloop_init();
  for(i=0;i<vm.codelgt;i++)
  {
    char a=vm.parsed_code[i];
    gen.srcidx=i;
#ifdef COMPILERDEBUG    
    printf("// op %c, stack now: ",a);
    for(j=0;j<=gen.gsp;j++)
      if(gen.gs[j].type==GSV_REG) printf("%c ",'A'+gen.gs[j].val);
        else printf("%x ",gen.gs[j].val);
    printf("\n");
#endif
    checkregusage();

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
      case('r'):
        gen_ror();
        break;
      case('l'):
        gen_shl();
        break;
      case('<'):
        gen_isneg();
        break;
      case('>'):
        gen_ispos();
        break;
      case('='):
        gen_iszero();
        break;
      case('q'):
        gen_sqrt();
        break;
      case('s'):
        gen_sin();
        break;
      case('a'):
        gen_atan2();
        break;
      
      case('?'):
        gen_if(vm.parsed_hints[i]);
        break;
      case(':'):
        gen_else(vm.parsed_hints[i]);
        break;
      case(';'):
        gen_endif();
        break;
      
      case('!'):
        gen_store();
        break;
      case('@'):
        gen_load();
        break;

      case('R'):
        gen_rpop();
        break;
      case('P'):
        gen_rpush();
        break;

      case('['):
        gen_do();
        break;
      case(']'):
        gen_while();
        break;
      case('X'):
        gen_times();
        break;
      case('L'):
        gen_loop();
        break;

      case('w'):
        gen_whereami();
        break;
    }
  }
  gen_finish();
  return 0;
#else
  return -1;
#endif
}
