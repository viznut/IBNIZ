#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "ibniz.h"

int clipboard_load()
{
  char*text=SDL_GetClipboardText();
  char*copy;
  if(!text)
  {
    fprintf(stderr,"Reading clipboard: %s\n",SDL_GetError());
    return 0;
  }
  copy=strdup(text);
  SDL_free(text);
  if(!copy)
  {
    fprintf(stderr,"Not enough memory to read clipboard\n");
    return 0;
  }
  free(clipboard);
  clipboard=copy;
  return 1;
}

void clipboard_store()
{
  if(!SDL_SetClipboardText(clipboard?clipboard:""))
    fprintf(stderr,"Writing clipboard: %s\n",SDL_GetError());
}
