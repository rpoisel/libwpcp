#include "test.h"
//#include "wpcp.h"
#include "../src/wpcp_session.h"
#include <stdlib.h>
#include <math.h>


struct wpcp_internal_t g_wpcp_internal;
struct wpcp_session_t* g_session;
static struct wpcp_cbor_write_buffer_t g_write_buffer;

static void tc_setup(void)
{
  testcase_setup();
  memset(&g_wpcp_internal, 0, sizeof(g_wpcp_internal));
  g_session = wpcp_session_create(&g_wpcp_internal.wpcp, NULL);
}

static void tc_teardown(void)
{
  if (g_session)
    wpcp_session_delete(g_session);
  testcase_teardown();
}




size_t g_count;
void* g_user;
void** g_context;
void* g_context_content;
size_t g_remaining;
struct wpcp_subscription_t* g_subscription;

static void unsubscribe_cb(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, void** context, uint32_t remaining)
{
  g_count += 1;
  g_user = user;
  (void) result;
  g_context = context;
  g_context_content = *context;
  g_remaining = remaining;
  g_subscription = subscription;

  wpcp_return_unsubscribe(result, NULL, subscription);
}

static void has_out_message_cb(void* user)
{
  g_count += 1;
  g_user = user;
}

START_TEST(subscription_request_release)
{
  const uint8_t key1[] = { 0x01 };
  const uint8_t key2[] = { 0x02 };

  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  struct wpcp_subscription_t* subs2 = wpcp_request_subscription(&g_wpcp_internal, 4, NULL, 0);
  struct wpcp_subscription_t* subs3 = wpcp_request_subscription(&g_wpcp_internal, 8, NULL, 0);
  struct wpcp_subscription_t* subs4 = wpcp_request_subscription(&g_wpcp_internal, 2, NULL, 0);
  struct wpcp_subscription_t* subs5 = wpcp_request_subscription(&g_wpcp_internal, 0, key1, 1);
  struct wpcp_subscription_t* subs6 = wpcp_request_subscription(&g_wpcp_internal, 0, key2, 1);

  struct wpcp_subscription_t* subs5b = wpcp_request_subscription(&g_wpcp_internal, 0, key1, 1);
  struct wpcp_subscription_t* subs4b = wpcp_request_subscription(&g_wpcp_internal, 2, NULL, 0);
  struct wpcp_subscription_t* subs2b = wpcp_request_subscription(&g_wpcp_internal, 4, NULL, 0);
  struct wpcp_subscription_t* subs6b = wpcp_request_subscription(&g_wpcp_internal, 0, key2, 1);

  ck_assert_ptr_ne(subs1, NULL);
  ck_assert_ptr_ne(subs2, NULL);
  ck_assert_ptr_eq(subs2b, subs2);
  ck_assert_ptr_ne(subs3, NULL);
  ck_assert_ptr_ne(subs4, NULL);
  ck_assert_ptr_eq(subs4b, subs4);
  ck_assert_ptr_ne(subs5, NULL);
  ck_assert_ptr_eq(subs5b, subs5);
  ck_assert_ptr_ne(subs6, NULL);
  ck_assert_ptr_eq(subs6b, subs6);

  wpcp_release_subscription(&g_wpcp_internal, subs5b);
  wpcp_release_subscription(&g_wpcp_internal, subs4);
  wpcp_release_subscription(&g_wpcp_internal, subs2b);
  wpcp_release_subscription(&g_wpcp_internal, subs3);
  wpcp_release_subscription(&g_wpcp_internal, subs5);
  wpcp_release_subscription(&g_wpcp_internal, subs6b);
  wpcp_release_subscription(&g_wpcp_internal, subs2);
  wpcp_release_subscription(&g_wpcp_internal, subs1);
  wpcp_release_subscription(&g_wpcp_internal, subs6);
  wpcp_release_subscription(&g_wpcp_internal, subs4b);
}
END_TEST

START_TEST(subscription_realloc)
{
  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_ptr_ne(subs1, NULL);
  wpcp_release_subscription(&g_wpcp_internal, subs1);

  struct wpcp_subscription_t* subs2 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_ptr_ne(subs2, NULL);
  wpcp_release_subscription(&g_wpcp_internal, subs2);
}
END_TEST

START_TEST(subscription_malloc_fail)
{
  int malloc_fails[] = {1, 2, 4, 0};
  testcase_set_malloc_fails(malloc_fails);

  struct wpcp_session_t* session = wpcp_session_create(&g_wpcp_internal.wpcp, NULL);
  ck_assert_ptr_eq(session, NULL);

  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  struct wpcp_subscription_t* subs2 = wpcp_request_subscription(&g_wpcp_internal, 1, NULL, 0);
  struct wpcp_subscription_t* subs3 = wpcp_request_subscription(&g_wpcp_internal, 2, NULL, 0);

  ck_assert_ptr_eq(subs1, NULL);
  ck_assert_ptr_eq(subs2, NULL);
  ck_assert_ptr_ne(subs3, NULL);

  wpcp_release_subscription(&g_wpcp_internal, subs3);
}
END_TEST


START_TEST(subscription_usage_count)
{
  struct wpcp_subscription_t* subsa = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  struct wpcp_subscription_t* subsb = wpcp_request_subscription(&g_wpcp_internal, 1, NULL, 0);

  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsa), 0);
  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsb), 0);

  uint32_t id1 = wpcp_session_get_next_free_publish_id(g_session);
  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  struct wpcp_publish_item_t* item1 = wpcp_session_request_publish_item(g_session, subs1, id1);

  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsa), 1);
  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsb), 0);

  uint32_t id2 = wpcp_session_get_next_free_publish_id(g_session);
  struct wpcp_subscription_t* subs2 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  struct wpcp_publish_item_t* item2 = wpcp_session_request_publish_item(g_session, subs2, id2);

  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsa), 2);
  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsb), 0);

  uint32_t id3 = wpcp_session_get_next_free_publish_id(g_session);
  struct wpcp_subscription_t* subs3 = wpcp_request_subscription(&g_wpcp_internal, 1, NULL, 0);
  struct wpcp_publish_item_t* item3 = wpcp_session_request_publish_item(g_session, subs3, id3);

  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsa), 2);
  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsb), 1);

  wpcp_session_release_publish_item(g_session, item1);

  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsa), 1);
  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsb), 1);

  wpcp_session_release_publish_item(g_session, item2);

  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsa), 0);
  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsb), 1);

  wpcp_session_release_publish_item(g_session, item3);

  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsa), 0);
  ck_assert_int_eq(wpcp_subscription_get_usage_count(subsb), 0);

  wpcp_release_subscription(&g_wpcp_internal, subsb);
  wpcp_release_subscription(&g_wpcp_internal, subsa);
}
END_TEST

START_TEST(subscription_user)
{
  int var = 0;
  struct wpcp_subscription_t* subs = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);

  ck_assert_ptr_eq(wpcp_subscription_get_user(subs), NULL);
  wpcp_subscription_set_user(subs, &var);
  ck_assert_ptr_eq(wpcp_subscription_get_user(subs), &var);

  wpcp_release_subscription(&g_wpcp_internal, subs);
}
END_TEST

START_TEST(subscription_type)
{
  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 123, NULL, 0);
  struct wpcp_subscription_t* subs2 = wpcp_request_subscription(&g_wpcp_internal, 456, NULL, 0);

  ck_assert_int_eq(wpcp_subscription_get_type(subs1), 123);
  ck_assert_int_eq(wpcp_subscription_get_type(subs2), 456);

  wpcp_release_subscription(&g_wpcp_internal, subs2);
  wpcp_release_subscription(&g_wpcp_internal, subs1);
}
END_TEST

START_TEST(get_subscription_count)
{
  ck_assert_int_eq(wpcp_get_subscription_count(&g_wpcp_internal.wpcp), 0);

  struct wpcp_subscription_t* subsa = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_int_eq(wpcp_get_subscription_count(&g_wpcp_internal.wpcp), 1);

  struct wpcp_subscription_t* subsb = wpcp_request_subscription(&g_wpcp_internal, 1, NULL, 0);
  ck_assert_int_eq(wpcp_get_subscription_count(&g_wpcp_internal.wpcp), 2);

  wpcp_release_subscription(&g_wpcp_internal, subsb);
  ck_assert_int_eq(wpcp_get_subscription_count(&g_wpcp_internal.wpcp), 1);

  wpcp_release_subscription(&g_wpcp_internal, subsa);
  ck_assert_int_eq(wpcp_get_subscription_count(&g_wpcp_internal.wpcp), 0);
}
END_TEST

struct test_session_iterate_subscriptions_t {
  size_t counter;
  struct wpcp_subscription_t* subscriptions[2];
};

void test_session_iterate_subscriptions_callback(void* user, struct wpcp_subscription_t* subscription, struct wpcp_publish_handle_t* publish_handle)
{
  struct test_session_iterate_subscriptions_t* helper = (struct test_session_iterate_subscriptions_t*) user;
  ck_assert_ptr_eq(subscription, helper->subscriptions[helper->counter]);
  ck_assert_ptr_eq(publish_handle, &helper->subscriptions[helper->counter++]->publish_handle);
}

START_TEST(iterate_subscriptions)
{
  struct test_session_iterate_subscriptions_t helper;
  memset(&helper, 0, sizeof(helper));

  wpcp_iterate_subscriptions(&g_wpcp_internal.wpcp, &helper, test_session_iterate_subscriptions_callback);
  ck_assert_int_eq(helper.counter, 0);

  helper.counter = 0;
  helper.subscriptions[0] = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  wpcp_iterate_subscriptions(&g_wpcp_internal.wpcp, &helper, test_session_iterate_subscriptions_callback);
  ck_assert_int_eq(helper.counter, 1);

  helper.counter = 0;
  helper.subscriptions[1] = wpcp_request_subscription(&g_wpcp_internal, 1, NULL, 0);
  wpcp_iterate_subscriptions(&g_wpcp_internal.wpcp, &helper, test_session_iterate_subscriptions_callback);
  ck_assert_int_eq(helper.counter, 2);

  wpcp_release_subscription(&g_wpcp_internal, helper.subscriptions[0]);
  wpcp_release_subscription(&g_wpcp_internal, helper.subscriptions[1]);

  helper.counter = 0;
  wpcp_iterate_subscriptions(&g_wpcp_internal.wpcp, &helper, test_session_iterate_subscriptions_callback);
  ck_assert_int_eq(helper.counter, 0);
}
END_TEST

START_TEST(subscription_publish_item)
{
  uint32_t id1 = wpcp_session_get_next_free_publish_id(g_session);
  ck_assert_int_ne(id1, 0);
  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_ptr_ne(subs1, NULL);
  struct wpcp_publish_item_t* item1 = wpcp_session_request_publish_item(g_session, subs1, id1);
  ck_assert_ptr_ne(item1, NULL);

  uint32_t id2 = wpcp_session_get_next_free_publish_id(g_session);
  ck_assert_int_ne(id2, 0);
  ck_assert_int_ne(id2, id1);
  struct wpcp_subscription_t* subs2 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_ptr_eq(subs2, subs1);
  struct wpcp_publish_item_t* item2 = wpcp_session_request_publish_item(g_session, subs2, id2);
  ck_assert_ptr_ne(item2, NULL);

  wpcp_session_release_publish_item(g_session, item1);

  uint32_t id3 = wpcp_session_get_next_free_publish_id(g_session);
  ck_assert_int_ne(id3, 0);
  ck_assert_int_ne(id3, id2);
  struct wpcp_subscription_t* subs3 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_ptr_eq(subs3, subs1);
  struct wpcp_publish_item_t* item3 = wpcp_session_request_publish_item(g_session, subs3, id3);
  ck_assert_ptr_ne(item3, NULL);

  uint32_t id4 = wpcp_session_get_next_free_publish_id(g_session);
  ck_assert_int_ne(id4, 0);
  ck_assert_int_ne(id4, id2);
  ck_assert_int_ne(id4, id3);
  struct wpcp_subscription_t* subs4 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_ptr_eq(subs4, subs1);
  struct wpcp_publish_item_t* item4 = wpcp_session_request_publish_item(g_session, subs4, id4);
  ck_assert_ptr_ne(item4, NULL);

  wpcp_session_release_publish_item(g_session, item3);

  wpcp_session_release_publish_item(g_session, item2);

  wpcp_session_release_publish_item(g_session, item4);
}
END_TEST

START_TEST(subscription_publish_item_publish)
{
  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;

  uint32_t id1 = wpcp_session_get_next_free_publish_id(g_session);
  ck_assert_int_ne(id1, 0);
  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_ptr_ne(subs1, NULL);
  struct wpcp_publish_item_t* item1 = wpcp_session_request_publish_item(g_session, subs1, id1);
  ck_assert_ptr_ne(item1, NULL);

  ck_assert(!wpcp_session_has_out_message(g_session));

  g_count = 0;
  struct wpcp_cbor_write_buffer_t* write_buffer = wpcp_publish_handle_publish(&subs1->publish_handle);
  ck_assert_int_eq(g_count, 1);
  ck_assert_ptr_eq(g_user, NULL);
  wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer, 1);

  ck_assert(wpcp_session_has_out_message(g_session));

  struct wpcp_out_message_t* out_message = wpcp_session_out_message_create(g_session);

  ck_assert_ptr_ne(out_message, NULL);

  wpcp_session_out_message_delete(out_message);

  wpcp_session_release_publish_item(g_session, item1);
}
END_TEST

START_TEST(subscription_max_publish_item)
{
  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;
  g_session->free_publish_slots = 1;

  uint32_t id1 = wpcp_session_get_next_free_publish_id(g_session);
  ck_assert_int_ne(id1, 0);
  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_ptr_ne(subs1, NULL);
  struct wpcp_publish_item_t* item1 = wpcp_session_request_publish_item(g_session, subs1, id1);
  ck_assert_ptr_ne(item1, NULL);

  ck_assert(!wpcp_session_has_out_message(g_session));

  g_count = 0;
  struct wpcp_cbor_write_buffer_t* write_buffer = wpcp_publish_handle_publish(&subs1->publish_handle);
  ck_assert_int_ne(g_count, 0);
  wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer, 1);

  ck_assert(wpcp_session_has_out_message(g_session));

  g_count = 0;
  write_buffer = wpcp_publish_handle_publish(&subs1->publish_handle);
  wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer, 2);

  struct wpcp_out_message_t* out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL);
  wpcp_session_out_message_delete(out_message);

  ck_assert(!wpcp_session_has_out_message(g_session));

  g_count = 0;
  write_buffer = wpcp_publish_handle_publish(&subs1->publish_handle);
  ck_assert_int_eq(g_count, 0);
  wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer, 2);

  ck_assert(!wpcp_session_has_out_message(g_session));


  g_count = 0;
  wpcp_session_add_free_publish_slots(g_session, 1);
  ck_assert_int_ne(g_count, 0);

  ck_assert(wpcp_session_has_out_message(g_session));

  out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL);
  wpcp_session_out_message_delete(out_message);

  ck_assert(!wpcp_session_has_out_message(g_session));

  g_count = 0;
  wpcp_session_add_free_publish_slots(g_session, 1);
  ck_assert_int_ne(g_count, 0);

  ck_assert(wpcp_session_has_out_message(g_session));

  out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL);
  wpcp_session_out_message_delete(out_message);

  ck_assert(!wpcp_session_has_out_message(g_session));

  g_count = 0;
  wpcp_session_add_free_publish_slots(g_session, 1);
  ck_assert_int_eq(g_count, 0);

  wpcp_session_release_publish_item(g_session, item1);
}
END_TEST

START_TEST(subscription_publish_item_publish_two_sessions)
{
  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;

  struct wpcp_session_t* session = wpcp_session_create(&g_wpcp_internal.wpcp, NULL);

  uint32_t id1 = wpcp_session_get_next_free_publish_id(g_session);
  ck_assert_int_ne(id1, 0);
  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_ptr_ne(subs1, NULL);
  struct wpcp_publish_item_t* item1 = wpcp_session_request_publish_item(g_session, subs1, id1);
  ck_assert_ptr_ne(item1, NULL);

  uint32_t id2 = wpcp_session_get_next_free_publish_id(session);
  ck_assert_int_ne(id1, 0);
  struct wpcp_subscription_t* subs2 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_ptr_ne(subs1, NULL);
  struct wpcp_publish_item_t* item2 = wpcp_session_request_publish_item(session, subs2, id2);
  ck_assert_ptr_ne(item1, NULL);

  ck_assert(!wpcp_session_has_out_message(g_session));
  ck_assert(!wpcp_session_has_out_message(session));

  g_count = 0;
  struct wpcp_cbor_write_buffer_t* write_buffer = wpcp_publish_handle_publish(&subs1->publish_handle);
  ck_assert_int_eq(g_count, 2);
  wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer, 1);

  ck_assert(wpcp_session_has_out_message(g_session));
  ck_assert(wpcp_session_has_out_message(session));

  struct wpcp_out_message_t* out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL);
  wpcp_session_out_message_delete(out_message);

  ck_assert(!wpcp_session_has_out_message(g_session));
  ck_assert(wpcp_session_has_out_message(session));

  g_count = 0;
  write_buffer = wpcp_publish_handle_publish(&subs1->publish_handle);
  ck_assert_int_eq(g_count, 2);
  wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer, 2);

  ck_assert(wpcp_session_has_out_message(g_session));
  ck_assert(wpcp_session_has_out_message(session));

  out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL);
  wpcp_session_out_message_delete(out_message);

  out_message = wpcp_session_out_message_create(session);
  ck_assert_ptr_ne(out_message, NULL);
  wpcp_session_out_message_delete(out_message);

  ck_assert(!wpcp_session_has_out_message(g_session));
  ck_assert(!wpcp_session_has_out_message(session));

  wpcp_session_release_publish_item(session, item2);

  wpcp_session_release_publish_item(g_session, item1);

  wpcp_session_delete(session);
}
END_TEST

START_TEST(subscription_publish_item_publish_malloc_fail)
{
  int malloc_fails[] = {5, 7, 0};
  testcase_set_malloc_fails(malloc_fails);

  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;

  uint32_t id1 = wpcp_session_get_next_free_publish_id(g_session);
  ck_assert_int_ne(id1, 0);
  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_ptr_ne(subs1, NULL);
  struct wpcp_publish_item_t* item1 = wpcp_session_request_publish_item(g_session, subs1, id1);
  ck_assert_ptr_ne(item1, NULL);

  ck_assert(!wpcp_session_has_out_message(g_session));

  g_count = 0;
  struct wpcp_cbor_write_buffer_t* write_buffer = wpcp_publish_handle_publish(&subs1->publish_handle);
  ck_assert_int_eq(g_count, 0);
  ck_assert_ptr_eq(write_buffer, NULL);
  ck_assert(!wpcp_session_has_out_message(g_session));

  write_buffer = wpcp_publish_handle_publish(&subs1->publish_handle);
  ck_assert_int_eq(g_count, 1);
  ck_assert_ptr_ne(write_buffer, NULL);

  wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer, 1);

  ck_assert(wpcp_session_has_out_message(g_session));

  struct wpcp_out_message_t* out_message = wpcp_session_out_message_create(g_session);

  ck_assert_ptr_eq(out_message, NULL);

  wpcp_session_out_message_delete(out_message);

  wpcp_session_release_publish_item(g_session, item1);
}
END_TEST

START_TEST(subscription_release_publish_item_via_session)
{
  g_wpcp_internal.wpcp.unsubscribe.ex_cb = unsubscribe_cb;

  uint32_t id1 = wpcp_session_get_next_free_publish_id(g_session);
  ck_assert_int_ne(id1, 0);
  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_ptr_ne(subs1, NULL);
  struct wpcp_publish_item_t* item1 = wpcp_session_request_publish_item(g_session, subs1, id1);
  ck_assert_ptr_ne(item1, NULL);

  g_count = 0;
  wpcp_session_delete(g_session);
  g_session = NULL;

  ck_assert_int_eq(g_count, 1);
  ck_assert_ptr_eq(g_user, NULL);
  ck_assert_ptr_ne(g_context, NULL);
  ck_assert_ptr_eq(g_context_content, NULL);
  ck_assert_int_eq(g_remaining, 0);
  ck_assert_ptr_eq(g_subscription, subs1);
}
END_TEST

START_TEST(subscription_publish_item_malloc_fail)
{
  int malloc_fails[] = {1, 4, 0};
  testcase_set_malloc_fails(malloc_fails);

  uint32_t id1 = wpcp_session_get_next_free_publish_id(g_session);
  ck_assert_int_eq(id1, 0);
  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  ck_assert_ptr_ne(subs1, NULL);
  struct wpcp_publish_item_t* item1 = wpcp_session_request_publish_item(g_session, subs1, id1);
  ck_assert_ptr_eq(item1, NULL);
}
END_TEST

START_TEST(session_out_message)
{
  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;


  ck_assert(!wpcp_session_has_out_message(g_session));

  g_count = 0;
  wpcp_session_append_out_message(g_session, WPCP_MESSAGE_ID_RESULT, 1, 0, &g_write_buffer);

  ck_assert_int_eq(g_count, 1);
  ck_assert_ptr_eq(g_user, NULL);

  ck_assert(wpcp_session_has_out_message(g_session));

  struct wpcp_out_message_t* out_message = wpcp_session_out_message_create(g_session);

  ck_assert_ptr_ne(out_message, NULL);

  wpcp_session_out_message_delete(out_message);
}
END_TEST

START_TEST(session_out_message_malloc_fail)
{
  int malloc_fails[] = {1, 0};
  testcase_set_malloc_fails(malloc_fails);

  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;

  ck_assert(!wpcp_session_has_out_message(g_session));

  g_count = 0;
  wpcp_session_append_out_message(g_session, WPCP_MESSAGE_ID_RESULT, 1, 0, &g_write_buffer);

  ck_assert_int_eq(g_count, 1);
  ck_assert_ptr_eq(g_user, NULL);

  ck_assert(wpcp_session_has_out_message(g_session));

  struct wpcp_out_message_t* out_message = wpcp_session_out_message_create(g_session);

  ck_assert_ptr_eq(out_message, NULL);

  wpcp_session_out_message_delete(out_message);
}
END_TEST

START_TEST(result)
{
  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;
  ck_assert(!wpcp_session_has_out_message(g_session));

  struct wpcp_result_t* result1 = wpcp_request_result(g_session, 1, 2);
  struct wpcp_result_t* result2 = wpcp_request_result(g_session, 2, 1);
  struct wpcp_result_t* result3 = wpcp_request_result(g_session, 3, 1);
  struct wpcp_result_t* result4 = wpcp_request_result(g_session, 4, 1);

  ck_assert(!wpcp_session_has_out_message(g_session));

  wpcp_release_result(result2);
  wpcp_release_result(result1);
  wpcp_release_result(result1);
  wpcp_release_result(result4);
  wpcp_release_result(result3);

  ck_assert(wpcp_session_has_out_message(g_session));
}
END_TEST

START_TEST(result_subresult)
{
  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;
  ck_assert(!wpcp_session_has_out_message(g_session));

  struct wpcp_result_t* result1 = wpcp_request_result(g_session, 1, 2);
  struct wpcp_result_t* result2 = wpcp_request_result(g_session, 2, 1);

  ck_assert(!wpcp_session_has_out_message(g_session));

  wpcp_request_subresult(result1, 0);
  wpcp_request_subresult(result1, 2);
  wpcp_release_subresult(result1);
  wpcp_release_subresult(result1);

  wpcp_request_subresult(result2, 1);
  wpcp_release_subresult(result2);

  ck_assert(wpcp_session_has_out_message(g_session));
}
END_TEST

START_TEST(result_return)
{
  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;
  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_UNDEFINED;

  struct wpcp_result_t* result1 = wpcp_request_result(g_session, 1, 1);
  ck_assert(!wpcp_session_has_out_message(g_session));
  wpcp_result_add_value(result1, NULL, &value);

  ck_assert(wpcp_session_has_out_message(g_session));
  struct wpcp_out_message_t* out_message1 = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message1, NULL); // TODO: check
  wpcp_session_out_message_delete(out_message1);
  ck_assert(!wpcp_session_has_out_message(g_session));

  struct wpcp_result_t* result2 = wpcp_request_result(g_session, 1, 1);
  ck_assert(!wpcp_session_has_out_message(g_session));
  wpcp_result_add_unsigned_integer(result2, &value, 0);

  ck_assert(wpcp_session_has_out_message(g_session));
  struct wpcp_out_message_t* out_message2 = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message2, NULL); // TODO: check
  wpcp_session_out_message_delete(out_message2);
  ck_assert(!wpcp_session_has_out_message(g_session));
}
END_TEST

START_TEST(result_malloc_fail)
{
  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;
  int malloc_fails[] = {1, 3, 0};
  testcase_set_malloc_fails(malloc_fails);

  struct wpcp_result_t* result = wpcp_request_result(g_session, 1, 1);
  ck_assert(!result);

  result = wpcp_request_result(g_session, 1, 1);
  ck_assert(!wpcp_result_get_blocked_publish_item_target(result));
  wpcp_release_result(result);
}
END_TEST

START_TEST(result_block)
{
  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;
  ck_assert(!wpcp_session_has_out_message(g_session));

  struct wpcp_result_t* result1 = wpcp_request_result(g_session, 1, 1);

  uint32_t id1 = wpcp_session_get_next_free_publish_id(g_session);
  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  struct wpcp_publish_item_t* item1 = wpcp_session_request_publish_item(g_session, subs1, id1);

  ck_assert(!wpcp_session_has_out_message(g_session));

  *wpcp_result_get_blocked_publish_item_target(result1) = item1;

  g_count = 0;
  struct wpcp_cbor_write_buffer_t* write_buffer = wpcp_publish_handle_publish(&subs1->publish_handle);
  wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer, 1);
  ck_assert_int_eq(g_count, 0);

  ck_assert(!wpcp_session_has_out_message(g_session));

  wpcp_release_result(result1);

  ck_assert_int_ne(g_count, 0);

  ck_assert(wpcp_session_has_out_message(g_session));
  struct wpcp_out_message_t* out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL);
  wpcp_session_out_message_delete(out_message);

  ck_assert(wpcp_session_has_out_message(g_session));
  out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL);
  wpcp_session_out_message_delete(out_message);

  ck_assert(!wpcp_session_has_out_message(g_session));

  struct wpcp_result_t* result2 = wpcp_request_result(g_session, 1, 1);
  *wpcp_result_get_blocked_publish_item_target(result2) = item1;
  wpcp_release_result(result2);

  ck_assert(wpcp_session_has_out_message(g_session));
  out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL);
  wpcp_session_out_message_delete(out_message);

  ck_assert(!wpcp_session_has_out_message(g_session));

  wpcp_session_release_publish_item(g_session, item1);
}
END_TEST

START_TEST(result_block_multi)
{
  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;
  ck_assert(!wpcp_session_has_out_message(g_session));

  struct wpcp_result_t* result1 = wpcp_request_result(g_session, 1, 1);
  struct wpcp_result_t* result2 = wpcp_request_result(g_session, 1, 2);

  uint32_t id1 = wpcp_session_get_next_free_publish_id(g_session);
  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  struct wpcp_publish_item_t* item1 = wpcp_session_request_publish_item(g_session, subs1, id1);

  uint32_t id2 = wpcp_session_get_next_free_publish_id(g_session);
  struct wpcp_subscription_t* subs2 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  struct wpcp_publish_item_t* item2 = wpcp_session_request_publish_item(g_session, subs2, id2);

  uint32_t id3 = wpcp_session_get_next_free_publish_id(g_session);
  struct wpcp_subscription_t* subs3 = wpcp_request_subscription(&g_wpcp_internal, 1, NULL, 0);
  struct wpcp_publish_item_t* item3 = wpcp_session_request_publish_item(g_session, subs3, id3);

  ck_assert(!wpcp_session_has_out_message(g_session));

  *wpcp_result_get_blocked_publish_item_target(result1) = item1;
  *wpcp_result_get_blocked_publish_item_target(result2) = item2;
  *wpcp_result_get_blocked_publish_item_target(result2) = item3;

  g_count = 0;
  struct wpcp_cbor_write_buffer_t* write_buffer2 = wpcp_publish_handle_publish(&subs2->publish_handle);
  wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer2, 2);
  struct wpcp_cbor_write_buffer_t* write_buffer3 = wpcp_publish_handle_publish(&subs3->publish_handle);
  wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer3, 3);
  ck_assert_int_eq(g_count, 0);

  ck_assert(!wpcp_session_has_out_message(g_session));

  wpcp_release_result(result1);

  ck_assert_int_ne(g_count, 0);
  g_count = 0;

  ck_assert(wpcp_session_has_out_message(g_session));
  struct wpcp_out_message_t* out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL); // TODO: check result
  wpcp_session_out_message_delete(out_message);

  ck_assert(wpcp_session_has_out_message(g_session));
  out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL); // TODO: check publish
  wpcp_session_out_message_delete(out_message);

  ck_assert(!wpcp_session_has_out_message(g_session));

  ck_assert_int_eq(g_count, 0);
  wpcp_release_result(result2);
  ck_assert_int_eq(g_count, 0);
  wpcp_release_result(result2);
  ck_assert_int_ne(g_count, 0);

  ck_assert(wpcp_session_has_out_message(g_session));
  out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL); // TODO: check result
  wpcp_session_out_message_delete(out_message);

  ck_assert(wpcp_session_has_out_message(g_session));
  out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL); // TODO: check publish
  wpcp_session_out_message_delete(out_message);

  ck_assert(!wpcp_session_has_out_message(g_session));

  wpcp_session_release_publish_item(g_session, item3);
  wpcp_session_release_publish_item(g_session, item2);
  wpcp_session_release_publish_item(g_session, item1);
}
END_TEST

START_TEST(result_no_block)
{
  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;
  ck_assert(!wpcp_session_has_out_message(g_session));

  struct wpcp_result_t* result1 = wpcp_request_result(g_session, 1, 1);
  struct wpcp_result_t* result2 = wpcp_request_result(g_session, 1, 1);

  uint32_t id1 = wpcp_session_get_next_free_publish_id(g_session);
  struct wpcp_subscription_t* subs1 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  struct wpcp_publish_item_t* item1 = wpcp_session_request_publish_item(g_session, subs1, id1);
/*
  uint32_t id2 = wpcp_session_get_next_free_publish_id(g_session);
  struct wpcp_subscription_t* subs2 = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  struct wpcp_publish_item_t* item2 = wpcp_session_request_publish_item(g_session, subs2, id2);

  uint32_t id3 = wpcp_session_get_next_free_publish_id(g_session);
  struct wpcp_subscription_t* subs3 = wpcp_request_subscription(&g_wpcp_internal, 1, NULL, 0);
  struct wpcp_publish_item_t* item3 = wpcp_session_request_publish_item(g_session, subs3, id3);
*/
  ck_assert(!wpcp_session_has_out_message(g_session));

/*
  ck_assert(wpcp_result_block_publish_item(result1, item1));
  ck_assert(wpcp_result_block_publish_item(result2, item2));
  ck_assert(wpcp_result_block_publish_item(result2, item3));
*/

  g_count = 0;
  struct wpcp_cbor_write_buffer_t* write_buffer1 = wpcp_publish_handle_publish(&subs1->publish_handle);
  wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer1, 1);
  ck_assert_int_ne(g_count, 0);

  ck_assert(wpcp_session_has_out_message(g_session));

  g_count = 0;
  wpcp_release_result(result1);
  ck_assert_int_ne(g_count, 0);

  g_count = 0;
  wpcp_release_result(result2);
  ck_assert_int_ne(g_count, 0);

  struct wpcp_cbor_write_buffer_t* write_buffer2 = wpcp_publish_handle_publish(&subs1->publish_handle);
  wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer2, 2);

  wpcp_session_release_publish_item(g_session, item1);

  ck_assert(wpcp_session_has_out_message(g_session));
  struct wpcp_out_message_t* out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL); // TODO: check publish
  wpcp_session_out_message_delete(out_message);

  ck_assert(wpcp_session_has_out_message(g_session));
  out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL); // TODO: check result1
  wpcp_session_out_message_delete(out_message);

  ck_assert(wpcp_session_has_out_message(g_session));
  out_message = wpcp_session_out_message_create(g_session);
  ck_assert_ptr_ne(out_message, NULL); // TODO: check result2
  wpcp_session_out_message_delete(out_message);

  ck_assert(!wpcp_session_has_out_message(g_session));
}
END_TEST

START_TEST(result_release_after_delete_session)
{
  g_wpcp_internal.wpcp.has_out_message.cb = has_out_message_cb;

  struct wpcp_result_t* result1 = wpcp_request_result(g_session, 1, 1);
  struct wpcp_result_t* result2 = wpcp_request_result(g_session, 2, 1);

  uint32_t id = wpcp_session_get_next_free_publish_id(g_session);
  struct wpcp_subscription_t* subs = wpcp_request_subscription(&g_wpcp_internal, 0, NULL, 0);
  struct wpcp_publish_item_t* item = wpcp_session_request_publish_item(g_session, subs, id);

  *wpcp_result_get_blocked_publish_item_target(result2) = item;

  struct wpcp_cbor_write_buffer_t* write_buffer = wpcp_publish_handle_publish(&subs->publish_handle);
  wpcp_cbor_write_buffer_write_unsigned_integer(write_buffer, 1);

  wpcp_session_release_publish_item(g_session, item);

  wpcp_session_delete(g_session);
  g_session = NULL;

  wpcp_release_result(result1);
  wpcp_release_result(result2);
}
END_TEST

START_TEST(dummy_unsubscrible_result)
{
  struct wpcp_session_t* session = wpcp_session_create(&g_wpcp_internal.wpcp, NULL);
  struct wpcp_result_t* result = wpcp_convert_session_to_dummy_unsubscrible_result(session, 2);

  ck_assert(wpcp_is_dummy_unsubscrible_result(result));
  wpcp_release_dummy_unsubscribe_result(result);
  wpcp_release_dummy_unsubscribe_result(result);
}
END_TEST


TCase* testcase_session(void)
{
  TCase* ret = tcase_create ("Session");
  tcase_add_checked_fixture(ret, tc_setup, tc_teardown);

  tcase_add_test (ret, subscription_request_release);
  tcase_add_test (ret, subscription_realloc);
  tcase_add_test (ret, subscription_malloc_fail);
  tcase_add_test (ret, subscription_usage_count);
  tcase_add_test (ret, subscription_user);
  tcase_add_test (ret, subscription_type);
  tcase_add_test (ret, subscription_publish_item);
  tcase_add_test (ret, subscription_max_publish_item);
  tcase_add_test (ret, subscription_release_publish_item_via_session);
  tcase_add_test (ret, subscription_publish_item_malloc_fail);
  tcase_add_test (ret, get_subscription_count);
  tcase_add_test (ret, iterate_subscriptions);
  tcase_add_test (ret, session_out_message);
  tcase_add_test (ret, session_out_message_malloc_fail);
  tcase_add_test (ret, subscription_publish_item_publish);
  tcase_add_test (ret, subscription_publish_item_publish_malloc_fail);
  tcase_add_test (ret, subscription_publish_item_publish_two_sessions);
  tcase_add_test (ret, result);
  tcase_add_test (ret, result_subresult);
  tcase_add_test (ret, result_return);
  tcase_add_test (ret, result_malloc_fail);
  tcase_add_test (ret, result_block);
  tcase_add_test (ret, result_block_multi);
  tcase_add_test (ret, result_no_block);
  tcase_add_test (ret, result_release_after_delete_session);
  tcase_add_test (ret, dummy_unsubscrible_result);

  return ret;
}
