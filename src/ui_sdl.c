#ifdef __APPLE__
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif
#define IBNIZ_MAIN
#include "ibniz.h"
#include "texts.i"

struct
{
  SDL_Surface*s;
  SDL_Overlay*o;
  SDL_AudioSpec as;
  int winsz,xmargin,ymargin;
} sdl;

struct
{
  char runstat;
  uint32_t timercorr;
  uint32_t paused_since;
  uint32_t auplayptr;
  uint32_t auplaytime;
  uint32_t bmtime;
  int64_t cyclecounter;
  int framecounter;
  char audio_off;
  char osd_visible;
  char benchmark_mode;
  float mops;
  float fps;

  char opt_dumpkeys;
  char opt_nonrealtime;
  char opt_playback;
  char opt_dumpmedia;
} ui;

struct
{
  //int width;
  //int height;
  int framecount;
  int subframe;
  char audiopaused;
} dumper;

#define WIDTH 256

#define EDITBUFSZ (65536*2)

struct
{
  char*cursor;
  char*selectstart;
  char*selectend;
  char*selectbase;
  char readonly;
  int firsty;
  char*textbuffer;
} ed,ed_parallel;

uint8_t font[]=
{
#include "font.i"
};

#ifndef WIN32
#  define PLAYBACKGAP 4
#else
#  define PLAYBACKGAP 16
#endif

#define DEBUG

/*** rendering of videostack and osd ***/

void drawChar8x8(uint8_t*d,uint8_t*s,uint32_t fg,uint32_t bg)
{
  int y,x;
  for(y=0;y<8;y++)
  {
    int bitmap=s[y];
    uint8_t*dd=d+WIDTH*y*2;
    for(x=0;x<16;x+=2)
    {
      dd[x]=(bitmap&128)?fg:bg;
      bitmap<<=1;
    }
  }
}

void drawTextBuffer()
{
  int x=0,y=ed.firsty;
  int scroll=0;
  char*b=ed.textbuffer;
  char*lightstart=ed.cursor,*lightend=ed.cursor;

  if(ed.selectbase)
  {
    lightstart=ed.selectstart;
    lightend=ed.selectend;
  }

  for(;;)
  {
    int a=*b&127;
    int fg=0xffffff;
    int bg=0x000000;
    if(b>=lightstart && b<=lightend)
    {
      fg=0x000000; bg=0xffffff;
      if(y<0) scroll=-y;
      else
      if(y>=28) scroll=27-y;
    }
    if(y>=0 && y<28)
    {
      drawChar8x8( ((uint8_t*)(sdl.o->pixels[0]))+x*16+y*WIDTH*16,
        font+(a>=32?a-32:0)*8,fg,bg);
    }
    x++;
    if(x>=32)
    {
      y++;
      x=0;
    }
    if(a=='\n') { y++; x=0; }
    if(!a) break;
    b++;
  }
  ed.firsty+=scroll;
}

void drawString(char*s,int x,int y)
{
  int fg=0xffffff;
  int bg=0x000000;
  while(*s)
  {
    int a=*s;
    drawChar8x8( ((uint8_t*)(sdl.o->pixels[0]))+x*16+y*WIDTH*16,
      font+(a>=32?a-32:0)*8,fg,bg);
    s++;
    x++;
  }
}

void drawStatusPanel()
{
  char buf[24];
  int sgn,spc;
  uint32_t a;
  sprintf(buf,"T=%04X",gettimevalue()&0xFFFF);
  drawString(buf,0,28);
  if(ui.runstat)
  {
    if(ui.mops>0)
    {
      sprintf(buf,"%3.3f Mops%c",ui.mops,
        ui.benchmark_mode?'!':' ');
      drawString(buf,0,30);
    }
    if(ui.fps>0)
    {
      sprintf(buf,"%2.4f fps",ui.fps);
      drawString(buf,0,31);
    }
  }
  
  spc=vm.spchange[0];
  sgn='+';
  if(spc<0) { sgn='-'; spc=0-spc; }
  if(spc>15) spc=15;
  sprintf(buf,"VIDEO S=%05X (%c%X)",vm.prevsp[0]&0x1FFFF,sgn,spc);
  drawString(buf,13,28);
  drawString(vm.videomode?"t":"tyx",13,29);

  a=vm.prevstackval[0];
  sprintf(buf,"%04X.%04X",(a>>16)&0xFFFF,a&0xFFFF);
  drawString(buf,21,29);

  spc=vm.spchange[1];
  sgn='+';
  if(spc<0) { sgn='-'; spc=0-spc; }
  if(spc>15) spc=15;
  sprintf(buf,"AUDIO S=%05X (%c%X)",vm.prevsp[1]&0x1FFFF,sgn,spc);
  drawString(buf,13,30);

  drawString(ui.audio_off?"off":
    (vm.audiomode?"ster":"mono"),13,31);

  a=vm.prevstackval[1];
  sprintf(buf,"%04X.%04X",(a>>16)&0xFFFF,a&0xFFFF);
  drawString(buf,21,31);
}

void showyuv()
{
  SDL_Rect area={sdl.xmargin,sdl.ymargin,sdl.winsz,sdl.winsz};
  SDL_DisplayYUVOverlay(sdl.o,&area);
}

void updatescreen()
{
  int x,y;
  uint32_t*s=vm.mem+0xE0000+(vm.visiblepage<<16);

  for(y=0;y<256;y++)
  for(x=0;x<128;x++)
  {
    uint32_t b=s[0],a=s[1];

    // little_endian, TODO: support big-endian
    a=(a&0xff000000)|
      ((a<<8)&0x00ff0000)|
      ((b>>8)&0x0000ffff);
    a^=0x80008000;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    // big-endian not tested!
    a=(((a>>24)&0xff)) |
      (((a>>16)&0xff)<<8) |
      (((a>>8)&0xff)<<16) |
      (((a)&0xff)<<24);
#endif
    ((uint32_t*)(sdl.o->pixels[0]))[(WIDTH/2)*y+x]=a;
    s+=2;
  }

  if(ui.osd_visible)
  {
    drawTextBuffer();
    drawStatusPanel();
  }
  showyuv();
}

/*** timer-related ***/

int getticks()
{
  if(!ui.opt_nonrealtime)
#ifdef __APPLE__
  /* please find a proper fix */
    return SDL_GetTicks()/3;
#else
    return SDL_GetTicks();
#endif
  else
  {
    return dumper.framecount*50/3;
  }
}

uint32_t getcorrectedticks()
{
  uint32_t t;
  if(ui.runstat==1) t=getticks()-ui.timercorr;
               else t=ui.paused_since-ui.timercorr;
  return t;
}

uint32_t gettimevalue()
{
  uint32_t t=getcorrectedticks();
  return (t*3)/50; // milliseconds to 60Hz-frames
}

void waitfortimechange()
{
  int wait=200;
  if(ui.benchmark_mode) return;
  if(ui.runstat==1)
  {
    int f0=gettimevalue();
    int nexttickval=((f0+1)*50)/3+ui.timercorr;
    wait=nexttickval-getcorrectedticks()+1;
    if(wait<1) wait=1; else if(wait>17) wait=17;
  }
  SDL_Delay(wait);
}

/*** input-related ***/

void getkeystates()
{
  int m=SDL_GetModState();
  uint8_t*k=SDL_GetKeyState(NULL);
  m=((m&KMOD_CTRL)?64:0)|((m&(KMOD_ALT|KMOD_META))?32:0)|((m&KMOD_SHIFT)?16:0)
    |(k[SDLK_UP]?8:0)|(k[SDLK_DOWN]?4:0)|(k[SDLK_LEFT]?2:0)|(k[SDLK_RIGHT]?1:0);
  vm.userinput=(vm.userinput&0x80FFFFFF)|(m<<24);
}

/*** audio-related ***/

void pauseaudio(s)
{
  if(!ui.opt_nonrealtime)
    SDL_PauseAudio(s);
  else
  {
    dumper.audiopaused=s;
  }
}

void updateaudio(void*dum, uint8_t*d0, int lgt)
{
  int16_t*d=(int16_t*)d0;
  uint32_t aupp0=ui.auplayptr;
  for(lgt>>=1;lgt;lgt--)
  {
    *d++ = vm.mem[0xd0000+((ui.auplayptr>>16)&0xffff)]+0x8000;
    ui.auplayptr+=0x164A9; /* (61440<<16)/44100 */
    // todo later: some interpolation/filtering
  }
  if(aupp0>ui.auplayptr)
  {
    ui.auplaytime+=64*65536;
  }
}

/*** scheduling logic (not really that ui_sdl-specific) ***/

void scheduler_check()
{
  /*
    audiotime incs by 1 per frametick
    auplaytime incs by 1<<16 per frametick
    auplayptr incs by 1<<32 per 1<<22-inc of auplaytime
  */
  uint32_t playback_at = ui.auplaytime+(ui.auplayptr>>10);
  uint32_t auwriter_at = vm.audiotime*65536+vm.prevsp[1]*64;

  if((vm.prevsp[1]>0) && playback_at>auwriter_at)
  {
    DEBUG(stderr,"%x > %x! (sp %x & %x) jumping forward\n",playback_at,auwriter_at,
      vm.sp,vm.cosp);
    vm.audiotime=((ui.auplaytime>>16)&~63)+64;
    vm.preferredmediacontext=1;
  }
  else if(playback_at+PLAYBACKGAP*0x10000>auwriter_at)
    vm.preferredmediacontext=1;
  else
    vm.preferredmediacontext=0;
}

void checkmediaformats()
{
  if(vm.wcount[1]!=0 && vm.spchange[1]<=0)
  {
    DEBUG(stderr,"audio stack underrun; shut it off!\n");
    ui.audio_off=1;
    vm.spchange[1]=vm.wcount[1]=0;
    pauseaudio(1);
  }

  if(vm.wcount[0]==0) return;

  // t-video in tyx-video mode produces 2 words extra per wcount
  if((vm.videomode==0) && (vm.spchange[0]-vm.wcount[0]*2==1))
  {
    vm.videomode=1;
    DEBUG(stderr,"switched to t-video (sp changed by %d with %d w)\n",
      vm.spchange[0],vm.wcount);
  }
  else if((vm.videomode==1) && (vm.spchange[0]+vm.wcount[0]*2==1))
  {
    vm.videomode=0;
    DEBUG(stderr,"switched to tyx-video");
  }

  if((vm.videomode==1) && (vm.spchange[1]+vm.wcount[1]*2==1))
  {
    DEBUG(stderr,"A<=>V detected!\n");
    switchmediacontext();
    vm.videomode=0;
    /* prevent loop */
    vm.spchange[0]=0; vm.wcount[0]=0;
    vm.spchange[1]=0; vm.wcount[1]=0;
  }
}

/*** dumper (event recording/playback & video/audio file dumping) ***/

void pollplaybackevent(SDL_Event*e)
{
  static int next=0,nextkey=0,nextasc=0,nextmod=0;
  int now=getticks();
  e->type=SDL_NOEVENT;
  if(now<next)
    return;
  if(nextkey)
  {
    e->type=SDL_KEYDOWN;
    e->key.keysym.sym=nextkey;
    e->key.keysym.mod=nextmod;
    e->key.keysym.unicode=nextasc;
  }
  if(!feof(stdin))
  {
    int base=next;
    scanf("%d %d %d %d",&next,&nextkey,&nextasc,&nextmod);
    next+=base;
  } else next=nextkey=nextmod=0;
}

void dumpmediaframe()
{
  static char isfirst=1;
  int x,y;
  int16_t ab[735];

  if(isfirst)
  {
    printf("YUV4MPEG2 W%d H%d F%d:%d Ip A0:0 C420mpeg2 XYSCSS=420MPEG2\n",
      640,480,60,1);
    isfirst=0;
  }
  printf("FRAME\n");

  updatescreen();
  for(y=8*2;y<248*2;y++)
  {
    for(x=0;x<32*2;x++) putchar(0);
    for(x=0;x<256;x++)
    {
      char*oo=(char*)(sdl.o->pixels[0])+(y>>1)*256*2+x*2;
      putchar(oo[0]);
      putchar(oo[0]);
    }
    for(x=0;x<32*2;x++) putchar(0);
  }

  for(y=8;y<248;y++)
  {
    for(x=0;x<32;x++) putchar(0x80);
    for(x=0;x<256;x++)
    {
      char*oo=(char*)(sdl.o->pixels[0])+y*256*2+(x>>1)*4;
      putchar(oo[1]);
    }
    for(x=0;x<32;x++) putchar(0x80);
  }

  for(y=8;y<248;y++)
  {
    for(x=0;x<32;x++) putchar(0x80);
    for(x=0;x<256;x++)
    {
      char*oo=(char*)(sdl.o->pixels[0])+y*256*2+(x>>1)*4;
      putchar(oo[3]);
    }
    for(x=0;x<32;x++) putchar(0x80);
  }
  
  if(!dumper.audiopaused)
    updateaudio(NULL,(uint8_t*)ab,735*2);
  else
    memset(ab,0,735*2);
  fwrite(ab,735*2,1,stderr);
}

void nrtframestep()
{
  dumper.subframe=0;
  dumper.framecount++;
  if(ui.opt_dumpmedia)
  {
    dumpmediaframe();
  }
}

/*** editor functions ***/

char*getlinestart(char*b)
{
  while(b>ed.textbuffer && b[-1]!='\n') b--;
  return b;
}

char*getnextlinestart(char*b)
{
  while(*b && b[-1]!='\n') b++;
  return b;
}

char*getsrcvar(char*vn)
{
  char*s=ed.textbuffer;
  int vnlen=strlen(vn);
  for(;;)
  {
    char*s1=s;
    while(*s1!='\n' && *s1!='\0') s1++;
    if(!strncmp(s,vn,vnlen))
    {
      int len=(s1-s)-vnlen+1;
      char*d=malloc(len);
      memcpy(d,s+vnlen,len);
      d[len-1]='\0';
      return d;
    }
    s=s1;
    if(!*s) return NULL;
    s++;
  }
}

void inserttosrc(char*line)
{
  int linelgt=strlen(line);
  int i=strlen(ed.textbuffer);
  for(;i>=0;i--) ed.textbuffer[i+linelgt]=ed.textbuffer[i];
  memcpy(ed.textbuffer,line,linelgt);
  ed.cursor=ed.textbuffer;
}

int ishex(char c)
{
  if((c>='0' && c<='9') || (c>='A' && c<='F')) return 1; else return 0;
}

int isibnizspace(char c)
{
  if(c==' ' || c=='\n') return 1; else return 0;
}

void ed_increment(char*p)
{
  if(p<ed.textbuffer || ed.readonly) return;
  if(*p=='.' && p[1]!='.') ed_increment(p-1);
    else
  if(ishex(*p))
  {
    if(*p=='F') { *p='0'; ed_increment(p-1); }
      else
    if(*p=='9') *p='A';
      else
    (*p)++;
  }
}

void ed_decrement(char*p)
{
  if(p<ed.textbuffer || ed.readonly) return;
  if(*p=='.' && p[1]!='.') ed_decrement(p-1);
    else
  if(ishex(*p))
  {
    if(*p=='0') { *p='F'; ed_decrement(p-1); }
      else
    if(*p=='A') *p='9';
      else
    (*p)--;
  }
}

void ed_unselect()
{
  ed.selectstart=ed.selectend;
  ed.selectbase=NULL;
}

void ed_movecursor(char*target,char with_select)
{
  if(!with_select)
    ed_unselect();
  else
  {
    if(!ed.selectbase) ed.selectbase=ed.selectstart=ed.selectend=ed.cursor;
    if(target<=ed.selectbase) ed.selectstart=target;
    if(target>=ed.selectbase) ed.selectend=target;
  }
  ed.cursor=target;
}

void ed_prev()
{
  ed_unselect();
  while(ed.cursor>ed.textbuffer && !isibnizspace(*ed.cursor)) ed.cursor--;
  while(ed.cursor>ed.textbuffer && isibnizspace(*ed.cursor)) ed.cursor--;
}

void ed_next()
{
  ed_unselect();
  while(*ed.cursor && !isibnizspace(*ed.cursor)) ed.cursor++;
  while(*ed.cursor && isibnizspace(*ed.cursor)) ed.cursor++;
  while(*ed.cursor && !isibnizspace(*ed.cursor)) ed.cursor++;
  if(ed.cursor>ed.textbuffer) ed.cursor--;
}

void ed_left(char with_select)
{
  if(ed.cursor!=ed.textbuffer)
  {
    ed_movecursor(ed.cursor-1,with_select);
  }
}

void ed_right(char with_select)
{
  if(*ed.cursor) ed_movecursor(ed.cursor+1,with_select);
}

void ed_up(char with_select)
{
  char*p=getlinestart(ed.cursor);
  int x=ed.cursor-p;
  if(x>=32) ed_movecursor(ed.cursor-32,with_select);
    else
  if(p>ed.textbuffer)
  {
    char*pp=getlinestart(p-1);
    if(p-pp-1<x) ed_movecursor(p-1,with_select);
            else ed_movecursor(pp+x,with_select);
  } else ed_movecursor(ed.textbuffer,with_select);
}

void ed_down(char with_select)
{
  char*l0=getlinestart(ed.cursor);
  char*p=getnextlinestart(ed.cursor);
  if(p==ed.cursor && *p) p=getnextlinestart(p+1);
  if(ed.cursor<p-32) ed_movecursor(ed.cursor+32,with_select);
    else
  {
    int x=ed.cursor-l0;
    ed_movecursor(p,with_select);
    while(*ed.cursor && *ed.cursor!='\n' && x)
    {
      ed_movecursor(ed.cursor+1,with_select);
       x--;
    }
  }
  if(with_select) ed.selectend=ed.cursor;
}

void ed_deleteselection()
{
  char*s;
  int gap=ed.selectend-ed.selectstart+1;
  if(gap<=0) return;
  if(ed.readonly) return;

  if(ed.cursor>ed.selectend) ed.cursor-=gap;
    else if(ed.cursor>ed.selectstart) ed.cursor=ed.selectstart;

  s=ed.selectstart;
  for(;;)
  {
    char a=s[gap];
    *s++=a;
    if(!a)break;
  }

  ed_unselect();
}

void ed_backspace(int offset)
{
  if(ed.selectend>ed.selectstart)
    ed_deleteselection();
  else
  {
    if(ed.cursor!=ed.textbuffer)
    {
      char*s=(ed.cursor+=offset);
      for(;*s;s++)*s=s[1];
    }
  }
}

void ed_save()
{
  FILE*f;
  char*fn=getsrcvar("\\#file ");
  if(!fn)
  {
    inserttosrc("\\#file untitled.ib\n");
    fn=strdup("untitled.ib");
  }
  f=fopen(fn,"w");
  DEBUG(stderr,"filename: %s\n",fn);
  if(!f) inserttosrc("\\ ERROR: couldn't save file!\n");
    else
  {
    char*s=ed.textbuffer;
    while(*s)
    {
      char*s1=s;
      while(*s1 && *s1!='\n') s1++;
      if(*s1=='\n') s1++;
      if(memcmp(s,"\\#file ",6)) fwrite(s,s1-s,1,f);
      s=s1;
    }
    fclose(f);
    free(fn);
  }
}

void ed_char(int ascii)
{
  if(ed.readonly) return;

  if(ascii==13) ascii=10;
  if(ascii==10 || (ascii>=32 && ascii<=126))
  {
    if(ed.selectbase)
    {
      ed_deleteselection();
    }

    // if in insertmode...
    {
      char*s;
      for(s=ed.cursor;*s;s++);
      if(s>=ed.textbuffer+EDITBUFSZ) return;
      for(;s>=ed.cursor;s--)s[1]=*s;
    }

    *ed.cursor++=ascii;
  }
}

void ed_copy()
{
  int lgt=ed.selectend-ed.selectstart+1;
  if(lgt<0 || !ed.selectbase) lgt=0;
  free(clipboard);
  clipboard=malloc(lgt+1);
  memcpy(clipboard,ed.selectstart,lgt);
  clipboard[lgt]='\0';
  clipboard_store();
}

void ed_paste()
{
  char*s;
  clipboard_load();
  s=clipboard;
  if(!s) return;
  while(*s)
  {
    ed_char(*s);
    s++;
  }
}

void ed_cut()
{
  ed_copy();
  ed_deleteselection();
}

void ed_switchbuffers()
{
  char tmp[sizeof(ed)];
  memcpy((void*)&tmp,(void*)&ed,sizeof(ed));
  memcpy((void*)&ed,(void*)&ed_parallel,sizeof(ed));
  memcpy((void*)&ed_parallel,(void*)&tmp,sizeof(ed));
}

char*ed_getprogbuf()
{
  if(!ed.readonly) return ed.textbuffer;
     else return ed_parallel.textbuffer;
}

/*** main loop etc ***/

void interactivemode(char*codetoload)
{
  int codechanged=0;
  uint32_t prevtimevalue=gettimevalue();
  SDL_Event e;

  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,10);
  SDL_EnableUNICODE(1);
  ed.textbuffer=malloc(EDITBUFSZ*sizeof(char));
  strncpy(ed.textbuffer,codetoload,EDITBUFSZ-1);
  ed_unselect();
  ed.firsty=0;
  ed.cursor=ed.textbuffer;
  ed.readonly=0;
  
  ed_parallel.cursor=
  ed_parallel.selectstart=
  ed_parallel.selectend=
  ed_parallel.textbuffer=helpscreen;
  ed_parallel.readonly=1;

#ifdef X11
  SDL_EventState(SDL_SYSWMEVENT,SDL_ENABLE);
#endif
  
  for(;;)
  {
    uint32_t t = gettimevalue();
    if(prevtimevalue!=t || e.type!=SDL_NOEVENT)
    {
      updatescreen();
      vm.specialcontextstep=3;
      prevtimevalue=t;
      DEBUG(stderr,"t:%x audio:%x playback:%x video:%x\n",
        t,(vm.audiotime)+(((vm.mediacontext==1)?vm.sp:vm.cosp)>>10)
        ,(ui.auplaytime>>16)+(ui.auplayptr>>26),vm.videotime);
    }
    {
      static int lastpage=0;
      if(lastpage!=vm.visiblepage)
      {
        lastpage=vm.visiblepage;
        ui.framecounter++;
        if(ui.opt_nonrealtime) nrtframestep();
      }
    }
    if(t>=120+ui.bmtime)
    {
      float secs=(t-ui.bmtime)/60.0;
      ui.mops=ui.cyclecounter/(secs*1000000);
      ui.fps=ui.framecounter/secs;      
      ui.cyclecounter=ui.framecounter=0;
      ui.bmtime=t;
    }
    if(ui.runstat==0)
    {
      if(!ui.opt_playback)
        SDL_WaitEvent(&e);
      else
      {
        e.type=SDL_NOEVENT;
        SDL_PollEvent(&e);
        if(e.type==SDL_NOEVENT)
          pollplaybackevent(&e);
        if(e.type==SDL_NOEVENT && ui.opt_nonrealtime)
          nrtframestep();
      }
    }
    else
    {
      e.type=SDL_NOEVENT;
      SDL_PollEvent(&e);
      if(ui.opt_playback && e.type==SDL_NOEVENT)
        pollplaybackevent(&e);
      if(e.type==SDL_NOEVENT)
      {
        if(codechanged) 
        {
          vm_compile(ed_getprogbuf());
          if(ui.audio_off)
          {
            ui.audio_off=0;
            pauseaudio(0);
          }
          codechanged=0;
        }
        {
          int c = vm_run();
          ui.cyclecounter+=c;
        }
        if(ui.opt_nonrealtime)
        {
          dumper.subframe++;
          if(!(dumper.subframe&4095)) nrtframestep();
        }
        checkmediaformats();
        scheduler_check();
        continue;
      }
    }
    if(e.type==SDL_QUIT) break;
    if(e.type==SDL_KEYDOWN)
    {
      int sym=e.key.keysym.sym;
      int mod=e.key.keysym.mod;

      if(ui.opt_dumpkeys)
      {
        static int last=0;
        int now=getticks();
        if(!sym && e.key.keysym.unicode)
             sym=e.key.keysym.unicode;
        printf("%d %d %d %d\n",now-last,sym,
          e.key.keysym.unicode,mod);
        last=now;
      }

      getkeystates();

      
      if(sym==SDLK_ESCAPE) break;
      else
      if(sym==SDLK_TAB)
      {
        ui.osd_visible^=1;
      }
      else
      if(sym==SDLK_F1)
      {
        pauseaudio(ui.runstat);
        ui.runstat^=1;
        if(ui.runstat==0)
        {
          ui.paused_since=getticks();
        } else
        {
          ui.timercorr+=getticks()-ui.paused_since;
          ui.mops=ui.fps=ui.bmtime=0;
        }
      }
      else
      if(sym==SDLK_F2)
      {
        ui.timercorr=ui.paused_since=getticks();
        if(codechanged)
        {
          vm_compile(ed_getprogbuf());
          codechanged=0;
        }
        vm_init();
        ui.auplayptr=ui.auplaytime=0;
        pauseaudio(ui.runstat^1);
      }
      else
      if(ui.osd_visible)
      {
        /* editor keys */
      
        if(sym==SDLK_UP && (mod&KMOD_CTRL))
        {
          ed_increment(ed.cursor);
          codechanged=1;
        }
        else
        if(sym==SDLK_DOWN && (mod&KMOD_CTRL))
        {
          ed_decrement(ed.cursor);
          codechanged=1;
        }
        else
        if(sym==SDLK_LEFT && (mod&KMOD_CTRL))
        {
          ed_prev();
        }
        else
        if(sym==SDLK_RIGHT && (mod&KMOD_CTRL))
        {
          ed_next();
        }
        else
        if(sym==SDLK_LEFT)
        {
          ed_left(mod&KMOD_SHIFT);
        }
        else
        if(sym==SDLK_RIGHT)
        {
          ed_right(mod&KMOD_SHIFT);
        }
        else
        if(sym==SDLK_UP)
        {
          ed_up(mod&KMOD_SHIFT);
        }
        else
        if(sym==SDLK_DOWN)
        {
          ed_down(mod&KMOD_SHIFT);
        }
        else
        if(sym==SDLK_BACKSPACE)
        {
          ed_backspace(-1);
          codechanged=1;
        }
        else
        if(sym==SDLK_DELETE)
        {
          ed_backspace(0);
          codechanged=1;
        }
        else
        if(sym==SDLK_F12)
        {
          ed_switchbuffers();
        }
        else
        if(sym=='s' && (mod&KMOD_CTRL))
        {
          ed_save();
        }
        else
        if(sym=='c' && (mod&KMOD_CTRL))
        {
          ed_copy();
        }
        else
        if(sym=='k' && (mod&KMOD_CTRL))
        {
          ed_copy();
        }
        else
        if(sym=='v' && (mod&KMOD_CTRL))
        {
          ed_paste();
        }
        else
        if(sym=='x' && (mod&KMOD_CTRL))
        {
          ed_cut();
        }
        else
        if(sym=='a' && (mod&KMOD_CTRL))
        {
          if(ed.selectbase) ed_unselect();
            else
          {
            ed.selectstart=ed.textbuffer;
            ed.selectend=ed.textbuffer+strlen(ed.textbuffer);
            ed.selectbase=ed.cursor;
          }
        }
        else
        if(sym=='b' && (mod&KMOD_CTRL))
        {
          ui.benchmark_mode^=1;
        }
        else
        {
          if(e.key.keysym.unicode)
          {
            ed_char(e.key.keysym.unicode);
            codechanged=1;
          }
        }
      }
    }
    else if(e.type==SDL_KEYUP)
    {
      getkeystates();
    }
    else if(e.type==SDL_MOUSEMOTION)
    {
      int y=(e.motion.y*256)/sdl.winsz;
      int x=(e.motion.x*256)/sdl.winsz;
      if(y>=0 && x>=0 && y<=255 && x<=255)
        vm.userinput=(vm.userinput&0xFFFF0000)|(y<<8)|x;
    }
    else if(e.type==SDL_MOUSEBUTTONDOWN)
    {
      vm.userinput|=0x80000000;
    }
    else if(e.type==SDL_MOUSEBUTTONUP)
    {
      vm.userinput&=0x7FFFFFFF;
    }
    else if(e.type==SDL_VIDEORESIZE)
    {
      sdl.winsz=e.resize.w<e.resize.h?e.resize.w:e.resize.h;
      sdl.xmargin=(e.resize.w-sdl.winsz)/2;
      sdl.ymargin=(e.resize.h-sdl.winsz)/2;

      SDL_FreeSurface(sdl.s);
      sdl.s=SDL_SetVideoMode(e.resize.w,e.resize.h,0,SDL_RESIZABLE);
      SDL_FreeYUVOverlay(sdl.o);
      sdl.o=SDL_CreateYUVOverlay(256,256,SDL_YUY2_OVERLAY,sdl.s);
      SDL_WM_SetCaption("IBNIZ","IBNIZ");

      showyuv();
    }
#ifdef X11
    else if(e.type==SDL_SYSWMEVENT)
    {
      clipboard_handlesysreq(&e);
    }
#endif
  }
}

#ifdef __native_client__
int sdl_main(int argc,char**argv)
#else
int main(int argc,char**argv)
#endif
{
  signed char autorun=-1;
  char*codetoload = welcometext;
  ui.opt_dumpkeys=0;
  ui.opt_nonrealtime=0;
  ui.opt_playback=0;
  ui.opt_dumpmedia=0;
  ui.opt_nonrealtime=0;
  ui.osd_visible=1;
  argv++;
  while(*argv)
  {
    char*s=*argv;
    if(*s=='-')
    {
      while(s[0]=='-') s++;
      switch((int)*s)
      {
        case('h'):
          printf(usagetext,argv[0]);
          exit(0);
        case('v'):
          puts(versiontext);
          exit(0);
        case('c'):
          argv++;
          codetoload=*argv;
          if(autorun<0) autorun=1;
          break;
        case('n'):
          autorun=0;
          break;
        case('e'):
          ui.opt_dumpkeys=1;
          break;
        case('p'):
          ui.opt_playback=1;
          break;
        case('M'):
          ui.opt_dumpmedia=1;
          ui.opt_nonrealtime=1;
          break;
      }
    } else
    {
      FILE*f=fopen(s,"r");
      char buf[EDITBUFSZ+1];
      int hdrlgt,buflgt;
      //if(codetoload) free(codetoload);
      if(!f)
      {
        fprintf(stderr,"Can't load file '%s'\n",s);
        exit(1);
      }
      hdrlgt=sprintf(buf,"\\#file %s\n",s);
      buflgt=0;
      while(!feof(f))
      {
        buflgt+=fread(buf+hdrlgt+buflgt,1,EDITBUFSZ-hdrlgt,f);
      }
      buf[hdrlgt+buflgt]='\0';
      codetoload=strdup(buf);
      if(autorun<0) autorun=1;
    }
    argv++;
  }

#ifndef __native_client__
  SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
#endif

  sdl.winsz=512;
  sdl.s=SDL_SetVideoMode(sdl.winsz,sdl.winsz,0,SDL_RESIZABLE);
  sdl.o=SDL_CreateYUVOverlay(256,256,SDL_YUY2_OVERLAY,sdl.s);
  SDL_WM_SetCaption("IBNIZ", "IBNIZ");
 
  {SDL_AudioSpec as;
   as.freq=44100;
   as.format=AUDIO_S16;
   as.channels=1;
   as.samples=512;
   as.callback=updateaudio;
   SDL_OpenAudio(&as,NULL);
   DEBUG(stderr,"buffer size: %d\n",as.samples);
  }
  
  vm_compile(codetoload);
  ui.runstat=(autorun==1)?1:0;
  if(autorun==1) ui.osd_visible=0;
  ui.timercorr=ui.paused_since=getticks();
  vm_init();
  pauseaudio(ui.runstat^1);
  interactivemode(codetoload);
  
  SDL_Quit();
}
