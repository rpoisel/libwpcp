#include "wpcp.h"

#include "wpcp_cbor.h"
#include "wpcp_util.h"
#include "wpcp_session.h"
#include <string.h>

WPCP_BEGIN_EXTERN_C

#define ITM(name) { #name, sizeof(#name) - 1, NULL }

static void wpcp_setup_message_handler(struct wpcp_internal_t* wpcp);

struct wpcp_t* wpcp_create(void)
{
  struct wpcp_internal_t* ret;
  ret = wpcp_calloc(1, sizeof(*ret));
  if (!ret)
    return NULL;
  return &ret->wpcp;
}

void wpcp_delete(struct wpcp_t* wpcp)
{
  wpcp_free(wpcp);
}

struct wpcp_session_t* wpcp_session_create(struct wpcp_t* wpcp, void* user)
{
  struct wpcp_session_t* ret;
  ret = wpcp_malloc(sizeof(*ret));
  if (!ret)
    return NULL;

  ret->wpcp = (struct wpcp_internal_t*) wpcp;
  ret->user = user;
  ret->next_free_publish_item = 1;
  ret->kill = false;
  ret->hello_finished = false;
  ret->publish_items_length = 0;
  ret->publish_items = NULL;
  ret->results = NULL;
  ret->publish_queue.first = NULL;
  ret->publish_queue.last = NULL;
  ret->queued_out_messages = &ret->publish_item;
  ret->publish_item.next = &ret->publish_item;
  ret->queued_out_message_insert_location = &ret->queued_out_messages;
  ret->free_publish_slots = UINT32_MAX;

  wpcp_setup_message_handler(ret->wpcp);

  return ret;
}

void wpcp_session_delete(struct wpcp_session_t* session)
{
  uint32_t i = 0;
  uint32_t remaining = 0;
  void* context = NULL;
  struct wpcp_publish_item_t** publish_items = session->publish_items;
  struct wpcp_publish_queue_item_t* publish_queue_item = session->publish_queue.first;
  struct wpcp_result_t* result = session->results;

  while (session->queued_out_messages != &session->publish_item) {
    struct wpcp_queued_out_message_t* queued_out_message = session->queued_out_messages;
    session->queued_out_messages = queued_out_message->next;
    wpcp_free(queued_out_message);
  }

  while (publish_queue_item) {
    struct wpcp_publish_queue_item_data_t* publish_queue_item_data = publish_queue_item->data;
    publish_queue_item = publish_queue_item->next;
    wpcp_publish_queue_item_deref(publish_queue_item_data);
  }

  while (result) {
    result->session = NULL;
    result = result->next;
  }

  for (i = 1; i < session->publish_items_length; ++i) {
    if (publish_items[i])
      ++remaining;
  }

  if (remaining) {
    void* user = session->user;
    uint32_t publish_items_length = session->publish_items_length;
    struct wpcp_result_t* dummy_unsubscribe_result = wpcp_convert_session_to_dummy_unsubscrible_result(session, remaining);

    for (i = 1; i < publish_items_length; ++i) {
      struct wpcp_publish_item_t* item = publish_items[i];
      struct wpcp_subscription_t* subscription;
      if (!item)
        continue;

      subscription = wpcp_retain_subscription(item->subscription);
      wpcp_publish_item_release(item);
      session->wpcp->wpcp.unsubscribe.ex_cb(user, dummy_unsubscribe_result, subscription, &context, --remaining);
    }
  } else
    wpcp_free(session);

  wpcp_free(publish_items);
}

bool wpcp_is_dummy_unsubscrible_result(struct wpcp_result_t* result)
{
  return !result->session;
}

struct wpcp_result_t* wpcp_convert_session_to_dummy_unsubscrible_result(struct wpcp_session_t* session, uint32_t count)
{
  struct wpcp_internal_t* wpcp = session->wpcp;

  struct wpcp_result_t* ret = (struct wpcp_result_t*) session;
  ret->session = NULL;
  ret->wpcp = wpcp;
  ret->remaining = count;
  ret->request_payload_count = 0;
  WPCP_ASSERT(count);
  WPCP_ASSERT(wpcp_is_dummy_unsubscrible_result(ret));
  return ret;
}

void wpcp_release_dummy_unsubscribe_result(struct wpcp_result_t* result)
{
  WPCP_ASSERT(wpcp_is_dummy_unsubscrible_result(result));
  if (--(result->remaining))
    return;
  wpcp_free(result);
}

static bool wpcp_session_handle_hello_message(struct wpcp_session_t* session, uint32_t request_id, uint32_t payload_count, struct wpcp_cbor_read_buffer_t* read_buffer)
{
  struct wpcp_key_value_pair_t map_items[] = { { "role", 4, NULL} };
  struct wpcp_key_value_pair_t* additonal;
  uint32_t additional_count;
  struct wpcp_cbor_write_buffer_t buf;

  if (!payload_count)
    return false;

  if (!wpcp_cbor_read_buffer_read_map_items(read_buffer, map_items, WPCP_COUNT_OF(map_items), &additonal, &additional_count))
    return false;

  wpcp_cbor_write_buffer_init(&buf);
  wpcp_cbor_write_buffer_write_map_header(&buf, 2);
  wpcp_cbor_write_buffer_write_text_string(&buf, "role", 4);
  wpcp_cbor_write_buffer_write_unsigned_integer(&buf, 1);

  uint32_t message_handler_count = session->wpcp->message_handler_count;
  wpcp_cbor_write_buffer_write_text_string(&buf, "methods", 7);
  wpcp_cbor_write_buffer_write_array_header(&buf, message_handler_count * 2);
  for (uint32_t i = 0; i < message_handler_count; ++i) {
    const struct wpcp_message_handler_t* message_handler = &session->wpcp->message_handler[i];
    wpcp_cbor_write_buffer_write_text_string(&buf, message_handler->name, message_handler->name_length);
    wpcp_cbor_write_buffer_write_unsigned_integer(&buf, message_handler->type);
  }

  wpcp_session_append_out_message(session, WPCP_MESSAGE_ID_RESULT, request_id, 1, &buf);

  session->hello_finished = true;

  wpcp_clear_map_items(map_items, WPCP_COUNT_OF(map_items), additonal, additional_count);

  return true;
}

static bool wpcp_session_handle_processed_message(struct wpcp_session_t* session, uint32_t request_id, uint32_t payload_count, struct wpcp_cbor_read_buffer_t* read_buffer)
{
  (void) session;
  (void) request_id;
  (void) read_buffer;
  return payload_count == 0;
}

static bool wpcp_session_handle_unsubscribe_message(struct wpcp_session_t* session, uint32_t request_id, uint32_t payload_count, struct wpcp_cbor_read_buffer_t* read_buffer)
{
  bool ret = true;
  void* context = NULL;
  struct wpcp_result_t* result = wpcp_request_result(session, request_id, payload_count);
  if (!result)
    return false;

  while (payload_count--) {
    struct wpcp_subscription_t* subscription = NULL;
    uint32_t subscription_id = 0;

    if (!wpcp_cbor_read_buffer_read_unsigned_integer(read_buffer, &subscription_id))
      ret = false;

    if (subscription_id < session->publish_items_length) {
      struct wpcp_publish_item_t* item = session->publish_items[subscription_id];

      if (item) {
        WPCP_ASSERT(item->subscriptionid == subscription_id);
        subscription = wpcp_retain_subscription(item->subscription);
        wpcp_session_release_publish_item(session, item);
      }
    }

    session->wpcp->wpcp.unsubscribe.ex_cb(session->user, result, subscription, &context, payload_count);
  }

  return ret;
}

void wpcp_return_unsubscribe(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, struct wpcp_subscription_t* subscription)
{
  if (subscription)
    wpcp_release_subscription(result->wpcp, subscription);

  if (!wpcp_is_dummy_unsubscrible_result(result)) {
    struct wpcp_value_t value;
    value.type = WPCP_VALUE_TYPE_UINT64;
    value.value.uint = subscription ? 1 : 0;
    wpcp_result_add_value(result, diagnostic_info, &value);
  } else
    wpcp_release_dummy_unsubscribe_result(result);
}

static const struct wpcp_key_value_pair_t wpcp_map_items_read_data[] = { ITM(id) };

static bool wpcp_session_handle_read_data_message_item(struct wpcp_parsed_in_message_item_t* item)
{
  item->wpcp->read_data.ex_cb(
    item->user,
    item->result,
    item->map[0].value,
    &item->context,
    item->remaining,
    item->additional,
    item->additional_count);
  return true;
}

void wpcp_return_read_data(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, const struct wpcp_value_t* value, double timestamp, uint32_t status, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  struct wpcp_cbor_write_buffer_t* write_buffer = &result->output_buffer;

  wpcp_result_add_diagnostic_info(result, diagnostic_info);
  wpcp_cbor_write_buffer_write_data_item(write_buffer, value, timestamp, status, additional, additional_count);
  wpcp_release_result(result);
}

static const struct wpcp_key_value_pair_t wpcp_map_items_browse[] = { ITM(id) };

static bool wpcp_session_handle_browse_message_item(struct wpcp_parsed_in_message_item_t* item)
{
  item->wpcp->browse.ex_cb(
    item->user,
    item->result,
    item->map[0].value,
    &item->context,
    item->remaining,
    item->additional,
    item->additional_count);
  return true;
}

void wpcp_return_browse(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, uint32_t count)
{
  wpcp_result_add_diagnostic_info(result, diagnostic_info);
  wpcp_request_subresult(result, count);
}

void wpcp_return_browse_item(struct wpcp_result_t* result, const struct wpcp_value_t* id, const char* name, uint32_t name_length, const char* title, uint32_t title_length, const char* description, uint32_t description_length, const struct wpcp_value_t* type, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  struct wpcp_cbor_write_buffer_t* write_buffer = &result->output_buffer;
  uint32_t count = (id ? 1 : 0) + (name ? 1 : 0) + (title ? 1 : 0) + (description ? 1 : 0) + (type ? 1 : 0) + additional_count;

  wpcp_cbor_write_buffer_write_map_header(write_buffer, count);

  if (id) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, "id", 2);
    wpcp_cbor_write_buffer_write_wpcp_value(write_buffer, id);
  }

  if (name) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, "name", 4);
    wpcp_cbor_write_buffer_write_text_string(write_buffer, name, name_length);
  }

  if (title) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, "title", 5);
    wpcp_cbor_write_buffer_write_text_string(write_buffer, title, title_length);
  }

  if (description) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, "description", 11);
    wpcp_cbor_write_buffer_write_text_string(write_buffer, description, description_length);
  }

  if (type) {
    wpcp_cbor_write_buffer_write_text_string(write_buffer, "type", 4);
    wpcp_cbor_write_buffer_write_wpcp_value(write_buffer, type);
  }

  wpcp_cbor_write_buffer_write_additional(write_buffer, additional, additional_count);

  wpcp_release_subresult(result);
}

static const struct wpcp_key_value_pair_t wpcp_map_items_handle_alarm[] = { ITM(token), ITM(acknowledged) };

static bool wpcp_session_handle_handle_alarm_message_item(struct wpcp_parsed_in_message_item_t* item)
{
  item->wpcp->handle_alarm.ex_cb(
    item->user,
    item->result,
    item->map[0].value,
    item->map[1].value,
    &item->context,
    item->remaining,
    item->additional,
    item->additional_count);
  return true;
}

static const struct wpcp_key_value_pair_t wpcp_map_items_read_history_data[] = { ITM(id), ITM(starttime), ITM(endtime), ITM(maxresults), ITM(aggregation), ITM(interval) };

static bool wpcp_session_handle_read_history_data_message_item(struct wpcp_parsed_in_message_item_t* item)
{
  item->wpcp->read_history_data.ex_cb(
    item->user,
    item->result,
    item->map[0].value,
    item->map[1].value,
    item->map[2].value,
    item->map[3].value,
    item->map[4].value,
    item->map[5].value,
    &item->context,
    item->remaining,
    item->additional,
    item->additional_count);
  return true;
}

void wpcp_return_read_history_data(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, uint32_t count)
{
  wpcp_result_add_diagnostic_info(result, diagnostic_info);
  wpcp_request_subresult(result, count);
}

void wpcp_return_read_history_data_item(struct wpcp_result_t* result, const struct wpcp_value_t* value, double timestamp, uint32_t status, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  wpcp_cbor_write_buffer_write_data_item(&result->output_buffer, value, timestamp, status, additional, additional_count);
  wpcp_release_subresult(result);
}

static const struct wpcp_key_value_pair_t wpcp_map_items_read_history_alarm[] = { ITM(id), ITM(starttime), ITM(endtime), ITM(maxresults), ITM(filter) };

static bool wpcp_session_handle_read_history_alarm_message_item(struct wpcp_parsed_in_message_item_t* item)
{
  item->wpcp->read_history_alarm.ex_cb(
    item->user,
    item->result,
    item->map[0].value,
    item->map[1].value,
    item->map[2].value,
    item->map[3].value,
    item->map[4].value,
    &item->context,
    item->remaining,
    item->additional,
    item->additional_count);
  return true;
}

void wpcp_return_read_history_alarm(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, uint32_t count)
{
  wpcp_result_add_diagnostic_info(result, diagnostic_info);
  wpcp_request_subresult(result, count);
}

void wpcp_return_read_history_alarm_item(struct wpcp_result_t* result, const char* key, uint32_t key_length, bool retain, const struct wpcp_value_t* token, const struct wpcp_value_t* id, double timestamp, uint32_t priority, const char* message, uint32_t message_length, bool acknowledged, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  wpcp_cbor_write_buffer_write_alarm_item(&result->output_buffer, key, key_length, retain, token, id, timestamp, priority, message, message_length, acknowledged, additional, additional_count);
  wpcp_release_subresult(result);
}

static const struct wpcp_key_value_pair_t wpcp_map_items_write_data[] = { ITM(id), ITM(value) };

static bool wpcp_session_handle_write_data_message_item(struct wpcp_parsed_in_message_item_t* item)
{
  item->wpcp->write_data.ex_cb(
    item->user,
    item->result,
    item->map[0].value,
    item->map[1].value,
    &item->context,
    item->remaining,
    item->additional,
    item->additional_count);
  return true;
}

void wpcp_return_write_data(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, bool success)
{
  struct wpcp_value_t value;
  value.type = success ? WPCP_VALUE_TYPE_TRUE : WPCP_VALUE_TYPE_FALSE;
  wpcp_result_add_value(result, diagnostic_info, &value);
}

void wpcp_return_handle_alarm(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, bool success)
{
  struct wpcp_value_t value;
  value.type = success ? WPCP_VALUE_TYPE_TRUE : WPCP_VALUE_TYPE_FALSE;
  wpcp_result_add_value(result, diagnostic_info, &value);
}

static const struct wpcp_key_value_pair_t wpcp_map_items_subscribe_data[] = { ITM(id) };

static bool wpcp_session_handle_subscribe_data_message_item(struct wpcp_parsed_in_message_item_t* item)
{
  item->wpcp->subscribe_data.ex_cb(
    item->user,
    item->result,
    item->subscription,
    item->map[0].value,
    &item->context,
    item->remaining,
    item->additional,
    item->additional_count);

  return true;
}

static const struct wpcp_key_value_pair_t wpcp_map_items_subscribe_alarm[] = { ITM(id), ITM(filter) };

static bool wpcp_session_handle_subscribe_alarm_message_item(struct wpcp_parsed_in_message_item_t* item)
{
  item->wpcp->subscribe_alarm.ex_cb(
    item->user,
    item->result,
    item->subscription,
    item->map[0].value,
    item->map[1].value,
    &item->context,
    item->remaining,
    item->additional,
    item->additional_count);

  return true;
}

static bool wpcp_return_subscribe_internal(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, struct wpcp_subscription_t* subscription)
{
  struct wpcp_publish_item_t* publish_item = NULL;
  void* context = NULL;
  struct wpcp_publish_handle_t* list;
  struct wpcp_t* wpcp = &result->wpcp->wpcp;
  struct wpcp_session_t* session = result->session;

  uint32_t id = wpcp_session_get_next_free_publish_id(session);
  if (id) {
    struct wpcp_publish_item_t** blocked_publish_item_target = wpcp_result_get_blocked_publish_item_target(result);

    if (blocked_publish_item_target) {
      publish_item = wpcp_session_request_publish_item(session, subscription, id);

      if (publish_item)
        *blocked_publish_item_target = publish_item;
      else
        id = 0;
    } else
      id = 0;
  }

  wpcp_result_add_unsigned_integer(result, diagnostic_info, id);

  if (!id)
    return false;

//  if (subscription->publish_handle.length > 1) {
  list = wpcp_malloc(sizeof(*list));
  if (!list)
    return false;

  list->length = 1;
  list->items = publish_item;
  wpcp->republish.ex_cb(session->user, list, subscription, &context, 0);

  return true;
}

struct wpcp_publish_handle_t* wpcp_return_subscribe_accept(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, struct wpcp_subscription_t* subscription)
{
  if (!wpcp_return_subscribe_internal(result, diagnostic_info, subscription))
    return NULL;
  return &subscription->publish_handle;
}

void wpcp_return_subscribe_alias(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, struct wpcp_subscription_t* subscription, struct wpcp_publish_handle_t* publish_handle)
{
  struct wpcp_subscription_t* other_subscription;
  WPCP_ASSERT(publish_handle->length);
  other_subscription = publish_handle->items->subscription;
  WPCP_ASSERT(other_subscription->type == subscription->type);
  wpcp_return_subscribe_internal(result, diagnostic_info, wpcp_retain_subscription(other_subscription));
  wpcp_release_subscription(result->session->wpcp, subscription);
}

void wpcp_return_subscribe_reject(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, struct wpcp_subscription_t* subscription)
{
  wpcp_result_add_unsigned_integer(result, diagnostic_info, 0);
  wpcp_release_subscription(result->session->wpcp, subscription);
}

void wpcp_return_republish(struct wpcp_publish_handle_t* publish_handle)
{
  wpcp_free(publish_handle);
}

static struct wpcp_cbor_write_buffer_t* wpcp_publish_general(struct wpcp_publish_handle_t* publish_handle)
{
  struct wpcp_cbor_write_buffer_t* ret;

  if (!publish_handle->length)
    return NULL;

  ret = wpcp_publish_handle_publish(publish_handle);
  if (!ret)
    publish_handle->items->session->kill = true;

  return ret;
}

void wpcp_publish_data(struct wpcp_publish_handle_t* publish_handle, const struct wpcp_value_t* value, double timestamp, uint32_t status, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  struct wpcp_cbor_write_buffer_t* write_buffer = wpcp_publish_general(publish_handle);
  if (write_buffer)
    wpcp_cbor_write_buffer_write_data_item(write_buffer, value, timestamp, status, additional, additional_count);
}

void wpcp_publish_alarm(struct wpcp_publish_handle_t* publish_handle, const char* key, uint32_t key_length, bool retain, const struct wpcp_value_t* token, const struct wpcp_value_t* id, double timestamp, uint32_t priority, const char* message, uint32_t message_length, bool acknowledged, const struct wpcp_key_value_pair_t* additional, uint32_t additional_count)
{
  struct wpcp_cbor_write_buffer_t* write_buffer = wpcp_publish_general(publish_handle);
  if (write_buffer)
    wpcp_cbor_write_buffer_write_alarm_item(write_buffer, key, key_length, retain, token, id, timestamp, priority, message, message_length, acknowledged, additional, additional_count);
}

bool wpcp_session_handle_in_message(struct wpcp_session_t* session, const void* data, size_t size)
{
  uint32_t array_count;
  uint32_t message_id;
  uint32_t request_id;
  struct wpcp_cbor_read_buffer_t read_buffer;
  struct wpcp_message_handler_t* message_handler;

  read_buffer.begin = data;
  read_buffer.end = read_buffer.begin + size;

  if (!wpcp_cbor_read_buffer_read_array_header(&read_buffer, &array_count))
    return false;

  if (array_count < 2)
    return false;

  array_count -= 2;

  if (!wpcp_cbor_read_buffer_read_unsigned_integer(&read_buffer, &message_id))
    return false;

  if (!wpcp_cbor_read_buffer_read_unsigned_integer(&read_buffer, &request_id))
    return false;

  if (!session->hello_finished)
    return wpcp_session_handle_hello_message(session, request_id, array_count, &read_buffer);

  if (message_id >= session->wpcp->message_handler_count)
    return false;

  message_handler = &session->wpcp->message_handler[message_id];

  if (message_handler->handle_function)
    return message_handler->handle_function(session, request_id, array_count, &read_buffer);

  WPCP_ASSERT(message_handler->handle_function_items);

  if (array_count) {
    struct wpcp_parsed_in_message_item_t item;
    struct wpcp_key_value_pair_t map_items[8];

    WPCP_ASSERT(message_handler->map_items_count <= WPCP_COUNT_OF(map_items));
    memcpy(map_items, message_handler->map_items, message_handler->map_items_count * sizeof(map_items[0]));

    item.wpcp = &session->wpcp->wpcp;
    item.session = session;
    item.user = session->user;
    item.context = NULL;
    item.remaining = array_count;
    item.map = map_items;
    item.map_count = message_handler->map_items_count;
    item.subscription = NULL;
    item.result = wpcp_request_result(session, request_id, item.remaining);
    if (item.result) {
      bool ret = true;

      while (item.remaining--) {
        const uint8_t* key = read_buffer.begin;

        if (!wpcp_cbor_read_buffer_read_map_items(&read_buffer, item.map, item.map_count, &item.additional, &item.additional_count))
          ret = false;

        WPCP_ASSERT(message_handler->type != WPCP_MESSAGE_TYPE_GENERAL);
        if (message_handler->type != WPCP_MESSAGE_TYPE_CALL) {
          item.subscription = wpcp_request_subscription(session->wpcp, message_id, key, read_buffer.begin - key);
          if (!item.subscription)
            ret = false;
        }

        message_handler->handle_function_items(&item);

        wpcp_clear_map_items(item.map, item.map_count, item.additional, item.additional_count);
      }

      if (!ret)
        return false;
    } else
      return false;
  } else {
    struct wpcp_cbor_write_buffer_t payload;
    wpcp_cbor_write_buffer_init(&payload);
    wpcp_session_append_out_message(session, WPCP_MESSAGE_ID_RESULT, request_id, 0, &payload);
  }

  return read_buffer.end == read_buffer.begin;
}

static void wpcp_setup_message_handler(struct wpcp_internal_t* wpcp)
{
  struct wpcp_message_handler_t* message_handler = wpcp->message_handler;
  if (wpcp->message_handler_count)
    return;

#define ADD_HANDLE_FUNCTION_INTERNAL(type_, name_, handle_function_, handle_function_items_, map_items_, map_items_count_) do { \
    message_handler->type = WPCP_MESSAGE_TYPE_##type_; \
    message_handler->name = name_; \
    message_handler->name_length = sizeof(name_) - 1; \
    message_handler->handle_function = handle_function_; \
    message_handler->handle_function_items = handle_function_items_; \
    message_handler->map_items = map_items_; \
    message_handler->map_items_count = map_items_count_; \
    message_handler++; \
  } while (false)

#define ADD_HANDLE_FUNCTION_GENERAL(name, id, handle_function) \
  WPCP_ASSERT((message_handler - wpcp->message_handler) == WPCP_MESSAGE_ID_##id); \
  ADD_HANDLE_FUNCTION_INTERNAL(GENERAL, name, handle_function, NULL, NULL, 0)

#define ADD_HANDLE_FUNCTION_CALL(name, id) \
  if (wpcp->wpcp.id.ex_cb) \
    ADD_HANDLE_FUNCTION_INTERNAL(CALL, name, wpcp_session_handle_##id##_message, NULL, NULL, 0)

#define ADD_HANDLE_FUNCTION(type, name, id) \
  if (wpcp->wpcp.id.ex_cb) \
    ADD_HANDLE_FUNCTION_INTERNAL(type, name, NULL, wpcp_session_handle_##id##_message_item, wpcp_map_items_##id, WPCP_COUNT_OF(wpcp_map_items_##id))

  ADD_HANDLE_FUNCTION_GENERAL("publish", PUBLISH, NULL);
  ADD_HANDLE_FUNCTION_GENERAL("processed", PROCESSED, wpcp_session_handle_processed_message);
  ADD_HANDLE_FUNCTION_GENERAL("result", RESULT, NULL);
  ADD_HANDLE_FUNCTION_CALL("unsubscribe", unsubscribe);
  ADD_HANDLE_FUNCTION(CALL, "readdata", read_data);
  ADD_HANDLE_FUNCTION(CALL, "browse", browse);
  ADD_HANDLE_FUNCTION(CALL, "writedata", write_data);
  ADD_HANDLE_FUNCTION(CALL, "handlealarm", handle_alarm);
  ADD_HANDLE_FUNCTION(CALL, "readhistorydata", read_history_data);
  ADD_HANDLE_FUNCTION(CALL, "readhistoryalarm", read_history_alarm);
  ADD_HANDLE_FUNCTION(SUBSCRIBE_STATE, "subscribedata", subscribe_data);
  ADD_HANDLE_FUNCTION(SUBSCRIBE_FILTER, "subscribealarm", subscribe_alarm);

  wpcp->message_handler_count = message_handler - wpcp->message_handler;
  WPCP_ASSERT(wpcp->message_handler_count <= WPCP_COUNT_OF(wpcp->message_handler));
}

WPCP_END_EXTERN_C
