#include "wpcp_lws.h"

#include "wpcp_lws_main.h"
#include "wpcp_util.h"
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

#ifdef _WIN32
#ifndef snprintf
#define snprintf _snprintf
#endif
#else
#include <pthread.h>
#endif

WPCP_BEGIN_EXTERN_C

struct helper_t {
  struct lws* wsi;
  struct wpcp_lws_t* wpcp_lws;
  struct wpcp_session_t* session;
};

struct wpcp_lws_t {
  struct wpcp_t* wpcp;
  struct lws_context* context;
  struct lws_http_mount http_mount;
  struct lws_http_mount https_mount;
  struct lws_client_connect_info client_connect_info;
  struct lws* client;
  time_t next_client_connect;
  time_t client_reconnect_interval;
  const char* client_authorization_header;

  void* user;
  union wpcp_read_data_callback_t read_data;
  union wpcp_write_data_callback_t write_data;
  union wpcp_handle_alarm_callback_t handle_alarm;
  union wpcp_read_history_data_callback_t read_history_data;
  union wpcp_read_history_alarm_callback_t read_history_alarm;
  union wpcp_browse_callback_t browse;
  union wpcp_subscribe_data_callback_t subscribe_data;
  union wpcp_subscribe_alarm_callback_t subscribe_alarm;
  union wpcp_unsubscribe_callback_t unsubscribe;
  union wpcp_republish_callback_t republish;

#ifdef _WIN32
  CRITICAL_SECTION mutex;
#else
  pthread_mutex_t mutex;
#endif
};

WPCP_STATIC_INLINE struct wpcp_lws_t* wpcp_lws_wsi_context_user(struct lws *wsi)
{
  return lws_context_user(lws_get_context(wsi));
}

WPCP_STATIC_INLINE int ssl_option(enum wpcp_lws_options_secure_t value)
{
  if (value == WPCP_LWS_OPTIONS_SECURE_OFF)
    return 0;
  if (value == WPCP_LWS_OPTIONS_SECURE_ALLOW_SELF_SIGNED)
    return 2;
  return 1;
}

static void has_out_message_cb(void* user)
{
  struct helper_t* helper = user;
  lws_callback_on_writable(helper->wsi);
}

static void read_data_ex_cb(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  struct helper_t* helper = user;
  helper->wpcp_lws->read_data.ex_cb(helper->wpcp_lws->user, result, id, context, remaining, additional, additional_count);
}

static void write_data_ex_cb(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, const struct wpcp_value_t* value, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  struct helper_t* helper = user;
  helper->wpcp_lws->write_data.ex_cb(helper->wpcp_lws->user, result, id, value, context, remaining, additional, additional_count);
}

static void handle_alarm_ex_cb(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* token, const struct wpcp_value_t* acknowledge, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  struct helper_t* helper = user;
  helper->wpcp_lws->handle_alarm.ex_cb(helper->wpcp_lws->user, result, token, acknowledge, context, remaining, additional, additional_count);
}

static void read_history_data_ex_cb(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, const struct wpcp_value_t* starttime, const struct wpcp_value_t* endtime, const struct wpcp_value_t* maxresults, const struct wpcp_value_t* aggregation, const struct wpcp_value_t* interval, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  struct helper_t* helper = user;
  helper->wpcp_lws->read_history_data.ex_cb(helper->wpcp_lws->user, result, id, starttime, endtime, maxresults, aggregation, interval, context, remaining, additional, additional_count);
}

static void read_history_alarm_ex_cb(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, const struct wpcp_value_t* starttime, const struct wpcp_value_t* endtime, const struct wpcp_value_t* maxresults, const struct wpcp_value_t* filter, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  struct helper_t* helper = user;
  helper->wpcp_lws->read_history_alarm.ex_cb(helper->wpcp_lws->user, result, id, starttime, endtime, maxresults, filter, context, remaining, additional, additional_count);
}

static void browse_ex_cb(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  struct helper_t* helper = user;
  helper->wpcp_lws->browse.ex_cb(helper->wpcp_lws->user, result, id, context, remaining, additional, additional_count);
}

static void subscribe_data_ex_cb(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, const struct wpcp_value_t* id, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  struct helper_t* helper = user;
  helper->wpcp_lws->subscribe_data.ex_cb(helper->wpcp_lws->user, result, subscription, id, context, remaining, additional, additional_count);
}

static void subscribe_alarm_ex_cb(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, const struct wpcp_value_t* id, const struct wpcp_value_t* filter, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  struct helper_t* helper = user;
  helper->wpcp_lws->subscribe_alarm.ex_cb(helper->wpcp_lws->user, result, subscription, id, filter, context, remaining, additional, additional_count);
}

static void unsubscribe_ex_cb(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, void** context, uint32_t remaining)
{
  struct helper_t* helper = user;
  helper->wpcp_lws->unsubscribe.ex_cb(helper->wpcp_lws->user, result, subscription, context, remaining);
}

static void republish_ex_cb(void* user, struct wpcp_publish_handle_t* publish_handle, struct wpcp_subscription_t* subscription, void** context, uint32_t remaining)
{
  struct helper_t* helper = user;
  helper->wpcp_lws->republish.ex_cb(helper->wpcp_lws->user, publish_handle, subscription, context, remaining);
}

static int rwpcp_wpcp_calback(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len)
{
  struct helper_t* helper = user;

  switch (reason) {
  case LWS_CALLBACK_CLIENT_ESTABLISHED:
  case LWS_CALLBACK_ESTABLISHED:
    helper->wsi = wsi;
    helper->wpcp_lws = wpcp_lws_wsi_context_user(wsi);
    helper->session = wpcp_session_create(helper->wpcp_lws->wpcp, helper);
    break;

  case LWS_CALLBACK_CLIENT_WRITEABLE:
  case LWS_CALLBACK_SERVER_WRITEABLE:
    wpcp_lws_lock(helper->wpcp_lws);
    if (wpcp_session_has_out_message(helper->session)) {
      int written;
      size_t out_message_length;
      struct wpcp_out_message_t* out_message = wpcp_session_out_message_create(helper->session);
      wpcp_lws_unlock(helper->wpcp_lws);
      if (!out_message) {
        lwsl_err("ERROR creating WPCP message\n");
        return -1;
      }

      out_message_length = out_message->length;
      written = lws_write(wsi, out_message->data, out_message_length, LWS_WRITE_BINARY);
      wpcp_session_out_message_delete(out_message);
      if (written < (int)out_message_length) {
        lwsl_err("ERROR writing to di socket\n");
        return -1;
      }

      wpcp_lws_lock(helper->wpcp_lws);
      if (wpcp_session_has_out_message(helper->session))
        lws_callback_on_writable(wsi);
    }
    wpcp_lws_unlock(helper->wpcp_lws);
    break;

  case LWS_CALLBACK_CLIENT_RECEIVE:
  case LWS_CALLBACK_RECEIVE:
    wpcp_lws_lock(helper->wpcp_lws);
    if (!wpcp_session_handle_in_message(helper->session, in, len)) {
      wpcp_lws_unlock(helper->wpcp_lws);
      lwsl_err("ERROR handling WPCP message\n");
      return -1;
    }
    wpcp_lws_unlock(helper->wpcp_lws);
    break;

  case LWS_CALLBACK_CLOSED:
    wpcp_lws_lock(helper->wpcp_lws);
    wpcp_session_delete(helper->session);
    wpcp_lws_unlock(helper->wpcp_lws);
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

static int default_lws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
  struct wpcp_lws_t* wpcp_lws = wpcp_lws_wsi_context_user(wsi);
  (void) user;

  switch(reason) {
  case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
    if (wpcp_lws->client_authorization_header) {
      char** p = in;
      size_t cnt = snprintf(*p, len, "Authorization: %s\r\n", wpcp_lws->client_authorization_header);

      if (cnt >= len) {
        lwsl_err("ERROR adding Authorization header\n");
        return -1;
      }

      *p += cnt;
    }
    break;

  case LWS_CALLBACK_HTTP:
    return lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, NULL);

  case LWS_CALLBACK_WSI_DESTROY:
    if (wsi == wpcp_lws->client)
      wpcp_lws->client = NULL;
    break;

  default:
    break;
  }

  return 0;
}

static struct lws_protocols protocols[] = {
  {
    "", default_lws_callback,
    0, 0, 0, NULL
  }, {
    "wpcp", rwpcp_wpcp_calback,
    sizeof(struct helper_t),
    0, 0, NULL
  }, {
    "rwpcp", rwpcp_wpcp_calback,
    sizeof(struct helper_t),
    0, 0, NULL
  }, {
    NULL, NULL, 0, 0, 0, NULL
  }
};

void wpcp_lws_set_log_level(int level, void (*log_emit_function)(int level, const char *line))
{
  lws_set_log_level(level, log_emit_function);
}

struct wpcp_t* wpcp_lws_wpcp_create(void)
{
  struct wpcp_t* ret = wpcp_create();
  if (!ret)
    return NULL;
  ret->out_message_pre_padding = LWS_SEND_BUFFER_PRE_PADDING;
  ret->out_message_post_padding = LWS_SEND_BUFFER_POST_PADDING;
  ret->has_out_message.cb = has_out_message_cb;
  return ret;
}

struct wpcp_lws_t* wpcp_lws_create(const struct wpcp_lws_options_t* options)
{
  struct wpcp_lws_t* ret;
  struct lws_context_creation_info info;

  ret = wpcp_calloc(1, sizeof(*ret));
  if (!ret)
    return NULL;

  ret->wpcp = wpcp_create();
  if (!ret->wpcp) {
    wpcp_free(ret->wpcp);
    return NULL;
  }

#ifdef _WIN32
  InitializeCriticalSection(&ret->mutex);
#else
  pthread_mutex_init(&ret->mutex, NULL);
#endif

  memset(&info, 0, sizeof(info));
  info.user = ret;
  info.gid = -1;
  info.uid = -1;
  info.protocols = protocols;
  info.server_string = options->server_string ? options->server_string : "libwpcp";
  info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | LWS_SERVER_OPTION_EXPLICIT_VHOSTS;

  ret->context = lws_create_context(&info);
  ret->client_reconnect_interval = options->rwpcp_reconnect_interval ? options->rwpcp_reconnect_interval : 10;
  ret->client_authorization_header = options->rwpcp_authorization;
  ret->user = options->user;

  ret->wpcp->out_message_pre_padding = LWS_SEND_BUFFER_PRE_PADDING;
  ret->wpcp->out_message_post_padding = LWS_SEND_BUFFER_POST_PADDING;
  ret->wpcp->has_out_message.cb = has_out_message_cb;

#define SET_WPCP_CALLBACK(name) \
  if (options->name.ex_cb) { \
    ret->name.ex_cb = options->name.ex_cb; \
    ret->wpcp->name.ex_cb = name##_ex_cb; \
  }
  SET_WPCP_CALLBACK(read_data);
  SET_WPCP_CALLBACK(write_data);
  SET_WPCP_CALLBACK(handle_alarm);
  SET_WPCP_CALLBACK(read_history_data);
  SET_WPCP_CALLBACK(read_history_alarm);
  SET_WPCP_CALLBACK(browse);
  SET_WPCP_CALLBACK(subscribe_data);
  SET_WPCP_CALLBACK(subscribe_alarm);
  SET_WPCP_CALLBACK(unsubscribe);
  SET_WPCP_CALLBACK(republish);
#undef SET_WPCP_CALLBACK

  if (!ret->context) {
    lwsl_err("creating context failed\n");
    wpcp_lws_delete(ret);
    return NULL;
  }

  if (options->http_port) {
    ret->http_mount.mountpoint = "/";
    ret->http_mount.mountpoint_len = 1;
    ret->http_mount.origin_protocol = LWSMPRO_FILE;
    ret->http_mount.origin = options->http_rootdir;

    info.port = options->http_port;
    info.iface = options->http_interface;
    info.mounts = options->http_rootdir ? &ret->http_mount : NULL;

    lws_create_vhost(ret->context, &info);
  }

  if (options->https_port) {
    ret->https_mount.mountpoint = "/";
    ret->https_mount.mountpoint_len = 1;
    ret->https_mount.origin_protocol = LWSMPRO_FILE;
    ret->https_mount.origin = options->https_rootdir;

    info.port = options->https_port;
    info.iface = options->https_interface;
    info.mounts = options->https_rootdir ? &ret->https_mount : NULL;
    info.ssl_ca_filepath = options->https_ca_filepath;
    info.ssl_cert_filepath = options->https_cert_filepath;
    info.ssl_cipher_list = options->https_cipher_list;
    info.ssl_private_key_filepath = options->https_private_key_filepath;
    info.ssl_private_key_password = options->https_private_key_password;

    lws_create_vhost(ret->context, &info);
  }

  if (options->rwpcp_address) {
    int rwpcp_fallback_port = options->rwpcp_secure == WPCP_LWS_OPTIONS_SECURE_OFF ? 80 : 443;
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.iface = NULL;
    info.mounts = NULL;
    info.http_proxy_address = options->rwpcp_proxy_address;
    info.http_proxy_port = options->rwpcp_proxy_port;
    info.ssl_ca_filepath = options->rwpcp_ca_filepath;
    info.ssl_cert_filepath = options->rwpcp_cert_filepath;
    info.ssl_cipher_list = options->rwpcp_cipher_list;
    info.ssl_private_key_filepath = options->rwpcp_private_key_filepath;
    info.ssl_private_key_password = options->rwpcp_private_key_password;

    ret->client_connect_info.address = options->rwpcp_address;
    ret->client_connect_info.context = ret->context;
    ret->client_connect_info.host = options->rwpcp_host ? options->rwpcp_host : options->rwpcp_address;
    ret->client_connect_info.origin = options->rwpcp_origin;
    ret->client_connect_info.path = options->rwpcp_path ? options->rwpcp_path : "/";
    ret->client_connect_info.port = options->rwpcp_port ? options->rwpcp_port : rwpcp_fallback_port;
    ret->client_connect_info.protocol = protocols[2].name;
    ret->client_connect_info.ssl_connection = ssl_option(options->rwpcp_secure);
    ret->client_connect_info.vhost = lws_create_vhost(ret->context, &info);
  }

  return ret;
}

struct wpcp_t* wpcp_lws_get_wpcp(struct wpcp_lws_t* wpcp_lws)
{
  return wpcp_lws->wpcp;
}

int wpcp_lws_service(struct wpcp_lws_t* wpcp_lws, int timeout_ms)
{
  if (!wpcp_lws->client && wpcp_lws->client_connect_info.context) {
    time_t current_time = time(NULL);
    if (wpcp_lws->next_client_connect < current_time) {
      lwsl_notice("Connecting client to %s\n", wpcp_lws->client_connect_info.address);
      wpcp_lws->client = lws_client_connect_via_info(&wpcp_lws->client_connect_info);
      wpcp_lws->next_client_connect = current_time + wpcp_lws->client_reconnect_interval;
    }
  }

  return lws_service(wpcp_lws->context, timeout_ms);
}

void wpcp_lws_cancel_service(struct wpcp_lws_t* wpcp_lws)
{
  lws_cancel_service(wpcp_lws->context);
}


void wpcp_lws_lock(struct wpcp_lws_t* wpcp_lws)
{
#ifdef _WIN32
  EnterCriticalSection(&wpcp_lws->mutex);
#else
  pthread_mutex_lock(&wpcp_lws->mutex);
#endif
}

void wpcp_lws_unlock(struct wpcp_lws_t* wpcp_lws)
{
#ifdef _WIN32
  LeaveCriticalSection(&wpcp_lws->mutex);
#else
  pthread_mutex_unlock(&wpcp_lws->mutex);
#endif
}

void wpcp_lws_delete(struct wpcp_lws_t* wpcp_lws)
{
  if (!wpcp_lws)
    return;
  lws_context_destroy(wpcp_lws->context);
#ifdef _WIN32
  DeleteCriticalSection(&wpcp_lws->mutex);
#else
  pthread_mutex_destroy(&wpcp_lws->mutex);
#endif
  wpcp_delete(wpcp_lws->wpcp);
  wpcp_free(wpcp_lws);
}

WPCP_END_EXTERN_C
