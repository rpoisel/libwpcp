#ifndef WPCP_SESSION_H
#define WPCP_SESSION_H

#include "wpcp.h"
#include "wpcp_cbor.h"

struct wpcp_internal_t;

enum wpcp_message_type_t {
  WPCP_MESSAGE_TYPE_GENERAL = 0,
  WPCP_MESSAGE_TYPE_CALL = 1,
  WPCP_MESSAGE_TYPE_SUBSCRIBE_EVENT = 2,
  WPCP_MESSAGE_TYPE_SUBSCRIBE_STATE = 3,
  WPCP_MESSAGE_TYPE_SUBSCRIBE_FILTER = 4
};

enum wpcp_message_id_t {
  WPCP_MESSAGE_ID_PUBLISH,
  WPCP_MESSAGE_ID_PROCESSED,
  WPCP_MESSAGE_ID_RESULT
};

struct wpcp_publish_item_t {
  struct wpcp_publish_item_t* next;
  struct wpcp_publish_item_t* prev;
  struct wpcp_subscription_t* subscription;
  struct wpcp_session_t* session;
  uint32_t subscriptionid;
};

struct wpcp_publish_handle_t {
  size_t length;
  struct wpcp_publish_item_t* items;
};

struct wpcp_subscription_t {
  void* user;
  struct wpcp_publish_handle_t publish_handle;
  uint32_t reference_count;
  uint32_t type;
  size_t key_length;
  uint8_t key[1];
};

struct wpcp_queued_out_message_t {
  struct wpcp_out_message_t out_message;
  struct wpcp_queued_out_message_t* next;
};

struct wpcp_publish_queue_item_data_t {
  size_t references_inital;
  size_t references_actual;
  struct wpcp_cbor_write_buffer_t output_buffer;
};

struct wpcp_publish_queue_item_t {
  struct wpcp_publish_queue_item_t* next;
  uint32_t subscriptionid;
  struct wpcp_publish_queue_item_data_t* data;
};

struct wpcp_publish_queue_t {
  struct wpcp_publish_queue_item_t* first;
  struct wpcp_publish_queue_item_t* last;
};

struct wpcp_blocked_item_list_t {
  struct wpcp_publish_queue_t publish_queue;
  struct wpcp_publish_item_t* items[1];
};

struct wpcp_result_t {
  struct wpcp_internal_t* wpcp;
  struct wpcp_session_t* session;
  struct wpcp_result_t* next;
  struct wpcp_result_t* prev;
  struct wpcp_blocked_item_list_t* blocked_items;
  uint32_t request_id;
  uint32_t request_payload_count;
  uint32_t remaining;
  uint32_t remaining_subresults;
  struct wpcp_cbor_write_buffer_t output_buffer;
};

struct wpcp_session_t {
  struct wpcp_internal_t* wpcp;
  void* user;
  struct wpcp_queued_out_message_t* queued_out_messages;
  struct wpcp_queued_out_message_t** queued_out_message_insert_location;
  struct wpcp_result_t* results;
  struct wpcp_publish_queue_t publish_queue;
  struct wpcp_publish_item_t** publish_items;
  uint32_t publish_items_length;
  uint32_t next_free_publish_item;
  uint32_t free_publish_slots;
  bool hello_finished;
  bool kill;
  struct wpcp_queued_out_message_t publish_item;
};

struct wpcp_parsed_in_message_item_t {
  struct wpcp_t* wpcp;
  struct wpcp_session_t* session;
  void* user;
  struct wpcp_result_t* result;
  void* context;
  uint32_t remaining;
  struct wpcp_key_value_pair_t* map;
  uint32_t map_count;
  struct wpcp_key_value_pair_t* additional;
  uint32_t additional_count;
  struct wpcp_subscription_t* subscription;
};

struct wpcp_message_handler_t {
  uint32_t name_length;
  const char* name;
  enum wpcp_message_type_t type;
  uint32_t map_items_count;
  const struct wpcp_key_value_pair_t* map_items;
  bool (*handle_function)(struct wpcp_session_t* session, uint32_t request_id, uint32_t payload_count, struct wpcp_cbor_read_buffer_t* read_buffer);
  bool (*handle_function_items)(struct wpcp_parsed_in_message_item_t* item);
};

struct wpcp_internal_t {
  struct wpcp_t wpcp;
  size_t message_handler_count;
  struct wpcp_message_handler_t message_handler[16];
  size_t subscription_count;
  struct wpcp_subscription_t** subscriptions;
};




struct wpcp_subscription_t* wpcp_request_subscription(struct wpcp_internal_t* wpcp, uint32_t type, const uint8_t* key, size_t key_length);

struct wpcp_subscription_t* wpcp_retain_subscription(struct wpcp_subscription_t* subscription);

void wpcp_release_subscription(struct wpcp_internal_t* wpcp, struct wpcp_subscription_t* item);

uint32_t wpcp_session_get_next_free_publish_id(struct wpcp_session_t* session);

struct wpcp_publish_item_t* wpcp_session_request_publish_item(struct wpcp_session_t* session, struct wpcp_subscription_t* subscription, uint32_t id);

void wpcp_publish_item_release(struct wpcp_publish_item_t* item);

void wpcp_publish_queue_item_deref(struct wpcp_publish_queue_item_data_t* data);

void wpcp_session_release_publish_item(struct wpcp_session_t* session, struct wpcp_publish_item_t* item);

void wpcp_session_add_free_publish_slots(struct wpcp_session_t* session, uint32_t count);


void wpcp_session_append_out_message(struct wpcp_session_t* session, enum wpcp_message_id_t message_id, uint32_t request_id, uint32_t payload_count, struct wpcp_cbor_write_buffer_t* buffer);


struct wpcp_cbor_write_buffer_t* wpcp_publish_handle_publish(struct wpcp_publish_handle_t* publish_handle);


struct wpcp_result_t* wpcp_request_result(struct wpcp_session_t* session, uint32_t request_id, uint32_t payload_count);

struct wpcp_publish_item_t** wpcp_result_get_blocked_publish_item_target(struct wpcp_result_t* result);

void wpcp_result_add_diagnostic_info(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info);

void wpcp_result_add_unsigned_integer(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, uint32_t value);

void wpcp_result_add_value(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, const struct wpcp_value_t* value);

void wpcp_release_result(struct wpcp_result_t* result);

void wpcp_request_subresult(struct wpcp_result_t* result, uint32_t count);

void wpcp_release_subresult(struct wpcp_result_t* result);



bool wpcp_is_dummy_unsubscrible_result(struct wpcp_result_t* result);

struct wpcp_result_t* wpcp_convert_session_to_dummy_unsubscrible_result(struct wpcp_session_t* session, uint32_t count);

void wpcp_release_dummy_unsubscribe_result(struct wpcp_result_t* result);

#endif
