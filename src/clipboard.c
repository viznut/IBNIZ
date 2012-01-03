#include "ibniz.h"

#if defined(WIN32)
#include <windows.h>

void clipboard_load()
{
  HGLOBAL data;
  if (!IsClipboardFormatAvailable(CF_TEXT)) 
    return; 
  if (!OpenClipboard(NULL)) 
    return; 
  data = GetClipboardData(CF_TEXT);
  if(data)
  {
    char*t=(char*)GlobalLock(data);
    int lgt=strlen(t);
    if(clipboard) free(clipboard);
    clipboard=malloc(strlen(t)+1);
    strcpy(clipboard,t);
    GlobalUnlock(data);
  }
  CloseClipboard();
}

void clipboard_store()
{
  HGLOBAL buffer;

  if (!OpenClipboard(NULL)) 
    return; 
  EmptyClipboard();
  
  buffer = GlobalAlloc(GMEM_DDESHARE,strlen(clipboard)+1);
  if(!buffer)
  {
    CloseClipboard();
    return;
  }
  buffer = (char*)GlobalLock(buffer);
  strcpy(buffer,clipboard);
  GlobalUnlock(buffer);
  SetClipboardData(CF_TEXT,buffer);
  CloseClipboard();
}

#elif defined(X11)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

struct
{
  SDL_SysWMinfo swi;
  Atom cbatom;
} clipbrd;

void clipboard_init()
{
  if(!clipbrd.cbatom)
  {
    SDL_GetWMInfo(&clipbrd.swi);
    if(!clipbrd.swi.info.x11.display) return;
    clipbrd.swi.info.x11.lock_func();
    clipbrd.cbatom = XInternAtom(clipbrd.swi.info.x11.display,
      "IBNIZ_CB",False);
    clipbrd.swi.info.x11.unlock_func();
  }
}

char* getcbtext(XSelectionEvent *e)
{
  Atom type;
  int format,lgt;
  unsigned long nitems;
  unsigned long remaining;
  unsigned char *xdata;

  if(e->property==None) return NULL;

  xdata = NULL;
  XGetWindowProperty(e->display, e->requestor,
    clipbrd.cbatom, 0, 0, False,
    AnyPropertyType, &type, &format,
    &nitems, &remaining, &xdata);

  if(type == None) return NULL;
  
  if(xdata) XFree(xdata);

  xdata = NULL;
  XGetWindowProperty(e->display, e->requestor,
    clipbrd.cbatom, 0, (remaining+3)/4, False,
    AnyPropertyType, &type, &format,
    &nitems, &remaining, &xdata);

  if(type==None || !xdata) return NULL;

  lgt=(nitems*format)/8;

  if(lgt) XStoreBytes(clipbrd.swi.info.x11.display,xdata,lgt);

  XDeleteProperty(e->display,e->requestor,clipbrd.cbatom);

  return xdata;
}

char* readX11clipboard()
{
  Window owner;

  owner = XGetSelectionOwner(clipbrd.swi.info.x11.display,XA_PRIMARY);

  if(owner==clipbrd.swi.info.x11.window)
    return NULL;

  if(owner!=None)
  {
    int i=64;

    XConvertSelection(clipbrd.swi.info.x11.display,XA_PRIMARY,XA_STRING,
      clipbrd.cbatom,clipbrd.swi.info.x11.window,CurrentTime);
    XFlush(clipbrd.swi.info.x11.display);

    for(;i;i--)
    {
      XEvent e;
      XNextEvent(clipbrd.swi.info.x11.display, &e);
      if(e.type==SelectionNotify)
      {
        if(e.xselection.property==None) return NULL;
        return getcbtext(&e.xselection);
      }
    }
  } else return NULL;
}

void clipboard_load()
{
  char*cbt;  
  clipboard_init();
  if(!clipbrd.cbatom) return;
  clipbrd.swi.info.x11.lock_func();
  cbt=readX11clipboard();
  clipbrd.swi.info.x11.unlock_func();
  if(cbt)
  {
    if(clipboard) free(clipboard);
    clipboard=cbt;
  }
}

void clipboard_store()
{
  clipboard_init();
  if(!clipbrd.cbatom) return;
  clipbrd.swi.info.x11.lock_func();
  XSetSelectionOwner(clipbrd.swi.info.x11.display,XA_PRIMARY,
    clipbrd.swi.info.x11.window,CurrentTime);
  XFlush(clipbrd.swi.info.x11.display);
  clipbrd.swi.info.x11.unlock_func();
}

void clipboard_handlesysreq(SDL_Event*e)
{
  XEvent res;
  if(e->syswm.msg->event.xevent.type==SelectionRequest)
  {
    XSelectionRequestEvent*req=
      &(e->syswm.msg->event.xevent.xselectionrequest);
    if(req->target==XA_STRING)
    {
      XChangeProperty(clipbrd.swi.info.x11.display,req->requestor,
        req->property,XA_STRING,8,PropModeReplace,clipboard,strlen(clipboard));
      res.xselection.property=req->property;
    } else
      res.xselection.property=None;

    res.xselection.type=SelectionNotify;
    res.xselection.display=clipbrd.swi.info.x11.display;
    res.xselection.requestor=req->requestor;
    res.xselection.target=req->target;
    res.xselection.time=req->time;
    XSendEvent(clipbrd.swi.info.x11.display,req->requestor,0,0,&res);
    XFlush(clipbrd.swi.info.x11.display);
  }
}
#else

void clipboard_load()
{
}

void clipboard_store()
{
}

#endif
