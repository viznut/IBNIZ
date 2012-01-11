#ifndef VM_H

#define MEMSIZE 0x100000
#define MAXCODESIZE 4096
#define MAXDATASIZE 4096

GLOBAL struct
{
  /* main register set */

    char  *ip;
   int32_t*stack;
  uint32_t sp;
  uint32_t stackmask;

  uint32_t*rstack;
  uint32_t rsp;
  uint32_t rstackmask;

  /* parallel register set */

   int32_t*costack;
  uint32_t cosp;
  uint32_t costackmask;
  
  uint32_t*corstack;
  uint32_t corsp;
  uint32_t corstackmask;

  /* i/o stuff */

  char     mediacontext; /* 0=video, 1=audio, 2=stdio */
  char     videomode;    /* 0=txy, 1=t */
  char     audiomode;    /* 0=mono, 1=stereo */
  char     preferredmediacontext;
  char     visiblepage;
  char     stopped;
  uint32_t videotime;
  uint32_t audiotime;
  uint32_t userinput;

  /* vm_slow-specific */

  char specialcontextstep;
  void(*pmv_func)();
  
  /* dynamic stack balance detection */

  uint32_t prevsp[2];
  uint32_t prevstackval[2];
  int      currentwcount[2];
  int16_t  spchange[2];
  int      wcount[2];

  /* memory */

  int32_t  mem[MEMSIZE];
  int      codelgt;
  int      datalgt;
  int      dataptr;
  uint32_t parsed_data[MAXDATASIZE];

  /* compiler-related (also directly executed by vm_slow) */

  char parsed_code[MAXCODESIZE];
  uint32_t parsed_hints[MAXCODESIZE];
} vm;

#endif

#define OP_LOADIMM '0'

#define ROL(a,s)   ((((uint32_t)(a))<<(s))|(((uint32_t)(a))>>(32-(s))))
#define ROR(a,s)   ((((uint32_t)(a))>>(s))|(((uint32_t)(a))<<(32-(s))))

#define IBNIZ_MUL(a,b) ((((int64_t)((int32_t)(a)))*((int32_t)(b)))>>16)
#define IBNIZ_DIV(a,b) ((b)==0?0:((((int64_t)((int32_t)(a)))<<16)/((int32_t)b)))
#define IBNIZ_MOD(a,b) ((b)==0?0:((a)%(b)))

#define IBNIZ_SQRT(a)    ((a<0)?0:(sqrt((a)/65536.0)*65536.0))
#define IBNIZ_SIN(a)     (sin((a)*(2*M_PI/65536.0))*65536.0)
#define IBNIZ_ATAN2(a,b) (atan2((a),(b))*(65536.0/(2*M_PI)))

#define IBNIZ_ROR(a,b) ROR((a),(((b)>>16)&31))
#define IBNIZ_SHL(a,b) (((b)&(32<<16))?((a)>>(((~(b))>>16)&31)):((a)<<((((b))>>16)&31)))

#define IBNIZ_ISNEG(a)  ((a)<0?(a):0)
#define IBNIZ_ISPOS(a)  ((a)>0?(a):0)
#define IBNIZ_ISZERO(a) ((a)==0?0x10000:0)

#define SWAP(t,a,b) { t tmp=(a);(a)=(b);(b)=tmp; }
