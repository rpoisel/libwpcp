#include "test.h"
//#include "wpcp.h"
#include "../src/wpcp_session.h"
#include <stdlib.h>
#include <math.h>

#define INVALID_MESSAGE_ID 0x55

struct wpcp_t* g_wpcp;
struct wpcp_session_t* g_session;
struct wpcp_out_message_t* g_hello_result_message;
struct wpcp_value_t* g_hello_result_value;


static void ignore_has_out_message(void* user)
{
  (void) user;
}

static void ignore_republish(void* user, struct wpcp_publish_handle_t* publish_handle, struct wpcp_subscription_t* subscription)
{
  (void) user;
  (void) subscription;
  wpcp_return_republish(publish_handle);
}

static void ignore_unsubscribe(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription)
{
  (void) user;
  wpcp_return_unsubscribe(result, NULL, subscription);
}

static void tc_setup(void)
{
  testcase_setup();
  g_wpcp = wpcp_create();
  g_wpcp->has_out_message.cb = ignore_has_out_message;
  g_session = NULL;
}

static void tc_teardown(void)
{
  if (g_hello_result_value) {
    wpcp_value_free(g_hello_result_value);
    g_hello_result_value = NULL;
    wpcp_session_out_message_delete(g_hello_result_message);
    g_hello_result_message = NULL;
  }
  if (g_session) {
    wpcp_session_delete(g_session);
    g_session = NULL;
  }
  wpcp_delete(g_wpcp);
  testcase_teardown();
}

static uint8_t find_message_id(const char* name)
{
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = g_hello_result_message->data;
  buffer.end = buffer.begin + g_hello_result_message->length;

  ck_assert_int_eq(g_hello_result_value->type, WPCP_VALUE_TYPE_ARRAY);
  ck_assert_int_eq(g_hello_result_value->value.length, 3);
  struct wpcp_value_t* argument = g_hello_result_value->data.first_child->next->next;
  ck_assert_int_eq(argument->type, WPCP_VALUE_TYPE_MAP);
  ck_assert_int_eq(argument->value.length, 2);
  struct wpcp_value_t* methods = argument->data.first_child->next->next->next;

  uint8_t i;
  struct wpcp_value_t* val = methods->data.first_child;
  for (i = 0; i < methods->value.length; ++i) {
    if (!strncmp(name, val->data.text_string, val->value.length))
      return i;
    val = val->next->next;
    if (!val)
      break;
  }

  return INVALID_MESSAGE_ID;
}

static bool send_message(const char* name, const uint8_t* data, size_t size)
{
  uint8_t* data_copy = malloc(size);
  memcpy(data_copy, data, size);

  ck_assert_int_eq(data_copy[2], INVALID_MESSAGE_ID);
  data_copy[2] = find_message_id(name);
  ck_assert_int_ne(data_copy[2], INVALID_MESSAGE_ID);

  bool ret = wpcp_session_handle_in_message(g_session, data_copy, size);

  free(data_copy);

  return ret;
}


static void handle_hello()
{
  wpcp_has_out_message_cb_t old_has_out_message_cb = g_wpcp->has_out_message.cb;
  g_wpcp->has_out_message.cb = ignore_has_out_message;
  if (g_session)
    wpcp_session_delete(g_session);
  g_session = wpcp_session_create(g_wpcp, NULL);

  const uint8_t data[] = { 0x83, 0x18, 0x00, 0x00, 0xa0 };
  ck_assert(!wpcp_session_has_out_message(g_session));
  ck_assert(wpcp_session_handle_in_message(g_session, data, sizeof(data)));
  ck_assert(wpcp_session_has_out_message(g_session));

  struct wpcp_cbor_read_buffer_t buffer;
  g_hello_result_message = wpcp_session_out_message_create(g_session);
  buffer.begin = g_hello_result_message->data;
  buffer.end = buffer.begin + g_hello_result_message->length;
  g_hello_result_value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);

  ck_assert(!wpcp_session_has_out_message(g_session));

  g_wpcp->has_out_message.cb = old_has_out_message_cb;
}

START_TEST(create_malloc_fail)
{
  int malloc_fails[] = {1, 3, 0};
  testcase_set_malloc_fails(malloc_fails);

  ck_assert_ptr_eq(wpcp_create(), NULL);
}
END_TEST

START_TEST(invalid_in_messages)
{
  g_session = wpcp_session_create(g_wpcp, NULL);

  ck_assert(!wpcp_session_handle_in_message(g_session, NULL, 0));

  const uint8_t data1[] = { 0x80 };
  ck_assert(!wpcp_session_handle_in_message(g_session, data1, sizeof(data1)));

  const uint8_t data2[] = { 0x81, 0x00 };
  ck_assert(!wpcp_session_handle_in_message(g_session, data2, sizeof(data2)));

  const uint8_t data3[] = { 0x82, 0x20, 0x00 };
  ck_assert(!wpcp_session_handle_in_message(g_session, data3, sizeof(data3)));

  const uint8_t data4[] = { 0x82, 0x00, 0x20 };
  ck_assert(!wpcp_session_handle_in_message(g_session, data4, sizeof(data4)));

  const uint8_t data5[] = { 0x82, 0x00, 0x00 };
  ck_assert(!wpcp_session_handle_in_message(g_session, data5, sizeof(data5)));

  const uint8_t data6[] = { 0x82, 0x18, 0x00, 0x00 };
  ck_assert(!wpcp_session_handle_in_message(g_session, data6, sizeof(data6)));

  const uint8_t data7[] = { 0x83, 0x18, 0x00, 0x00, 0x00 };
  ck_assert(!wpcp_session_handle_in_message(g_session, data7, sizeof(data7)));

  handle_hello();

  const uint8_t data8[] = { 0x83, 0x1a, 0xff, 0xff, 0xff, 0xff, 0x00 };
  ck_assert(!wpcp_session_handle_in_message(g_session, data8, sizeof(data8)));

  const uint8_t data9[] = { 0x84, 0x1a, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00 };
  ck_assert(!wpcp_session_handle_in_message(g_session, data9, sizeof(data9)));

  const uint8_t data10[] = { 0x84, 0x1a, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 };
  ck_assert(!wpcp_session_handle_in_message(g_session, data10, sizeof(data10)));
}
END_TEST

static void browse_cb_0(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id)
{
  (void) user;
  (void) id;
  wpcp_return_browse(result, NULL, 0);
}

static void browse_cb_1(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id)
{
  (void) user;
  (void) id;
  wpcp_return_browse(result, NULL, 1);
  struct wpcp_value_t item_id;
  item_id.type = WPCP_VALUE_TYPE_NULL;
  struct wpcp_value_t item_type;
  item_type.type = WPCP_VALUE_TYPE_NULL;
  wpcp_return_browse_item(result, &item_id, "name", 4, "title", 5, "desc", 4, &item_type, NULL, 0);
}

START_TEST(browse_0)
{
  g_wpcp->browse.cb = browse_cb_0;
  handle_hello();

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0xa0 };
  send_message("browse", data, sizeof(data));
}
END_TEST

START_TEST(browse_1)
{
  g_wpcp->browse.cb = browse_cb_1;
  handle_hello();

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0xa0 };
  send_message("browse", data, sizeof(data));
}
END_TEST


void read_data_cb(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id)
{
  (void) user;
  wpcp_return_read_data(result, NULL, id, 0.0, 0, NULL, 0);
}

START_TEST(read_data_missing)
{
  handle_hello();
  ck_assert_int_eq(find_message_id("readdata"), INVALID_MESSAGE_ID);
}
END_TEST

START_TEST(read_malloc_fail)
{
  g_wpcp->read_data.cb = read_data_cb;
  handle_hello();

  int malloc_fails[] = {1, 0};
  testcase_set_malloc_fails(malloc_fails);

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0xa0 };
  ck_assert(!send_message("readdata", data, sizeof(data)));
}
END_TEST

START_TEST(read_data_without_payload)
{
  g_wpcp->read_data.cb = read_data_cb;
  handle_hello();

  const uint8_t data[] = { 0x82, 0x18, INVALID_MESSAGE_ID, 0x00 };
  ck_assert(send_message("readdata", data, sizeof(data)));
}
END_TEST

START_TEST(read_data_with_invalid_payload)
{
  g_wpcp->read_data.cb = read_data_cb;
  handle_hello();

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0x00 };
  ck_assert(!send_message("readdata", data, sizeof(data)));
}
END_TEST

START_TEST(read_data_payload_without_id)
{
  g_wpcp->read_data.cb = read_data_cb;
  handle_hello();

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0xa0 };
  ck_assert(send_message("readdata", data, sizeof(data)));
}
END_TEST


void write_data_cb(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, const struct wpcp_value_t* value)
{
  (void) user;
  wpcp_return_write_data(result, NULL, !!id && !!value);
}

START_TEST(write_data_missing)
{
  handle_hello();
  ck_assert_int_eq(find_message_id("writedata"), INVALID_MESSAGE_ID);
}
END_TEST

START_TEST(write_data)
{
  g_wpcp->write_data.cb = write_data_cb;
  handle_hello();

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0xa0 };
  send_message("writedata", data, sizeof(data));
}
END_TEST


static void handle_alarm_cb(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* token, const struct wpcp_value_t* acknowledge)
{
  (void) user;
  (void) token;
  (void) acknowledge;
  wpcp_return_handle_alarm(result, NULL, false);
}

START_TEST(handle_alarm)
{
  g_wpcp->handle_alarm.cb = handle_alarm_cb;
  handle_hello();

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0xa0 };
  send_message("handlealarm", data, sizeof(data));
}
END_TEST

static void read_history_data_cb_0(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, const struct wpcp_value_t* starttime, const struct wpcp_value_t* endtime, const struct wpcp_value_t* maxresults, const struct wpcp_value_t* aggregation, const struct wpcp_value_t* interval)
{
  (void) user;
  (void) id;
  (void) starttime;
  (void) endtime;
  (void) maxresults;
  (void) aggregation;
  (void) interval;
  wpcp_return_read_history_data(result, NULL, 0);
}

static void read_history_data_cb_1(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, const struct wpcp_value_t* starttime, const struct wpcp_value_t* endtime, const struct wpcp_value_t* maxresults, const struct wpcp_value_t* aggregation, const struct wpcp_value_t* interval)
{
  (void) user;
  (void) id;
  (void) starttime;
  (void) endtime;
  (void) maxresults;
  (void) aggregation;
  (void) interval;
  wpcp_return_read_history_data(result, NULL, 1);
  wpcp_return_read_history_data_item(result, NULL, 0.0, 0, NULL, 0);
}

START_TEST(read_history_data_0)
{
  g_wpcp->read_history_data.cb = read_history_data_cb_0;
  handle_hello();

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0xa0 };
  send_message("readhistorydata", data, sizeof(data));
}
END_TEST

START_TEST(read_history_data_1)
{
  g_wpcp->read_history_data.cb = read_history_data_cb_1;
  handle_hello();

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0xa0 };
  send_message("readhistorydata", data, sizeof(data));
}
END_TEST


static void subscribe_data_cb_alias(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, const struct wpcp_value_t* id)
{
  static struct wpcp_publish_handle_t* publish_handle;
  (void) user;
  (void) id;

  if (publish_handle)
    wpcp_return_subscribe_alias(result, NULL, subscription, publish_handle);
  else
    publish_handle = wpcp_return_subscribe_accept(result, NULL, subscription);
}

static void subscribe_data_cb_reject(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, const struct wpcp_value_t* id)
{
  (void) user;
  (void) id;
  wpcp_return_subscribe_reject(result, NULL, subscription);
}

START_TEST(subscribe_data_alias)
{
  g_wpcp->republish.cb = ignore_republish;
  g_wpcp->subscribe_data.cb = subscribe_data_cb_alias;
  g_wpcp->unsubscribe.cb = ignore_unsubscribe;
  handle_hello();

  const uint8_t data[] = { 0x84, 0x18, INVALID_MESSAGE_ID, 0x00, 0xa0, 0xa0 };
  send_message("subscribedata", data, sizeof(data));
}
END_TEST

START_TEST(subscribe_data_reject)
{
  g_wpcp->subscribe_data.cb = subscribe_data_cb_reject;
  handle_hello();

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0xa0 };
  send_message("subscribedata", data, sizeof(data));
}
END_TEST


static void subscribe_alarm_cb_alias(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, const struct wpcp_value_t* id, const struct wpcp_value_t* filter)
{
  static struct wpcp_publish_handle_t* publish_handle;
  (void) user;
  (void) id;
  (void) filter;

  if (publish_handle)
    wpcp_return_subscribe_alias(result, NULL, subscription, publish_handle);
  else
    publish_handle = wpcp_return_subscribe_accept(result, NULL, subscription);
}

static void subscribe_alarm_cb_reject(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, const struct wpcp_value_t* id, const struct wpcp_value_t* filter)
{
  (void) user;
  (void) id;
  (void) filter;
  wpcp_return_subscribe_reject(result, NULL, subscription);
}

START_TEST(subscribe_alarm_alias)
{
  g_wpcp->republish.cb = ignore_republish;
  g_wpcp->subscribe_alarm.cb = subscribe_alarm_cb_alias;
  g_wpcp->unsubscribe.cb = ignore_unsubscribe;

  handle_hello();

  const uint8_t data[] = { 0x84, 0x18, INVALID_MESSAGE_ID, 0x00, 0xa0, 0xa0 };
  send_message("subscribealarm", data, sizeof(data));
}
END_TEST

START_TEST(subscribe_alarm_reject)
{
  g_wpcp->subscribe_alarm.cb = subscribe_alarm_cb_reject;
  handle_hello();

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0xa0 };
  send_message("subscribealarm", data, sizeof(data));
}
END_TEST


void unsubscribe_cb(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription)
{
  (void) user;
  wpcp_return_unsubscribe(result, NULL, subscription);
}

START_TEST(unsubscribe_malloc_fail)
{
  g_wpcp->unsubscribe.cb = unsubscribe_cb;
  handle_hello();

  int malloc_fails[] = {1, 0};
  testcase_set_malloc_fails(malloc_fails);

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0x00 };
  ck_assert(!send_message("unsubscribe", data, sizeof(data)));
}
END_TEST

START_TEST(unsubscribe_unknown)
{
  g_wpcp->unsubscribe.cb = unsubscribe_cb;
  handle_hello();

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0x0f };
  ck_assert(send_message("unsubscribe", data, sizeof(data)));
}
END_TEST

START_TEST(unsubscribe_invalid)
{
  g_wpcp->unsubscribe.cb = unsubscribe_cb;
  handle_hello();

  const uint8_t data[] = { 0x83, 0x18, INVALID_MESSAGE_ID, 0x00, 0xa0 };
  ck_assert(!send_message("unsubscribe", data, sizeof(data)));
}
END_TEST



TCase* testcase_wpcp(void)
{
  TCase* ret = tcase_create ("WPCP");
  tcase_add_checked_fixture(ret, tc_setup, tc_teardown);

  tcase_add_test(ret, browse_0);
  tcase_add_test(ret, browse_1);
  tcase_add_test(ret, read_data_missing);
  tcase_add_test(ret, read_data_payload_without_id);
  tcase_add_test(ret, read_data_with_invalid_payload);
  tcase_add_test(ret, read_data_without_payload);
  tcase_add_test(ret, write_data_missing);
  tcase_add_test(ret, write_data);
  tcase_add_test(ret, invalid_in_messages);
  tcase_add_test(ret, create_malloc_fail);
  tcase_add_test(ret, read_malloc_fail);
  tcase_add_test(ret, handle_alarm);
  tcase_add_test(ret, read_history_data_0);
  tcase_add_test(ret, read_history_data_1);
  tcase_add_test(ret, subscribe_data_alias);
  tcase_add_test(ret, subscribe_alarm_alias);
  tcase_add_test(ret, subscribe_data_reject);
  tcase_add_test(ret, subscribe_alarm_reject);
  tcase_add_test(ret, unsubscribe_malloc_fail);
  tcase_add_test(ret, unsubscribe_unknown);
  tcase_add_test(ret, unsubscribe_invalid);

  return ret;
}
