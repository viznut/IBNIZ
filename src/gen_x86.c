#include "ibniz.h"
#include "gen.h"

#define NUMREGS 5
char*regnames[]={
  "ebx","ecx","esi","edi","ebp"
#ifdef AMD64
  ,"r8","r9","r10","r11","r12","r13","r14","r15"
#endif
};

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
