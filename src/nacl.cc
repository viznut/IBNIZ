#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>

#include "SDL/SDL.h"
#include "SDL/SDL_nacl.h"

extern "C" int sdl_main(int argc, char** argv);

class IBNIZInstance : public pp::Instance {
 private:
  pthread_t main_thread_;

  static void* Start(void* arg) {
    const char* argv[] = { "ibniz", NULL };
    sdl_main(1, const_cast<char**>(argv));
    return 0;
  }

 public:
  explicit IBNIZInstance(PP_Instance instance)
      : pp::Instance(instance) {
    RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE |
                       PP_INPUTEVENT_CLASS_KEYBOARD);
  }

  virtual ~IBNIZInstance() {
    if (main_thread_) { pthread_join(main_thread_, NULL); }
  }

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    SDL_NACL_SetInstance(pp_instance(), 512, 512);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    pthread_create(&main_thread_, NULL, Start, NULL);
    return true;
  }

  virtual bool HandleInputEvent(const pp::InputEvent& event) {
    SDL_NACL_PushEvent(event);
    return true;
  }
};

class IBNIZModule : public pp::Module {
 public:
  IBNIZModule() : pp::Module() {}
  virtual ~IBNIZModule() {}

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new IBNIZInstance(instance);
  }
};

namespace pp {
Module* CreateModule() {
  return new IBNIZModule();
}
}  // namespace pp
