#ifndef WPCP_LWS_MAIN_H
#define WPCP_LWS_MAIN_H

#include <wpcp_lws.h>

WPCP_BEGIN_EXTERN_C

struct wpcp_lws_main_t {
  struct wpcp_lws_options_t options;

  int argc;
  char** argv;
  const char* (*handle_argument)(const char* name, const char* value);
  void (*start)(struct wpcp_lws_t* wpcp_lws);
  void (*stop)(struct wpcp_lws_t* wpcp_lws);
  int service_timeout_ms;
  void (*service)(struct wpcp_lws_t* wpcp_lws);
};

typedef void (*wpcp_lws_init_cleanup_function_t)(struct wpcp_lws_main_t* lws);

WPCP_LWS_EXPORT int wpcp_lws_main(int argc, char* argv[], wpcp_lws_init_cleanup_function_t init, wpcp_lws_init_cleanup_function_t cleanup);

WPCP_LWS_EXPORT void wpcp_lws_main_lock(void);

WPCP_LWS_EXPORT void wpcp_lws_main_unlock(void);

#define WPCP_LWS_MAIN(init, cleanup) int main(int argc, char* argv[]) { return wpcp_lws_main(argc, argv, init, cleanup); }

WPCP_END_EXTERN_C

#endif
