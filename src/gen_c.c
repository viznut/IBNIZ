#include <stdio.h>
#include <stdlib.h>
#include "ibniz.h"
#include "gen.h"

#define NUMREGS 26

/*** functions implemented for compilation targets: ***/

char*ivarnames[] = { "t","y","x","v3","v4","v5","v6","v7" };

void gen_nativeinit()
{
  printf(
"#include <SDL/SDL.h>\n"
"#include <math.h>\n"
"\n"
"#define ROR(a,s)   ((((uint32_t)(a))>>(s))|(((uint32_t)(a))<<(32-(s))))\n"
"#define IBNIZ_ROR(a,b) ROR((a),(((b)>>16)&31))\n"
"#define IBNIZ_MUL(a,b) ((((int64_t)((int32_t)(a)))*((int32_t)(b)))>>16)\n"
"#define IBNIZ_DIV(a,b) ((b)==0?0:((((int64_t)((int32_t)(a)))<<16)/((int32_t)b)))\n"
"#define IBNIZ_MOD(a,b) ((b)==0?0:((a)%(b)))\n"
"#define IBNIZ_SQRT(a)    ((a<0)?0:(sqrt((a)/65536.0)*65536.0))\n"
"#define IBNIZ_SIN(a)     (sin((a)*(2*M_PI/65536.0))*65536.0)\n"
"#define IBNIZ_ATAN2(a,b) (atan2((a),(b))*(65536.0/(2*M_PI)))\n"
"#define IBNIZ_ROR(a,b) ROR((a),(((b)>>16)&31))\n"
"#define IBNIZ_SHL(a,b) (((b)&(32<<16))?((a)>>(((~(b))>>16)&31)):((a)<<((((b))>>16)&31)))\n"
"#define IBNIZ_ISNEG(a)  ((a)<0?(a):0)\n"
"#define IBNIZ_ISPOS(a)  ((a)>0?(a):0)\n"
"#define IBNIZ_ISZERO(a) ((a)==0?0x10000:0)\n"
"\n"
"int32_t mem[0x100000];\n"
"int32_t*stack=mem+0xE0000;\n"
"uint32_t*rstack=mem+0xC0000;\n"
"int32_t sp;\n"
"uint32_t rsp;\n"
"\n"
"int32_t A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z;\n"
"SDL_Surface*s;\n"
"SDL_Overlay*o;\n"
"\n"
"main()\n"
"{\n"
"  SDL_Init(SDL_INIT_VIDEO);\n"
"  s=SDL_SetVideoMode(512,512,0,0);\n"
"  o=SDL_CreateYUVOverlay(256,256,SDL_YUY2_OVERLAY,s);\n"
"  sp=0;\n"
"  for(;;)\n"
"  {\n"
"    SDL_Rect area={0,0,512,512};\n"
"    SDL_Event e;\n"
"    int t,y,x;\n"
"    SDL_PollEvent(&e);\n"
"    if(e.type==SDL_KEYDOWN)break;\n"
"\n"
"    t=((SDL_GetTicks()*3)/50)<<16;\n");

  /* tyx loop */
printf("    for(y=-0xFFFF;y<0x10000;y+=0x200)\n"
       "      for(x=-0xFFFF;x<0x10000;x+=0x200){\n");

printf("/******************* generated code begins *************************/\n");
}

void* gen_nativefinish()
{
  printf(
"/********************** generated code ends ************************/\n"
"{}"
"}\n"
"  {\n"
"    uint32_t*src=stack+0x10000-(sp&0x10000),*trg=(uint32_t*)(o->pixels[0]);\n"
"    for(x=256*128;x;x--)\n"
"    {\n"
"      uint32_t b=src[0],a=src[1];\n"
"      a=(a&0xff000000)|((a<<8)&0x00ff0000)|((b>>8)&0x0000ffff);\n"
"      a^=0x80008000;\n"
"      *trg++=a;\n"
"      src+=2;\n"
"    }\n"
"  }\n"
"  SDL_DisplayYUVOverlay(o,&area);\n"
"  SDL_Delay(10);\n"
"  }\n"
"  SDL_Quit();\n"
"}\n");

  return NULL;
}

void gen_nativerun(void*a)
{
}

/* register moves */

void gen_mov_reg_reg_reg(int t, int s)
{
  printf("%c=%c;\n",t+'A',s+'A');
}

void gen_mov_reg_imm(int t, int32_t i)
{
  printf("%c=0x%X;\n",t+'A',i);
}

void gen_mov_reg_ivar(int r, int v)
{
  printf("%c=%s;\n",r+'A',ivarnames[v]);
}

void gen_mov_ivar_reg(int v, int r)
{
  printf("%s=%c;\n",ivarnames[v],r+'A');
}

// ...more

/* load & store */

void gen_load_reg_reg(int t, int a)
{
  printf("%c=mem[ROR(%c,16)&0xFFFFF];\n",t+'A',a+'A');
}

void gen_load_reg_imm(int t, int32_t a)
{
  printf("%c=mem[0x%X];\n",t+'A',ROR(a,16)&0xFFFFF);
}

void gen_store_reg_reg(int a, int s)
{
  printf("mem[ROR(%c,16)&0xFFFFF]=%c;\n",a+'A',s+'A');
}

void gen_store_reg_imm(int a, int32_t s)
{
  printf("mem[ROR(%c,16)&0xFFFFF]=0x%X;\n",a+'A',s);
}

void gen_store_imm_reg(int32_t a, int s)
{
  printf("mem[0x%X]=%c;\n",ROR(a,16)&0xFFFFFF,s+'A');
}

void gen_store_imm_imm(int32_t a, int32_t s)
{
  printf("mem[0x%X]=0x%X;\n",ROR(a,16)&0xFFFFFF,s);
}

/* arithmetic */

void gen_add_reg_reg_reg(int t, int s1, int s)
{
  printf("%c=%c+%c;\n",t+'A',s1+'A',s+'A');
}

void gen_add_reg_reg_imm(int t, int s1, int32_t i)
{
  printf("%c=%c+0x%X;\n",t+'A',s1+'A',i);
}

void gen_sub_reg_reg_reg(int t, int s1, int s)
{
  printf("%c=%c-%c;\n",t+'A',s1+'A',s+'A');
}

void gen_sub_reg_reg_imm(int t, int s1, int32_t i)
{
  printf("%c=%c-0x%X;\n",t+'A',s1+'A',i);
}

void gen_mul_reg_reg_reg(int t, int s1, int s)
{
  printf("%c=IBNIZ_MUL(%c,%c);\n",t+'A',s1+'A',s+'A');
}

void gen_mul_reg_reg_imm(int t, int s1, int32_t i)
{
  printf("%c=IBNIZ_MUL(%c,0x%X);\n",t+'A',s1+'A',i);
}

void gen_div_reg_reg_reg(int t, int s1, int s)
{
  printf("%c=IBNIZ_DIV(%c,%c);\n",t+'A',s1+'A',s+'A');
}

void gen_div_reg_reg_imm(int t, int s1, int32_t i)
{
  printf("%c=IBNIZ_DIV(%c,0x%X);\n",t+'A',s1+'A',i);
}

void gen_mod_reg_reg_reg(int t, int s1, int s)
{
  printf("%c=IBNIZ_MOD(%c,%c);\n",t+'A',s1+'A',s+'A');
}

void gen_mod_reg_reg_imm(int t, int s1, int32_t i)
{
  printf("%c=IBNIZ_MOD(%c,0x%X);\n",t+'A',s1+'A',i);
}

void gen_and_reg_reg_reg(int t, int s1, int s)
{
  printf("%c=%c&%c;\n",t+'A',s1+'A',s+'A');
}

void gen_and_reg_reg_imm(int t, int s1, int32_t i)
{
  printf("%c=%c&0x%X;\n",t+'A',s1+'A',i);
}

void gen_or_reg_reg_reg(int t, int s1, int s)
{
  printf("%c=%c|%c;\n",t+'A',s1+'A',s+'A');
}

void gen_or_reg_reg_imm(int t, int s1, int32_t i)
{
  printf("%c=%c|0x%X;\n",t+'A',s1+'A',i);
}

void gen_xor_reg_reg_reg(int t, int s1, int s)
{
  printf("%c=%c^%c;\n",t+'A',s1+'A',s+'A');
}

void gen_xor_reg_reg_imm(int t, int s1, int32_t i)
{
  printf("%c=%c^0x%X;\n",t+'A',s1+'A',i);
}

void gen_ror_reg_reg_reg(int t,int s1,int s)
{
  printf("%c=IBNIZ_ROR(%c,%c);\n",t+'A',s1+'A',s+'A');
}

void gen_ror_reg_reg_imm(int t,int s1,int32_t i)
{
  printf("%c=IBNIZ_ROR(%c,0x%X);\n",t+'A',s1+'A',i);
}

void gen_shl_reg_reg_reg(int t,int s1,int s)
{
  printf("%c=IBNIZ_SHL(%c,%c);\n",t+'A',s1+'A',s+'A');
}

void gen_shl_reg_reg_imm(int t,int s1,int32_t i)
{
  printf("%c=IBNIZ_SHL(%c,0x%X);\n",t+'A',s1+'A',i);
}

void gen_neg_reg_reg(int t,int s)
{
  printf("%c=~%c\n",t+'A',s+'A');
}

void gen_atan2_reg_reg_reg(int t,int s1,int s)
{
  printf("%c=IBNIZ_ATAN2(%c,%c);\n",t+'A',s1+'A',s+'A');
}

void gen_atan2_reg_reg_imm(int t,int s1,int32_t i)
{
  printf("%c=IBNIZ_ATAN2(%c,0x%X);\n",t+'A',s1+'A',i);
}

void gen_sin_reg_reg(int t,int s)
{
  printf("%c=IBNIZ_SIN(%c);\n",t+'A',s+'A');
}

void gen_sqrt_reg_reg(int t,int s)
{
  printf("%c=IBNIZ_SQRT(%c);\n",t+'A',s+'A');
}

void gen_isneg_reg_reg(int t,int s)
{
  printf("%c=IBNIZ_ISNEG(%c);\n",t+'A',s+'A');
}

void gen_ispos_reg_reg(int t,int s)
{
  printf("%c=IBNIZ_ISPOS(%c);\n",t+'A',s+'A');
}

void gen_iszero_reg_reg(int t,int s)
{
  printf("%c=IBNIZ_ISZERO(%c);\n",t+'A',s+'A');
}

/* stack */

void gen_push_reg(int s)
{
  printf("stack[(++sp)&0x1FFFF]=%c;\n",s+'A');
}

void gen_push_imm(int32_t i)
{
  printf("stack[(++sp)&0x1FFFF]=0x%X;\n",i);
}

void gen_pop_reg(int t)
{
  printf("%c=stack[sp--];sp&=0x1FFFF;\n",t+'A');
}

void gen_pop_noreg()
{
  printf("sp=(sp-1)&0x1FFFF;\n");
}

void gen_dup_reg(int t)
{
  printf("%c=stack[sp];\n",t+'A');
}

// some missing: pick & bury

/* rstack */

void gen_rpush_reg(int s)
{
  printf("rsp=(rsp+1)&0x7FFF;rstack[rsp]=%c;\n",s+'A');
}

void gen_rpush_imm(int32_t s)
{
  printf("rsp=(rsp+1)&0x7FFF;rstack[rsp]=0x%X;\n",s);
}

void gen_rpush_lab(int l)
{
  printf("rsp=(rsp+1)&0x7FFF;rstack[rsp]=(&&l%d)-(void*)main;\n",l);
}

void gen_rpop_reg(int t)
{
  printf("rstack[rsp]=%c;rsp=(rsp-1)&0x7FFF;\n",t+'A');
}

void gen_rpop_noreg()
{
  printf("rsp=(rsp-1)&0x7FFF;\n");
}

// todo

/* conditionals */

void gen_beq_reg_lab(int s,int l)
{
  printf("if(!%c) goto l%d;\n",s+'A',l);
}

void gen_bne_reg_lab(int s,int l)
{
  printf("if(%c) goto l%d;\n",s+'A',l);
}

void gen_beq_reg_rstack(int s)
{
  printf("if(!%c) goto *((void*)main+rstack[rsp]);\n",s+'A');
}

void gen_bne_reg_rstack(int s)
{
  printf("if(%c) goto *((void*)main+rstack[rsp]);\n",s+'A');
}

// todo

/* jumps */

void gen_label(int l)
{
  printf("l%d:\n",l);
}

void gen_jmp_lab(int l)
{
  printf("goto l%d;\n",l);
}

void gen_jmp_rstack()
{
  printf("goto (main+rstack[rsp]);\n");
}

void gen_jmp_rpop()
{
  printf("tmp=rstack[rsp];rsp=(rsp-1)&0x7FFFF;goto (main+tmp);\n");
}

/* some more complex things */

// todo

