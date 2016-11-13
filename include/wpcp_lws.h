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

struct wpcp_lws_t;

enum wpcp_lws_options_secure_t {
  WPCP_LWS_OPTIONS_SECURE_ON = 0,
  WPCP_LWS_OPTIONS_SECURE_OFF = 1,
  WPCP_LWS_OPTIONS_SECURE_ALLOW_SELF_SIGNED = 2
};

struct wpcp_lws_options_t {
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

  const char* server_string;

  uint16_t http_port;
  const char* http_interface;
  const char* http_rootdir;

  uint16_t https_port;
  const char* https_interface;
  const char* https_rootdir;
  const char* https_ca_filepath;
  const char* https_cert_filepath;
  const char* https_cipher_list;
  const char* https_private_key_filepath;
  const char* https_private_key_password;

  uint16_t rwpcp_port;
  const char* rwpcp_address;
  const char* rwpcp_authorization;
  const char* rwpcp_path;
  const char* rwpcp_host;
  const char* rwpcp_origin;
  const char* rwpcp_ca_filepath;
  const char* rwpcp_cert_filepath;
  const char* rwpcp_cipher_list;
  const char* rwpcp_private_key_filepath;
  const char* rwpcp_private_key_password;
  const char* rwpcp_proxy_address;
  uint16_t rwpcp_proxy_port;
  int rwpcp_reconnect_interval;
  enum wpcp_lws_options_secure_t rwpcp_secure;
};

WPCP_LWS_EXPORT void wpcp_lws_set_log_level(int level, void (*log_emit_function)(int level, const char *line));

WPCP_LWS_EXPORT struct wpcp_lws_t* wpcp_lws_create(const struct wpcp_lws_options_t* options);

WPCP_LWS_EXPORT struct wpcp_t* wpcp_lws_get_wpcp(struct wpcp_lws_t* wpcp_lws);

WPCP_LWS_EXPORT int wpcp_lws_service(struct wpcp_lws_t* wpcp_lws, int timeout_ms);

WPCP_LWS_EXPORT void wpcp_lws_cancel_service(struct wpcp_lws_t* wpcp_lws);

WPCP_LWS_EXPORT void wpcp_lws_lock(struct wpcp_lws_t* wpcp_lws);

WPCP_LWS_EXPORT void wpcp_lws_unlock(struct wpcp_lws_t* wpcp_lws);

WPCP_LWS_EXPORT void wpcp_lws_delete(struct wpcp_lws_t* wpcp_lws);

WPCP_END_EXTERN_C

#endif
