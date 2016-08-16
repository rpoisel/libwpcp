#ifndef WPCP_H
#define WPCP_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
#define WPCP_BEGIN_EXTERN_C extern "C" {
#define WPCP_END_EXTERN_C }
#else
#define WPCP_BEGIN_EXTERN_C
#define WPCP_END_EXTERN_C
#endif

#if defined(_WIN32)
#define WPCP_EXPORT_DECLARATION __declspec(dllexport)
#define WPCP_IMPORT_DECLARATION __declspec(dllimport)
#elif defined(__GNUC__)
#define WPCP_EXPORT_DECLARATION __attribute__((visibility("default")))
#define WPCP_IMPORT_DECLARATION WPCP_EXPORT_DECLARATION
#else
#define WPCP_EXPORT_DECLARATION
#define WPCP_IMPORT_DECLARATION
#endif

#if defined(WPCP_STATIC)
#define WPCP_EXPORT
#elif defined(WPCP_EXPORTS)
#define WPCP_EXPORT WPCP_EXPORT_DECLARATION
#else
#define WPCP_EXPORT WPCP_IMPORT_DECLARATION
#endif

#if defined(_MSC_VER)
#define WPCP_CDECL __cdecl
#else
#define WPCP_CDECL
#endif

WPCP_BEGIN_EXTERN_C

struct wpcp_t;
struct wpcp_publish_handle_t;
struct wpcp_result_t;
struct wpcp_session_t;
struct wpcp_subscription_t;

enum wpcp_value_type_t {
  WPCP_VALUE_TYPE_FALSE = 20,
  WPCP_VALUE_TYPE_TRUE = 21,
  WPCP_VALUE_TYPE_NULL = 22,
  WPCP_VALUE_TYPE_UNDEFINED = 23,
  WPCP_VALUE_TYPE_SIMPLE_VALUE = 24,
  WPCP_VALUE_TYPE_FLOAT = 26,
  WPCP_VALUE_TYPE_DOUBLE = 27,
  WPCP_VALUE_TYPE_UINT64 = 256,
  WPCP_VALUE_TYPE_INT64,
  WPCP_VALUE_TYPE_BYTE_STRING,
  WPCP_VALUE_TYPE_TEXT_STRING,
  WPCP_VALUE_TYPE_ARRAY,
  WPCP_VALUE_TYPE_MAP,
  WPCP_VALUE_TYPE_TAG
};

struct wpcp_value_t {
  enum wpcp_value_type_t type;

  union {
    uint64_t uint;
    int64_t sint;
    float flt;
    double dbl;
    size_t length;
  } value;

  union {
    const void* byte_string;
    const char* text_string;
    struct wpcp_value_t* first_child;
  } data;

  struct wpcp_value_t* next;
};

struct wpcp_key_value_pair_t {
  const char* key;
  uint32_t key_length;
  struct wpcp_value_t* value;
};

typedef void (WPCP_CDECL *wpcp_has_out_message_cb_t)(void* user);
typedef void(WPCP_CDECL *wpcp_read_data_cb_t)(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id);
typedef void(WPCP_CDECL *wpcp_read_data_ex_cb_t)(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);
typedef void(WPCP_CDECL *wpcp_write_data_cb_t)(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, const struct wpcp_value_t* value);
typedef void(WPCP_CDECL *wpcp_write_data_ex_cb_t)(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, const struct wpcp_value_t* value, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);
typedef void(WPCP_CDECL *wpcp_handle_alarm_cb_t)(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* token, const struct wpcp_value_t* acknowledge);
typedef void(WPCP_CDECL *wpcp_handle_alarm_ex_cb_t)(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* token, const struct wpcp_value_t* acknowledge, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);
typedef void(WPCP_CDECL *wpcp_read_history_data_cb_t)(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, const struct wpcp_value_t* starttime, const struct wpcp_value_t* endtime, const struct wpcp_value_t* maxresults, const struct wpcp_value_t* aggregation, const struct wpcp_value_t* interval);
typedef void(WPCP_CDECL *wpcp_read_history_data_ex_cb_t)(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, const struct wpcp_value_t* starttime, const struct wpcp_value_t* endtime, const struct wpcp_value_t* maxresults, const struct wpcp_value_t* aggregation, const struct wpcp_value_t* interval, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);
typedef void(WPCP_CDECL *wpcp_read_history_alarm_cb_t)(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, const struct wpcp_value_t* starttime, const struct wpcp_value_t* endtime, const struct wpcp_value_t* maxresults, const struct wpcp_value_t* filter);
typedef void(WPCP_CDECL *wpcp_read_history_alarm_ex_cb_t)(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, const struct wpcp_value_t* starttime, const struct wpcp_value_t* endtime, const struct wpcp_value_t* maxresults, const struct wpcp_value_t* filter, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);
typedef void(WPCP_CDECL *wpcp_browse_cb_t)(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id);
typedef void(WPCP_CDECL *wpcp_browse_ex_cb_t)(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);
typedef void(WPCP_CDECL *wpcp_subscribe_data_cb_t)(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, const struct wpcp_value_t* id);
typedef void(WPCP_CDECL *wpcp_subscribe_data_ex_cb_t)(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, const struct wpcp_value_t* id, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);
typedef void(WPCP_CDECL *wpcp_subscribe_alarm_cb_t)(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, const struct wpcp_value_t* id, const struct wpcp_value_t* filter);
typedef void(WPCP_CDECL *wpcp_subscribe_alarm_ex_cb_t)(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, const struct wpcp_value_t* id, const struct wpcp_value_t* filter, void** context, uint32_t remaining, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);
typedef void(WPCP_CDECL *wpcp_unsubscribe_cb_t)(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription);
typedef void(WPCP_CDECL *wpcp_unsubscribe_ex_cb_t)(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, void** context, uint32_t remaining);
typedef void(WPCP_CDECL *wpcp_republish_cb_t)(void* user, struct wpcp_publish_handle_t* publish_handle, struct wpcp_subscription_t* subscription);
typedef void(WPCP_CDECL *wpcp_republish_ex_cb_t)(void* user, struct wpcp_publish_handle_t* publish_handle, struct wpcp_subscription_t* subscription, void** context, uint32_t remaining);

union wpcp_has_out_message_callback_t {
  wpcp_has_out_message_cb_t cb;
};
union wpcp_read_data_callback_t {
  wpcp_read_data_cb_t cb;
  wpcp_read_data_ex_cb_t ex_cb;
};
union wpcp_write_data_callback_t {
  wpcp_write_data_cb_t cb;
  wpcp_write_data_ex_cb_t ex_cb;
};
union wpcp_handle_alarm_callback_t {
  wpcp_handle_alarm_cb_t cb;
  wpcp_handle_alarm_ex_cb_t ex_cb;
};
union wpcp_read_history_data_callback_t {
  wpcp_read_history_data_cb_t cb;
  wpcp_read_history_data_ex_cb_t ex_cb;
};
union wpcp_read_history_alarm_callback_t {
  wpcp_read_history_alarm_cb_t cb;
  wpcp_read_history_alarm_ex_cb_t ex_cb;
};
union wpcp_browse_callback_t {
  wpcp_browse_cb_t cb;
  wpcp_browse_ex_cb_t ex_cb;
};
union wpcp_subscribe_data_callback_t {
  wpcp_subscribe_data_cb_t cb;
  wpcp_subscribe_data_ex_cb_t ex_cb;
};
union wpcp_subscribe_alarm_callback_t {
  wpcp_subscribe_alarm_cb_t cb;
  wpcp_subscribe_alarm_ex_cb_t ex_cb;
};
union wpcp_unsubscribe_callback_t {
  wpcp_unsubscribe_cb_t cb;
  wpcp_unsubscribe_ex_cb_t ex_cb;
};
union wpcp_republish_callback_t {
  wpcp_republish_cb_t cb;
  wpcp_republish_ex_cb_t ex_cb;
};


struct wpcp_t {
  size_t out_message_pre_padding;
  size_t out_message_post_padding;
  union wpcp_has_out_message_callback_t has_out_message;
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
};

struct wpcp_out_message_t {
  size_t length;
  void* data;
};


WPCP_EXPORT struct wpcp_t* wpcp_create(void);

WPCP_EXPORT void wpcp_delete(struct wpcp_t* wpcp);

WPCP_EXPORT struct wpcp_session_t* wpcp_session_create(struct wpcp_t* wpcp, void* user);

WPCP_EXPORT void wpcp_session_delete(struct wpcp_session_t* session);

WPCP_EXPORT bool wpcp_session_handle_in_message(struct wpcp_session_t* session, const void* data, size_t size);

WPCP_EXPORT bool wpcp_session_has_out_message(struct wpcp_session_t* session);

WPCP_EXPORT struct wpcp_out_message_t* wpcp_session_out_message_create(struct wpcp_session_t* session);

WPCP_EXPORT void wpcp_session_out_message_delete(struct wpcp_out_message_t* out_message);

WPCP_EXPORT uint32_t wpcp_subscription_get_type(struct wpcp_subscription_t* subscription);

WPCP_EXPORT void* wpcp_subscription_get_user(struct wpcp_subscription_t* subscription);

WPCP_EXPORT void wpcp_subscription_set_user(struct wpcp_subscription_t* subscription, void* user);

WPCP_EXPORT size_t wpcp_subscription_get_usage_count(struct wpcp_subscription_t* subscription);

WPCP_EXPORT void wpcp_return_read_data(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, const struct wpcp_value_t* value, double timestamp, uint32_t status, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);

WPCP_EXPORT void wpcp_return_write_data(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, bool success);

WPCP_EXPORT void wpcp_return_handle_alarm(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, bool success);

WPCP_EXPORT void wpcp_return_read_history_data(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, uint32_t count);

WPCP_EXPORT void wpcp_return_read_history_data_item(struct wpcp_result_t* result, const struct wpcp_value_t* value, double timestamp, uint32_t status, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);

WPCP_EXPORT void wpcp_return_read_history_alarm(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, uint32_t count);

WPCP_EXPORT void wpcp_return_read_history_alarm_item(struct wpcp_result_t* result, const char* key, uint32_t key_length, bool retain, const struct wpcp_value_t* token, const struct wpcp_value_t* id, double timestamp, uint32_t priority, const char* message, uint32_t message_length, bool acknowledged, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);

WPCP_EXPORT void wpcp_return_browse(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, uint32_t count);

WPCP_EXPORT void wpcp_return_browse_item(struct wpcp_result_t* result, const struct wpcp_value_t* id, const char* name, uint32_t name_length, const char* title, uint32_t title_length, const char* description, uint32_t description_length, const struct wpcp_value_t* type, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);

WPCP_EXPORT void wpcp_publish_data(struct wpcp_publish_handle_t* publish_handle, const struct wpcp_value_t* value, double timestamp, uint32_t status, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);

WPCP_EXPORT void wpcp_publish_alarm(struct wpcp_publish_handle_t* publish_handle, const char* key, uint32_t key_length, bool retain, const struct wpcp_value_t* token, const struct wpcp_value_t* id, double timestamp, uint32_t priority, const char* message, uint32_t message_length, bool acknowledged, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count);

WPCP_EXPORT struct wpcp_publish_handle_t* wpcp_return_subscribe_accept(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, struct wpcp_subscription_t* subscription);

WPCP_EXPORT void wpcp_return_subscribe_alias(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, struct wpcp_subscription_t* subscription, struct wpcp_publish_handle_t* publish_handle);

WPCP_EXPORT void wpcp_return_subscribe_reject(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, struct wpcp_subscription_t* subscription);

WPCP_EXPORT void wpcp_return_unsubscribe(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, struct wpcp_subscription_t* subscription);

WPCP_EXPORT void wpcp_return_republish(struct wpcp_publish_handle_t* publish_handle);

WPCP_END_EXTERN_C

#endif
