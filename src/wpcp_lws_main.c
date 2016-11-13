#include "wpcp_lws_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning(push, 2)
#endif
#include <libwebsockets.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifndef _WIN32_WCE
#include <signal.h>
#endif

WPCP_BEGIN_EXTERN_C

struct wpcp_lws_main_t g_wpcp_lws_main;
struct wpcp_lws_t* g_wpcp_lws;
bool force_exit;

#ifndef _WIN32_WCE
static void sighandler(int sig)
{
  (void) sig;
  force_exit = true;
  wpcp_lws_cancel_service(g_wpcp_lws);
}
#endif

static const char* handle_argument(struct wpcp_lws_options_t* options, const char* key, const char* value, const char* original_error)
{
  const char* err_no_value = "no value sepcified";
  const char* err_invalid_port = "port must be between 1 and 65535";

#define HANDLE_NUMBER_ARGUMENT(name, property) \
  if (!strcmp(key, name)) {                    \
    if (!value)                                \
      return err_no_value;                     \
    options->property = atoi(value);           \
    return NULL;                               \
  }

#define HANDLE_PORT_ARGUMENT(name, property) \
  if (!strcmp(key, name)) {                  \
    unsigned long int port;                  \
    if (!value)                              \
      return err_no_value;                   \
    port = strtoul(value, NULL, 10);         \
    if (port < 1 || port > 0xffffffff)       \
      return err_invalid_port;               \
    options->property = (uint16_t) port;     \
    return NULL;                             \
  }

#define HANDLE_STRING_ARGUMENT(name, property) \
  if (!strcmp(key, name)) {                    \
    if (!value)                                \
      return err_no_value;                     \
    options->property = value;                 \
    return NULL;                               \
  }

  HANDLE_STRING_ARGUMENT("http-interface", http_interface)
  HANDLE_PORT_ARGUMENT("http-port", http_port)
  HANDLE_STRING_ARGUMENT("http-rootdir", http_rootdir)

  HANDLE_STRING_ARGUMENT("https-ca-filepath", https_ca_filepath)
  HANDLE_STRING_ARGUMENT("https-cert-filepath", https_cert_filepath)
  HANDLE_STRING_ARGUMENT("https-cipher-list", https_cipher_list)
  HANDLE_STRING_ARGUMENT("https-interface", https_interface)
  HANDLE_PORT_ARGUMENT("https-port", https_port)
  HANDLE_STRING_ARGUMENT("https-private-key-filepath", https_private_key_filepath)
  HANDLE_STRING_ARGUMENT("https-private-key-password", https_private_key_password)
  HANDLE_STRING_ARGUMENT("https-rootdir", https_rootdir)

  HANDLE_STRING_ARGUMENT("rwpcp-address", rwpcp_address)
  HANDLE_STRING_ARGUMENT("rwpcp-authorization", rwpcp_authorization)
  HANDLE_STRING_ARGUMENT("rwpcp-ca-filepath", rwpcp_ca_filepath)
  HANDLE_STRING_ARGUMENT("rwpcp-cert-filepath", rwpcp_cert_filepath)
  HANDLE_STRING_ARGUMENT("rwpcp-cipher-list", rwpcp_cipher_list)
  HANDLE_STRING_ARGUMENT("rwpcp-host", rwpcp_host)
  HANDLE_STRING_ARGUMENT("rwpcp-origin", rwpcp_origin)
  HANDLE_STRING_ARGUMENT("rwpcp-path", rwpcp_path)
  HANDLE_PORT_ARGUMENT("rwpcp-port", rwpcp_port)
  HANDLE_STRING_ARGUMENT("rwpcp-private-key-filepath", rwpcp_private_key_filepath)
  HANDLE_STRING_ARGUMENT("rwpcp-private-key-password", rwpcp_private_key_password)
  HANDLE_STRING_ARGUMENT("rwpcp-proxy-address", rwpcp_proxy_address)
  HANDLE_PORT_ARGUMENT("rwpcp-proxy-port", rwpcp_proxy_port)
  HANDLE_NUMBER_ARGUMENT("rwpcp-reconnect-interval", rwpcp_reconnect_interval)

  HANDLE_STRING_ARGUMENT("server-string", server_string)

#undef HANDLE_NUMBER_ARGUMENT
#undef HANDLE_PORT_ARGUMENT
#undef HANDLE_STRING_ARGUMENT

  if (!strcmp(key, "debug-level")) {
    int debug_level;
    if (!value)
      return err_no_value;
    debug_level = atoi(value);
    if (debug_level < 0 || debug_level > 7)
      return "value must be between 0 and 7";
    wpcp_lws_set_log_level(debug_level, NULL);
    return NULL;
  }

  if (!strcmp(key, "rwpcp-secure")) {
    if (!value)
      return err_no_value;
    if (!strcmp(value, "on"))
      options->rwpcp_secure = WPCP_LWS_OPTIONS_SECURE_ON;
    else if (!strcmp(value, "off"))
      options->rwpcp_secure = WPCP_LWS_OPTIONS_SECURE_OFF;
    else if (!strcmp(value, "allow_self_signed"))
      options->rwpcp_secure = WPCP_LWS_OPTIONS_SECURE_ALLOW_SELF_SIGNED;
    else
      return "valid options are 'on', 'off' and 'allow_self_signed'";
    return NULL;
  }

  return original_error;
}

static int parse_arguments(struct wpcp_lws_options_t* options)
{
  char** args = g_wpcp_lws_main.argv + 1;
  int remaining = g_wpcp_lws_main.argc - 1;

  while (remaining) {
    const char* argument_error = "unknown error";
    const char* key = *args;
    const char* value = NULL;

    if (key[0] != '-' || key[1] != '-' || key[2] == '\0') {
      lwsl_err("invalid argument key '%s'\n", key);
      return -1;
    }

    key += 2;
    --remaining;
    ++args;

    if (remaining) {
      value = *args;
      if (value[0] != '-' || value[1] != '-' || value[2] == '\0') {
        --remaining;
        ++args;
      } else
        value = NULL;
    }

    if (g_wpcp_lws_main.handle_argument)
      argument_error = g_wpcp_lws_main.handle_argument(key, value);

    if (argument_error)
      argument_error = handle_argument(options, key, value, argument_error);

    if (argument_error) {
      lwsl_err("invalid argument '%s'%s%s%s (%s)\n", key, value ? " = '" : "", value ? value : "", value ? "'" : "", argument_error);
      return -1;
    }
  }

  return 0;
}

int wpcp_lws_main(int argc, char* argv[], wpcp_lws_init_cleanup_function_t init, wpcp_lws_init_cleanup_function_t cleanup)
{
  int ret;

#ifndef _WIN32_WCE
  signal(SIGINT, sighandler);
#endif

  g_wpcp_lws_main.argc = argc;
  g_wpcp_lws_main.argv = argv;

  if (init)
    init(&g_wpcp_lws_main);

  ret = parse_arguments(&g_wpcp_lws_main.options);
  if (!ret)
    g_wpcp_lws = wpcp_lws_create(&g_wpcp_lws_main.options);

  if (g_wpcp_lws) {
    if (g_wpcp_lws_main.start)
      g_wpcp_lws_main.start(g_wpcp_lws);

    while (!force_exit)
      wpcp_lws_service(g_wpcp_lws, 500);

    if (g_wpcp_lws_main.stop)
      g_wpcp_lws_main.stop(g_wpcp_lws);

    wpcp_lws_delete(g_wpcp_lws);
  }

  if (cleanup)
    cleanup(&g_wpcp_lws_main);

  return ret;
}

void wpcp_lws_main_lock(void)
{
  wpcp_lws_lock(g_wpcp_lws);
}

void wpcp_lws_main_unlock(void)
{
  wpcp_lws_unlock(g_wpcp_lws);
}

WPCP_END_EXTERN_C
