#ifndef WPCP_LWS_H
#define WPCP_LWS_H

#include <wpcp.h>

#if defined(WPCP_LWS_STATIC)
#define WPCP_LWS_EXPORT
#elif defined(WPCP_LWS_EXPORTS)
#define WPCP_LWS_EXPORT WPCP_EXPORT_DECLARATION
#else
#define WPCP_LWS_EXPORT WPCP_IMPORT_DECLARATION
#endif

WPCP_BEGIN_EXTERN_C

struct wpcp_lws_t {
  struct wpcp_t* wpcp;
  int argc;
  char** argv;
  const char* (*handle_argument)(const char* name, const char* value);
  void (*start)(void);
  void (*stop)(void);
};

typedef void (*wpcp_lws_init_cleanup_function_t)(struct wpcp_lws_t* lws);

WPCP_LWS_EXPORT int wpcp_lws_main(int argc, char* argv[], wpcp_lws_init_cleanup_function_t init, wpcp_lws_init_cleanup_function_t cleanup);

WPCP_LWS_EXPORT void wpcp_lws_lock(void);

WPCP_LWS_EXPORT void wpcp_lws_unlock(void);

#define WPCP_LWS_MAIN(init, cleanup) int main(int argc, char* argv[]) { return wpcp_lws_main(argc, argv, init, cleanup); }

WPCP_END_EXTERN_C

#endif
