#include "ibniz.h"
#include "gen.h"

#ifdef AMD64
#define NUMREGS 6
#else
#define NUMREGS 14
#endif

char*regnames[]={
  "eax","ecx","edx","ebx", "esp","ebp","esi","edi"
#ifdef AMD64
  ,"r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d"
#endif
};

char*regallocorder={
  1,3,5,7,2,0,
#ifdef AMD64
  8,9,10,11,12,13,14,15
#endif
};


/*
  0 eax, 1 ecx, 2 edx,  3 ebx, 4 esp, 5 ebp, 6 esi, 7 edi
  8 r8d, 9 r9d, ...
*/

// mask out unallocable regs

void gen_nativeinit()
{
}

void gen_nativefinish()
{
  printf("ret\n");
}

gen_mov_reg_imm(int t,uint32_t imm)
{
  DEBUG(stderr,"mov %s,0x%X",regnames[t],imm);
#ifdef AMD64
  if(t&8) *gen.co++=0x41;
#endif
  *gen.co++=0xB8+(t&7);
  *((uint32_t*)gen.co)=imm;
  gen.co+=4;
}

gen_mov_reg_reg(int t,int s)
{
  DEBUG(stderr,"mov %s,%s",regnames[t],regnames[s]);
#ifdef AMD64
  if((t&8) || (s&8)) *gen.co++=0x40|((t&8)>>3)|((s&8)>>1);
#endif
  *gen.co++=0x89;
  *gen.co++=0xC0+(((t&7)<<3)|(s&7));
}

gen_add_reg_imm(int t,uint32_t imm)
{
  DEBUG(stderr,"add %s,0x%X",regnames[t],imm);
#ifdef AMD64
  if(t&8) *gen.co++=0x41;
#endif
  if(!t)
    *gen.co++=0x05;
  else
  {  
    *gen.co++=0x81;
    *gen.co++=0xC0|(t&7);
  }
  *((uint32_t*)gen.co)=imm;
  gen.co+=4;
}

gen_add_reg_reg(int t,int s)  
{
  DEBUG(stderr,"add %s,%s",regnames[t],regnames[s]);
#ifdef AMD64
  if((t&8) || (s&8)) *gen.co++=0x40|((t&8)>>3)|((s&8)>>1);
#endif
  *gen.co++=0x01;
  *gen.co++=0xC0+(((t&7)<<3)|(s&7));
}

/* TODO FINISH */
