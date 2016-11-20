#include "wpcp_session.h"

#include "wpcp_cbor.h"
#include "wpcp_util.h"
#include <string.h>

WPCP_BEGIN_EXTERN_C

WPCP_STATIC_INLINE int wpcp_subscription_key_compare(uint32_t type, const uint8_t* key, size_t key_length, const struct wpcp_subscription_t* item)
{
  if (type < item->type)
    return -1;
  if (type > item->type)
    return 1;
  if (key_length < item->key_length)
    return -1;
  if (key_length > item->key_length)
    return 1;
  return memcmp(key, item->key, key_length);
}

WPCP_STATIC_INLINE void wpcp_subscriptions_memmove(struct wpcp_internal_t* wpcp, size_t dest, size_t src, size_t count)
{
  memmove(wpcp->subscriptions + dest, wpcp->subscriptions + src, count * sizeof(wpcp->subscriptions[0]));
}

WPCP_STATIC_INLINE bool wpcp_subscriptions_realloc(struct wpcp_internal_t* wpcp, ptrdiff_t diff)
{
  size_t count = wpcp->subscription_count + diff;
  void* new_items = wpcp_realloc(wpcp->subscriptions, count * sizeof(wpcp->subscriptions[0]));
  if (!new_items && count)
    return false;
  wpcp->subscriptions = new_items;
  return true;
}

struct wpcp_subscription_t* wpcp_retain_subscription(struct wpcp_subscription_t* subscription)
{
  subscription->reference_count += 1;
  return subscription;
}

struct wpcp_subscription_t* wpcp_request_subscription(struct wpcp_internal_t* wpcp, uint32_t type, const uint8_t* key, size_t key_length)
{
  size_t low = 0;
  size_t high = wpcp->subscription_count;
  struct wpcp_subscription_t* ret;

  while (low != high) {
    size_t pos = (low + high) / 2;
    int cmp = wpcp_subscription_key_compare(type, key, key_length, wpcp->subscriptions[pos]);
    if (!cmp)
      return wpcp_retain_subscription(wpcp->subscriptions[pos]);
    if (cmp > 0)
      low = pos + 1;
    else
      high = pos;
  }

  if (!wpcp_subscriptions_realloc(wpcp, 1))
    return NULL;

  ret = wpcp_malloc(sizeof(*ret) - sizeof(ret->key) + key_length * sizeof(key[0]));
  if (!ret)
    return NULL;

  ret->user = NULL;
  ret->reference_count = 1;
  ret->type = type;
  ret->publish_handle.length = 0;
  ret->publish_handle.items = NULL;
  ret->key_length = key_length;
  memcpy(ret->key, key, key_length);

  wpcp_subscriptions_memmove(wpcp, low + 1, low, wpcp->subscription_count - low);
  wpcp->subscriptions[low] = ret;
  wpcp->subscription_count += 1;

  return ret;
}

void wpcp_release_subscription(struct wpcp_internal_t* wpcp, struct wpcp_subscription_t* subscription)
{
  size_t low = 0;
  size_t high = wpcp->subscription_count;

  subscription->reference_count -= 1;
  if (subscription->reference_count)
    return;

  for (;;) {
    size_t pos = (low + high) / 2;
    int cmp = wpcp_subscription_key_compare(subscription->type, subscription->key, subscription->key_length, wpcp->subscriptions[pos]);
    if (!cmp) {
      wpcp->subscription_count -= 1;
      wpcp_subscriptions_memmove(wpcp, pos, pos + 1, wpcp->subscription_count - pos);
      wpcp_subscriptions_realloc(wpcp, 0);
      wpcp_free(subscription);
      return;
    }
    if (cmp > 0)
      low = pos + 1;
    else
      high = pos;
  }
}

uint32_t wpcp_subscription_get_type(struct wpcp_subscription_t* subscription)
{
  return subscription->type;
}

void* wpcp_subscription_get_user(struct wpcp_subscription_t* subscription)
{
  return subscription->user;
}

void wpcp_subscription_set_user(struct wpcp_subscription_t* subscription, void* user)
{
  subscription->user = user;
}

size_t wpcp_subscription_get_usage_count(struct wpcp_subscription_t* subscription)
{
  return subscription->publish_handle.length;
}

WPCP_STATIC_INLINE bool wpcp_session_has_sendable_publish_item(struct wpcp_session_t* session)
{
  return !!session->publish_queue.first && !!session->free_publish_slots;
}

void wpcp_session_add_free_publish_slots(struct wpcp_session_t* session, uint32_t count)
{
  session->free_publish_slots += count;
  if (session->free_publish_slots == count && wpcp_session_has_sendable_publish_item(session))
    session->wpcp->wpcp.has_out_message.cb(session->user);
}

uint32_t wpcp_session_get_next_free_publish_id(struct wpcp_session_t* session)
{
  uint32_t publish_items_length = session->publish_items_length;
  uint32_t id = session->next_free_publish_item;
  struct wpcp_publish_item_t** publish_items = session->publish_items;
  const uint32_t additional_items = 16;
  const uint32_t new_publish_items_length = publish_items_length + additional_items;
  struct wpcp_publish_item_t** new_publish_items;

  while (id < publish_items_length) {
    if (!publish_items[id])
      return id;
    ++id;
  }

  new_publish_items = wpcp_realloc(publish_items, new_publish_items_length * sizeof(publish_items[0]));
  if (!new_publish_items)
    return 0;

  memset(new_publish_items + publish_items_length, 0, additional_items * sizeof(publish_items[0]));
  session->publish_items = new_publish_items;
  session->publish_items_length = new_publish_items_length;
  return id;
}

struct wpcp_publish_item_t* wpcp_session_request_publish_item(struct wpcp_session_t* session, struct wpcp_subscription_t* subscription, uint32_t id)
{
  struct wpcp_publish_item_t* ret;

  ret = wpcp_malloc(sizeof(*ret));
  if (!ret) {
    wpcp_release_subscription(session->wpcp, subscription);
    return NULL;
  }

  ret->subscription = subscription;
  ret->session = session;
  ret->subscriptionid = id;
  ret->prev = NULL;
  ret->next = subscription->publish_handle.items;
  if (ret->next)
    ret->next->prev = ret;
  subscription->publish_handle.length += 1;
  subscription->publish_handle.items = ret;
  session->publish_items[id] = ret;
  session->next_free_publish_item = id + 1;

  return ret;
}

void wpcp_publish_item_release(struct wpcp_publish_item_t* item)
{
  struct wpcp_internal_t* wpcp = item->session->wpcp;
  struct wpcp_subscription_t* subscription = item->subscription;
  subscription->publish_handle.length -= 1;

  if (item->prev)
    item->prev->next = item->next;
  else {
    WPCP_ASSERT(subscription->publish_handle.items == item);
    subscription->publish_handle.items = item->next;
  }
  if (item->next)
    item->next->prev = item->prev;

  wpcp_free(item);
  wpcp_release_subscription(wpcp, subscription);
}

void wpcp_session_release_publish_item(struct wpcp_session_t* session, struct wpcp_publish_item_t* item)
{
  session->publish_items[item->subscriptionid] = NULL;
  if (item->subscriptionid < session->next_free_publish_item)
    session->next_free_publish_item = item->subscriptionid;
  wpcp_publish_item_release(item);
}

void* wpcp_out_message_alloc(struct wpcp_t* wpcp, size_t header_size, enum wpcp_message_id_t message_id, uint32_t request_id, uint32_t payload_count, size_t content_size, uint8_t** target)
{
  size_t before = wpcp->out_message_pre_padding;
  size_t after = wpcp->out_message_post_padding;
  size_t max_message_header_size = 1 + sizeof(message_id) + 1 + sizeof(request_id) + 1 + sizeof(payload_count);
  struct wpcp_out_message_t* ret;
  WPCP_ASSERT(header_size >= sizeof(*ret));
  ret = wpcp_malloc(header_size + max_message_header_size + before + content_size + after);
  if (ret) {
    uint8_t* begin = ((uint8_t*)ret) + header_size + before;
    uint8_t* dest = begin;
    dest = wpcp_cbor_write_array_header(dest, 2 + payload_count);
    dest = wpcp_cbor_write_unsigned_integer(dest, message_id);
    dest = wpcp_cbor_write_unsigned_integer(dest, request_id);

    *target = dest;
    ret->data = begin;
    ret->length = (dest - begin) + content_size;
  }
  return ret;
}

void wpcp_session_append_out_message(struct wpcp_session_t* session, enum wpcp_message_id_t message_id, uint32_t request_id, uint32_t payload_count, struct wpcp_cbor_write_buffer_t* buffer)
{
  struct wpcp_queued_out_message_t* ret;
  size_t header_size = sizeof(*ret);
  uint8_t* dest;

  ret = wpcp_out_message_alloc(&session->wpcp->wpcp, header_size, message_id, request_id, payload_count, buffer->used_size, &dest);
  if (ret) {
    wpcp_cbor_write_write_buffer(dest, buffer);
    wpcp_cbor_write_buffer_clear(buffer);

    ret->next = &session->publish_item;
    *(session->queued_out_message_insert_location) = ret;
    session->queued_out_message_insert_location = &ret->next;
  } else
    session->kill = true;

  session->wpcp->wpcp.has_out_message.cb(session->user);
}

bool wpcp_session_has_out_message(struct wpcp_session_t* session)
{
  if (session->queued_out_messages != &session->publish_item)
    return true;
  return wpcp_session_has_sendable_publish_item(session) || session->kill;
}

void wpcp_publish_queue_item_deref(struct wpcp_publish_queue_item_data_t* buffer)
{
  if (--(buffer->references_actual))
    return;

  wpcp_cbor_write_buffer_clear(&buffer->output_buffer);
  wpcp_free(((struct wpcp_publish_queue_item_t*) buffer) - buffer->references_inital);
}

struct wpcp_out_message_t* wpcp_session_out_message_create(struct wpcp_session_t* session)
{
  struct wpcp_queued_out_message_t* first_queued_out_message = session->queued_out_messages;
  session->queued_out_messages = first_queued_out_message->next;

  if (session->kill)
    return NULL;

  if (first_queued_out_message != &session->publish_item) {
    if (session->queued_out_messages == &session->publish_item) {
      if (!session->publish_queue.first)
        session->queued_out_message_insert_location = &session->queued_out_messages;
    }

    return &first_queued_out_message->out_message;
  } else {

  struct wpcp_out_message_t* ret;

  const size_t max_size_of_cbor_uint32 = 1 + sizeof(uint32_t);
  uint32_t item_count = 0;
  size_t max_item_count = session->free_publish_slots;
  size_t size = 0;
  struct wpcp_publish_queue_item_t* item = session->publish_queue.first;
  uint8_t* dest;

  WPCP_ASSERT(item);
  WPCP_ASSERT(max_item_count > 0);

  do {
    ++item_count;
    size += max_size_of_cbor_uint32 + item->data->output_buffer.used_size;
    item = item->next;
  } while (item && item_count < max_item_count);

  ret = wpcp_out_message_alloc(&session->wpcp->wpcp, sizeof(*ret), WPCP_MESSAGE_ID_PUBLISH, 0, item_count * 2, size, &dest);
  if (!ret)
    return NULL;

  session->free_publish_slots -= item_count;
  item = session->publish_queue.first;
  while (item_count--) {
    struct wpcp_publish_queue_item_t* next_item = item->next;
    dest = wpcp_cbor_write_unsigned_integer(dest, item->subscriptionid);
    dest = wpcp_cbor_write_write_buffer(dest, &item->data->output_buffer);
    wpcp_publish_queue_item_deref(item->data);
    item = next_item;
  }

  session->publish_item.next = &session->publish_item;
  session->publish_queue.first = item;
  if (!item) {
    session->publish_queue.last = NULL;
    session->queued_out_message_insert_location = &session->queued_out_messages;
  }
  ret->length = dest - ((uint8_t*)ret->data);

  return ret;
  }
}

void wpcp_session_out_message_delete(struct wpcp_out_message_t* out_message)
{
  wpcp_free(out_message);
}

struct wpcp_publish_queue_item_data_t* wpcp_publish_queue_item_data_create(size_t refs)
{
  size_t before = refs * sizeof(struct wpcp_publish_queue_item_t);
  struct wpcp_publish_queue_item_data_t* ret;
  uint8_t* data;

  WPCP_ASSERT(refs);

  data = wpcp_malloc(before + sizeof(*ret));
  if (!data)
    return NULL;

  ret = (struct wpcp_publish_queue_item_data_t*)(data + before);
  ret->references_inital = refs;
  ret->references_actual = refs;
  wpcp_cbor_write_buffer_init(&ret->output_buffer);

  return ret;
}

struct wpcp_publish_queue_item_t* wpcp_publish_queue_item_get(struct wpcp_publish_queue_item_data_t* publish_queue_item_data, size_t index)
{
  struct wpcp_publish_queue_item_t* ret = (struct wpcp_publish_queue_item_t*) publish_queue_item_data;
  return ret - 1 - index;
}

void wpcp_publish_queue_append(struct wpcp_publish_queue_t* publish_queue, struct wpcp_publish_queue_item_t* publish_item_first, struct wpcp_publish_queue_item_t* publish_item_last)
{
  if (publish_queue->last)
    publish_queue->last->next = publish_item_first;
  else {
    WPCP_ASSERT(!publish_queue->first);
    publish_queue->first = publish_item_first;
  }

  publish_queue->last = publish_item_last;
  publish_item_last->next = NULL;
}

void wpcp_session_publish_queue_append(struct wpcp_session_t* session, struct wpcp_publish_queue_item_t* publish_item_first, struct wpcp_publish_queue_item_t* publish_item_last)
{
  wpcp_publish_queue_append(&session->publish_queue, publish_item_first, publish_item_last);
  if (session->free_publish_slots) {
    if (session->queued_out_message_insert_location == &session->queued_out_messages)
      session->queued_out_message_insert_location = &session->publish_item.next;
    session->wpcp->wpcp.has_out_message.cb(session->user);
  }
}

struct wpcp_blocked_item_list_t* wpcp_blocked_item_for_notify_item(struct wpcp_publish_item_t* publish_item)
{
  struct wpcp_result_t* result = publish_item->session->results;

  while (result) {
    struct wpcp_blocked_item_list_t* blocked_items = result->blocked_items;
    if (blocked_items) {
      struct wpcp_publish_item_t** blocked_notify_item = blocked_items->items;
      while (*blocked_notify_item) {
        if (*blocked_notify_item == publish_item)
          return blocked_items;
        ++blocked_notify_item;
      }
    }

    result = result->next;
  }

  return NULL;
}

void wpcp_session_add_publish_queue_item(struct wpcp_publish_item_t* publish_item, struct wpcp_publish_queue_item_t* publish_queue_item)
{
  struct wpcp_blocked_item_list_t* blocked_items = wpcp_blocked_item_for_notify_item(publish_item);

  if (blocked_items) {
    wpcp_publish_queue_append(&blocked_items->publish_queue, publish_queue_item, publish_queue_item);
    return;
  }

  wpcp_session_publish_queue_append(publish_item->session, publish_queue_item, publish_queue_item);
}

struct wpcp_cbor_write_buffer_t* wpcp_publish_handle_publish(struct wpcp_publish_handle_t* publish_handle)
{
  size_t i;
  struct wpcp_publish_queue_item_data_t* publish_queue_item_data = wpcp_publish_queue_item_data_create(publish_handle->length);
  struct wpcp_publish_item_t* publish_item = publish_handle->items;

  if (!publish_queue_item_data)
    return NULL;

  for (i = 0; i < publish_handle->length; ++i) {
    struct wpcp_publish_queue_item_t* item = wpcp_publish_queue_item_get(publish_queue_item_data, i);
    item->subscriptionid = publish_item->subscriptionid;
    item->data = publish_queue_item_data;
    wpcp_session_add_publish_queue_item(publish_item, item);
    publish_item = publish_item->next;
  }

  return &publish_queue_item_data->output_buffer;
}

struct wpcp_result_t* wpcp_request_result(struct wpcp_session_t* session, uint32_t request_id, uint32_t payload_count)
{
  struct wpcp_result_t* ret;
  ret = wpcp_calloc(1, sizeof(*ret));

  WPCP_ASSERT(payload_count);

  if (ret) {
    struct wpcp_cbor_write_buffer_t* output_buffer = &ret->output_buffer;
    ret->request_id = request_id;
    ret->request_payload_count = payload_count;
    ret->wpcp = session->wpcp;
    ret->session = session;
    ret->remaining = payload_count;
    ret->next = session->results;
    ret->blocked_items = NULL;
    if (ret->next)
      ret->next->prev = ret;
    session->results = ret;

    wpcp_cbor_write_buffer_init(output_buffer);
  }

  return ret;
}

struct wpcp_publish_item_t** wpcp_result_get_blocked_publish_item_target(struct wpcp_result_t* result)
{
  struct wpcp_publish_item_t** ret;

  if (!result->blocked_items)
    result->blocked_items = wpcp_calloc(1, sizeof(*result->blocked_items) + result->remaining * sizeof(result->blocked_items->items[0]));

  if (!result->blocked_items)
    return NULL;

  ret = result->blocked_items->items;
  while (ret[0])
    ++ret;
  WPCP_ASSERT(!ret[1]);

  return ret;
}

void wpcp_result_add_diagnostic_info(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info)
{
  struct wpcp_cbor_write_buffer_t* write_buffer = &result->output_buffer;

  if (diagnostic_info)
    wpcp_cbor_write_buffer_write_wpcp_value(write_buffer, diagnostic_info);
  else
    wpcp_cbor_write_buffer_write_null(write_buffer);
}

void wpcp_result_add_unsigned_integer(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, uint32_t value)
{
  wpcp_result_add_diagnostic_info(result, diagnostic_info);
  wpcp_cbor_write_buffer_write_unsigned_integer(&result->output_buffer, value);
  wpcp_release_result(result);
}

void wpcp_result_add_value(struct wpcp_result_t* result, const struct wpcp_value_t* diagnostic_info, const struct wpcp_value_t* value)
{
  wpcp_result_add_diagnostic_info(result, diagnostic_info);
  wpcp_cbor_write_buffer_write_wpcp_value(&result->output_buffer, value);
  wpcp_release_result(result);
}

void wpcp_release_result(struct wpcp_result_t* result)
{
  if (--(result->remaining))
    return;

  if (result->session) {
    if (result->next)
      result->next->prev = result->prev;
    if (result->prev)
      result->prev->next = result->next;
    else {
      WPCP_ASSERT(result->session->results == result);
      result->session->results = result->next;
    }

    wpcp_session_append_out_message(result->session,
      WPCP_MESSAGE_ID_RESULT,
      result->request_id,
      result->request_payload_count * 2,
      &result->output_buffer);

    if (result->blocked_items) {
      struct wpcp_publish_queue_t* blocked_publish_queue = &result->blocked_items->publish_queue;
      if (blocked_publish_queue->first)
        wpcp_session_publish_queue_append(result->session, blocked_publish_queue->first, blocked_publish_queue->last);
      wpcp_free(result->blocked_items);
    }
  } else if (result->blocked_items) {
    struct wpcp_publish_queue_item_t* blocked_item = result->blocked_items->publish_queue.first;
    while (blocked_item) {
      struct wpcp_publish_queue_item_t* item = blocked_item;
      blocked_item = item->next;
      wpcp_publish_queue_item_deref(item->data);
    }
    wpcp_free(result->blocked_items);
  }

  wpcp_free(result);
}

void wpcp_request_subresult(struct wpcp_result_t* result, uint32_t count)
{
  wpcp_cbor_write_buffer_write_array_header(&result->output_buffer, count);

  if (count)
    result->remaining_subresults = count;
  else
    wpcp_release_result(result);
}

void wpcp_release_subresult(struct wpcp_result_t* result)
{
  if (--(result->remaining_subresults))
    return;

  wpcp_release_result(result);
}

WPCP_END_EXTERN_C
