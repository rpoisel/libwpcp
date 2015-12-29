#include "wpcp_lws.h"
#include <stdio.h>
#include <stdlib.h>

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
#include <string.h>

#ifdef _WIN32
#ifndef snprintf
#define snprintf _snprintf
#endif
#else
#include <pthread.h>
#endif

WPCP_BEGIN_EXTERN_C

static struct wpcp_lws_t g_lws;
static int arg_debug_level = 7;
static int arg_http_port = CONTEXT_PORT_NO_LISTEN;
static const char* arg_http_rootdir = ".";
static const char* arg_rwpcp_auth = NULL;
static const char* arg_rwpcp_host = NULL;
static int arg_rwpcp_port = 80;
static int arg_rwpcp_ssl = 0;
static const char* arg_rwpcp_path = "/";
static const char* arg_rwpcp_origin = NULL;
static int arg_rwpcp_interval = 10;


struct helper_t {
  struct lws* wsi;
  struct wpcp_session_t* session;
};

void has_out_message_cb(void* user)
{
  struct helper_t* helper = user;
  lws_callback_on_writable(helper->wsi);
}

static char uri_buffer[256];
char* uri_buffer_insert_location;

struct lws* g_client;
bool force_exit;


const char default_mimetype[] = "application/octet-stream";
const char* find_mimetype(const char* extension)
{
  if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
    return "text/html";
  if (!strcmp(extension, "js"))
    return "text/javascript";
  if (!strcmp(extension, "ico"))
    return "image/x-icon";
  return default_mimetype;
}

int callback_for_libwebsocket(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len)
{
  struct helper_t* helper = user;

  switch (reason) {
  case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
    if (arg_rwpcp_auth) {
      char** p = in;
      size_t cnt = snprintf(*p, len, "Authorization: %s\r\n", arg_rwpcp_auth);

      if (cnt >= len) {
        lwsl_err("ERROR adding Authorization header\n");
        return -1;
      }

      *p += cnt;
    }
    break;

  case LWS_CALLBACK_CLIENT_ESTABLISHED:
  case LWS_CALLBACK_ESTABLISHED:
    helper->wsi = wsi;
    helper->session = wpcp_session_create(g_lws.wpcp, helper);
    break;

  case LWS_CALLBACK_CLIENT_WRITEABLE:
  case LWS_CALLBACK_SERVER_WRITEABLE:
    wpcp_lws_lock();
    if (wpcp_session_has_out_message(helper->session)) {
      int written;
      struct wpcp_out_message_t* out_message = wpcp_session_out_message_create(helper->session);
      size_t out_message_length = out_message->length;
      wpcp_lws_unlock();
      if (!out_message) {
        lwsl_err("ERROR creating WPCP message\n");
        return -1;
      }

      written = lws_write(wsi, out_message->data, out_message_length, LWS_WRITE_BINARY);
      wpcp_session_out_message_delete(out_message);
      if (written < (int)out_message_length) {
        lwsl_err("ERROR %d writing to di socket\n", out_message->length);
        return -1;
      }

      wpcp_lws_lock();
      if (wpcp_session_has_out_message(helper->session))
        lws_callback_on_writable(wsi);
    }
    wpcp_lws_unlock();
    break;

  case LWS_CALLBACK_CLIENT_RECEIVE:
  case LWS_CALLBACK_RECEIVE:
    wpcp_lws_lock();
    if (!wpcp_session_handle_in_message(helper->session, in, len)) {
      wpcp_lws_unlock();
      lwsl_err("ERROR handling WPCP message\n");
      return -1;
    }
    wpcp_lws_unlock();
    break;

  case LWS_CALLBACK_WSI_DESTROY:
    if (helper) {
      wpcp_lws_lock();
      wpcp_session_delete(helper->session);
      wpcp_lws_unlock();
    }
    if (wsi == g_client)
      g_client = NULL;
    break;

  case LWS_CALLBACK_HTTP:
    {
      const char* p;
      const char* path = in;
      size_t max_path = (sizeof(uri_buffer) / sizeof(uri_buffer[0]) - (uri_buffer_insert_location - uri_buffer));
      const char* content_type = default_mimetype;

      lwsl_debug("serving HTTP URI %s\n", path);
      if (path[0] != '/') {
        lws_return_http_status(wsi, 403, "");
        break;
      }

      if (strstr(path, "/../")) {
        lws_return_http_status(wsi, 403, "");
        break;
      }

      if (len >= max_path) {
        lws_return_http_status(wsi, 403, "");
        break;
      }

      memcpy(uri_buffer_insert_location, in, len);
      uri_buffer_insert_location[len] = '\0';
      p = uri_buffer_insert_location + len - 1;
      while (*p != '.' && *p != '/')
        p -= 1;

      if (*p == '.')
        content_type = find_mimetype(p + 1);

      lws_serve_http_file(wsi, uri_buffer, content_type, NULL, 0);
      break;
    }
    break;

  case LWS_CALLBACK_GET_THREAD_ID:
#ifdef _WIN32
    return GetCurrentThreadId();
#elif defined(__APPLE__)
    return pthread_mach_thread_np(pthread_self());
#else
    return pthread_self();
#endif

  default:
    break;
  }

  return 0;
}

static const char* handle_argument(const char* key, const char* value, const char* original_error)
{
  if (!strcmp(key, "http.port")) {
    if (!value)
      return "no value sepcified";
    arg_http_port = atoi(value);
    if (!arg_http_port)
      return "invalid value for HTTP port";
    return NULL;
  }

  if (!strcmp(key, "http.rootdir")) {
    if (!value)
      return "no value sepcified";
    arg_http_rootdir = value;
    return NULL;
  }

  if (!strcmp(key, "debug.level")) {
    if (!value)
      return "no value sepcified";
    arg_debug_level = atoi(value);
    if (arg_debug_level > 7)
      return "value must be between 0 and 7";
    return NULL;
  }

  if (!strcmp(key, "rwpcp.auth")) {
    if (!value)
      return "no value sepcified";
    arg_rwpcp_auth = value;
    return NULL;
  }
  if (!strcmp(key, "rwpcp.host")) {
    if (!value)
      return "no value sepcified";
    arg_rwpcp_host = value;
    return NULL;
  }
  if (!strcmp(key, "rwpcp.port")) {
    if (!value)
      return "no value sepcified";
    arg_rwpcp_port = atoi(value);
    if (!arg_rwpcp_port)
      return "invalid value for HTTP port";
    return NULL;
  }

  if (!strcmp(key, "rwpcp.ssl")) {
    if (value)
      return "value sepcified";
    arg_rwpcp_ssl = 1;
    return NULL;
  }

  if (!strcmp(key, "rwpcp.path")) {
    if (!value)
      return "no value sepcified";
    arg_rwpcp_path = value;
    return NULL;
  }

  if (!strcmp(key, "rwpcp.origin")) {
    if (!value)
      return "no value sepcified";
    arg_rwpcp_origin = value;
    return NULL;
  }

  if (!strcmp(key, "rwpcp.interval")) {
    if (!value)
      return "no value sepcified";
    arg_rwpcp_interval = atoi(value);
    return NULL;
  }

  return original_error;
}


void sighandler(int sig)
{
  (void) sig;
  force_exit = true;
}

static struct lws_protocols protocols[] = {
  {
    "wpcp",
    callback_for_libwebsocket,
    sizeof(struct helper_t),
    0, 0, NULL
  },
  {
    "rwpcp",
    callback_for_libwebsocket,
    sizeof(struct helper_t),
    0, 0, NULL
  },
  { NULL, NULL, 0, 0, 0, NULL } /* terminator */
};

#ifdef _WIN32
static CRITICAL_SECTION g_mutex;
#else
static pthread_mutex_t g_mutex;
#endif

int wpcp_lws_main(int argc, char* argv[], wpcp_lws_init_cleanup_function_t init, wpcp_lws_init_cleanup_function_t cleanup)
{
  struct lws_context_creation_info info;
  struct lws_context* context;
  time_t next_client_connect = 0;
  char** args = argv + 1;
  int remaining = argc - 1;
  size_t root_dir_length;
  int n = 0;

  g_lws.wpcp = wpcp_create();
  g_lws.argc = argc;
  g_lws.argv = argv;

#ifdef _WIN32
  InitializeCriticalSection(&g_mutex);
#else
  pthread_mutex_init(&g_mutex, NULL);
#endif

#ifndef _WIN32_WCE
  signal(SIGINT, sighandler);
#endif

  init(&g_lws);

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

    if (g_lws.handle_argument)
      argument_error = g_lws.handle_argument(key, value);

    if (argument_error)
      argument_error = handle_argument(key, value, argument_error);

    if (argument_error) {
      lwsl_err("invalid argument '%s'%s%s%s (%s)\n", key, value ? " = '" : "", value ? value : "", value ? "'" : "", argument_error);
      return -1;
    }
  }

  if (arg_http_port == CONTEXT_PORT_NO_LISTEN && !arg_rwpcp_host) {
    lwsl_err("no port and/or remote host given\n");
    return -1;
  }

  lws_set_log_level(arg_debug_level, NULL);

  memset(&info, 0, sizeof info);
  info.port = arg_http_port;
  info.gid = -1;
  info.options = 0;
  info.ssl_cert_filepath = NULL;
  info.ssl_private_key_filepath = NULL;
  info.uid = -1;
  info.protocols = protocols;

  root_dir_length = strlen(arg_http_rootdir);
  memcpy(uri_buffer, arg_http_rootdir, root_dir_length);
  uri_buffer_insert_location = uri_buffer + 1;

  g_lws.wpcp->out_message_pre_padding = LWS_SEND_BUFFER_PRE_PADDING;
  g_lws.wpcp->out_message_post_padding = LWS_SEND_BUFFER_POST_PADDING;
  g_lws.wpcp->has_out_message.cb = has_out_message_cb;

  context = lws_create_context(&info);
  if (context == NULL) {
    lwsl_err("creating context failed\n");
    return -1;
  }

  if (g_lws.start)
    g_lws.start();

  while (n >= 0 && !force_exit) {
    if (!g_client && arg_rwpcp_host) {
      time_t current_time = time(NULL);
      if (next_client_connect < current_time) {
        lwsl_debug("Connecting client to %s\n", arg_rwpcp_host);
        g_client = lws_client_connect(context, arg_rwpcp_host, arg_rwpcp_port, arg_rwpcp_ssl, arg_rwpcp_path, arg_rwpcp_host, arg_rwpcp_origin, protocols[1].name, -1);
        next_client_connect = current_time + arg_rwpcp_interval;
      }
    }
    n = lws_service(context, 500);
  }

  if (g_lws.stop)
    g_lws.stop();

  lws_context_destroy(context);
  cleanup(&g_lws);

#ifdef _WIN32
  DeleteCriticalSection(&g_mutex);
#else
  pthread_mutex_destroy(&g_mutex);
#endif

  wpcp_delete(g_lws.wpcp);

  return 0;
}

void wpcp_lws_lock(void)
{
#ifdef _WIN32
  EnterCriticalSection(&g_mutex);
#else
  pthread_mutex_lock(&g_mutex);
#endif
}

void wpcp_lws_unlock(void)
{
#ifdef _WIN32
  LeaveCriticalSection(&g_mutex);
#else
  pthread_mutex_unlock(&g_mutex);
#endif
}

WPCP_END_EXTERN_C
