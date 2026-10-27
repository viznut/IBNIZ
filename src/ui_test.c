#define SDL_MAIN_HANDLED
#define main ibniz_main
#include "ui_sdl.c"
#undef main
#include <assert.h>

void pushkey(SDL_Keycode key,SDL_Keymod mod)
{
  SDL_Event e={0};
  e.type=SDL_EVENT_KEY_DOWN;
  e.key.key=key;
  e.key.mod=mod;
  assert(SDL_PushEvent(&e));
}

void testvideo()
{
  SDL_Surface*image;
  Uint8 r,g,b,a;
  ui.osd_visible=0;
  vm.visiblepage=0;
  memset(vm.mem+0xE0000,0,256*256*sizeof(int32_t));
  updatescreen();
  assert(((uint8_t*)sdl.pixels)[0]==0);
  assert(((uint8_t*)sdl.pixels)[1]==128);
  assert(((uint8_t*)sdl.pixels)[2]==0);
  assert(((uint8_t*)sdl.pixels)[3]==128);

  /* Neutral chroma, black luma must survive YUY2 rendering. */
  image=SDL_RenderReadPixels(sdl.renderer,NULL);
  assert(image);
  assert(SDL_ReadSurfacePixel(image,256,256,&r,&g,&b,&a));
  assert(r<4 && g<4 && b<4);
  SDL_DestroySurface(image);
}

void testaudio()
{
  int16_t samples[2];
  ui.auplayptr=ui.auplaytime=0;
  vm.mem[0xD0000]=0;
  vm.mem[0xD0001]=0xFFFF;
  updateaudio(NULL,(uint8_t*)samples,sizeof(samples));
  assert(samples[0]==INT16_MIN && samples[1]==INT16_MAX);
  assert(ui.auplayptr==2*0x164A9);
  ui.auplayptr=0xFFFF0000;
  updateaudio(NULL,(uint8_t*)samples,sizeof(samples));
  assert(ui.auplaytime==64*65536);

  ui.runstat=1;
  ui.audio_off=0;
  queueaudio();
  assert(SDL_GetAudioStreamQueued(sdl.audio)==2048);
  pauseaudio(0);
  assert(!SDL_AudioDevicePaused(SDL_GetAudioStreamDevice(sdl.audio)));
  pauseaudio(1);
  assert(SDL_AudioDevicePaused(SDL_GetAudioStreamDevice(sdl.audio)));
  assert(SDL_ClearAudioStream(sdl.audio));
}

void testevents(SDL_Keymod clipboardmod)
{
  SDL_Event e={0};
  SDL_FlushEvents(SDL_EVENT_FIRST,SDL_EVENT_LAST);
  ui.osd_visible=1;
  ui.runstat=0;
  ui.audio_off=1;
  vm_compile("");
  vm_init();
  e.type=SDL_EVENT_TEXT_INPUT;
  e.text.text="Ab+";
  assert(SDL_PushEvent(&e));
  pushkey(SDLK_RETURN,0);
  e.text.text="1";
  assert(SDL_PushEvent(&e));
  pushkey(SDLK_UP,SDL_KMOD_CTRL);
  pushkey(SDLK_LEFT,SDL_KMOD_SHIFT);
  pushkey(SDLK_C,clipboardmod);
  pushkey(SDLK_RIGHT,0);
  pushkey(SDLK_V,clipboardmod);

  e.type=SDL_EVENT_WINDOW_RESIZED;
  e.window.data1=800;
  e.window.data2=512;
  assert(SDL_PushEvent(&e));
  e.type=SDL_EVENT_MOUSE_MOTION;
  e.motion.x=400;
  e.motion.y=256;
  assert(SDL_PushEvent(&e));
  pushkey(SDLK_F1,0);
  pushkey(SDLK_F2,0);
  pushkey(SDLK_F1,0);
  pushkey(SDLK_F2,0);
  /* F2 resets user input, so send the mouse event again. */
  assert(SDL_PushEvent(&e));
  e.motion.x=100;
  assert(SDL_PushEvent(&e));
  pushkey(SDLK_ESCAPE,0);
  interactivemode("");
  assert(strcmp(ed.textbuffer,"Ab+\n11")==0);
  assert(strcmp(clipboard,"1")==0);
  assert(sdl.winsz==512 && sdl.xmargin==144 && sdl.ymargin==0);
  assert((vm.userinput&0xFFFF)==0x8080);
  assert(ui.runstat==0);
  assert(ui.audio_off==0 && ui.auplayptr==0 && ui.auplaytime==0);
  assert(SDL_GetAudioStreamQueued(sdl.audio)==0);
  free(ed.textbuffer);
  free(clipboard);
  clipboard=NULL;
}

void testexternalclipboard(SDL_Keymod mod)
{
  const char*expression="dd* +\nA /";
  char*text;
  SDL_FlushEvents(SDL_EVENT_FIRST,SDL_EVENT_LAST);
  ui.osd_visible=1;
  ui.runstat=0;
  clipboard=strdup("stale internal clipboard");
  assert(clipboard);
  assert(SDL_SetClipboardText(expression));
  pushkey(SDLK_A,mod);
  pushkey(SDLK_V,mod);
  pushkey(SDLK_ESCAPE,0);
  interactivemode("old expression");
  assert(strcmp(ed.textbuffer,expression)==0);
  free(ed.textbuffer);

  SDL_FlushEvents(SDL_EVENT_FIRST,SDL_EVENT_LAST);
  pushkey(SDLK_A,mod);
  pushkey(SDLK_X,mod);
  pushkey(SDLK_ESCAPE,0);
  interactivemode("dd+");
  assert(strcmp(ed.textbuffer,"")==0);
  text=SDL_GetClipboardText();
  assert(text && strcmp(text,"dd+")==0);
  SDL_free(text);
  free(ed.textbuffer);
  free(clipboard);
  clipboard=NULL;
}

int main(void)
{
  SDL_SetMainReady();
  initsdl();
  testvideo();
  testaudio();
  testevents(SDL_KMOD_CTRL);
  testexternalclipboard(SDL_KMOD_CTRL);
#ifdef SDL_PLATFORM_MACOS
  testevents(SDL_KMOD_GUI);
  testexternalclipboard(SDL_KMOD_GUI);
#endif
  assert(playbackkey(273)==SDLK_UP);
  assert(recordingkey(SDLK_F12)==293);
  quitsdl();
  return 0;
}
