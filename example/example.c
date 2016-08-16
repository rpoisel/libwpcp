#include <wpcp_lws_main.h>

#include <assert.h>
#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif
#include <stdlib.h>
#include <string.h>

struct example_process_image_item_t {
  const char* id;
  struct wpcp_publish_handle_t* publish_handle;
  struct wpcp_value_t value;
  double timestamp;
};

struct example_process_image_item_t example_process_image[] = {
  { "var1", NULL, { WPCP_VALUE_TYPE_UINT64, { 1 }, { NULL }, NULL }, 0.0 },
  { "var2", NULL, { WPCP_VALUE_TYPE_UINT64, { 2 }, { NULL }, NULL }, 0.0 },
  { "var3", NULL, { WPCP_VALUE_TYPE_UINT64, { 3 }, { NULL }, NULL }, 0.0 },
  { "var4", NULL, { WPCP_VALUE_TYPE_UINT64, { 4 }, { NULL }, NULL }, 0.0 }
};


struct example_process_image_item_t* find_process_image_item(const struct wpcp_value_t* id)
{
  size_t i;

  if (!id || id->type != WPCP_VALUE_TYPE_TEXT_STRING)
    return NULL;

  for (i = 0; i < sizeof(example_process_image) / sizeof(example_process_image[0]); ++i) {
    if (strlen(example_process_image[i].id) != id->value.length)
      continue;
    if (!memcmp(example_process_image[i].id, id->data.text_string, id->value.length))
      return &example_process_image[i];
  }

  return NULL;
}

static double time_as_doble(void)
{
#ifdef _WIN32
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000ULL
  FILETIME filetime;
  ULARGE_INTEGER datetime;

#ifdef _WIN32_WCE
  GetCurrentFT(&filetime);
#else
  GetSystemTimeAsFileTime(&filetime);
#endif

  /*
   * As per Windows documentation for FILETIME, copy the resulting FILETIME structure to a
   * ULARGE_INTEGER structure using memcpy (using memcpy instead of direct assignment can
   * prevent alignment faults on 64-bit Windows).
   */
  memcpy(&datetime, &filetime, sizeof(datetime));

  /* Windows file times are in 100s of nanoseconds. */
  return ((double) (datetime.QuadPart - DELTA_EPOCH_IN_MICROSECS * 10)) / 10000;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((double) tv.tv_sec) * 1000 + ((double) tv.tv_usec) / 1000;
#endif
}


static void browse(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id)
{
  size_t i;
  (void)user;

  if (!id || id->type != WPCP_VALUE_TYPE_TEXT_STRING || id->value.length) {
    wpcp_return_browse(result, NULL, 0);
    return;
  }

  wpcp_return_browse(result, NULL, sizeof(example_process_image) / sizeof(example_process_image[0]));

  for (i = 0; i < sizeof(example_process_image) / sizeof(example_process_image[0]); ++i) {
    struct wpcp_value_t item_id;
    const char* name = example_process_image[i].id;
    uint32_t name_length = strlen(name);

    item_id.type = WPCP_VALUE_TYPE_TEXT_STRING;
    item_id.value.length = name_length;
    item_id.data.text_string = name;

    wpcp_return_browse_item(result, &item_id, name, name_length, name, name_length, NULL, 0, NULL, NULL, 0);
  }
}

static void read_data(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id)
{
  struct example_process_image_item_t* process_image_item = find_process_image_item(id);

  (void) user;

  if (process_image_item) {
    wpcp_return_read_data(result, NULL, &process_image_item->value, process_image_item->timestamp, 0, NULL, 0);
  } else {
    const char error_message[] = "id not found";
    struct wpcp_value_t diagnostic_info;
    diagnostic_info.type = WPCP_VALUE_TYPE_TEXT_STRING;
    diagnostic_info.data.text_string = error_message;
    diagnostic_info.value.length = sizeof(error_message) - 1;

    wpcp_return_read_data(result, &diagnostic_info, NULL, 0.0, 0, NULL, 0);
  }
}

static void write_data(void* user, struct wpcp_result_t* result, const struct wpcp_value_t* id, const struct wpcp_value_t* value)
{
  struct example_process_image_item_t* process_image_item = find_process_image_item(id);

  (void) user;

  if (process_image_item) {
    if (!value || value->type == WPCP_VALUE_TYPE_BYTE_STRING || value->type == WPCP_VALUE_TYPE_TEXT_STRING || value->type == WPCP_VALUE_TYPE_ARRAY || value->type == WPCP_VALUE_TYPE_MAP) {

      const char error_message[] = "invalid value";
      struct wpcp_value_t diagnostic_info;
      diagnostic_info.type = WPCP_VALUE_TYPE_TEXT_STRING;
      diagnostic_info.data.text_string = error_message;
      diagnostic_info.value.length = sizeof(error_message) - 1;

      wpcp_return_write_data(result, &diagnostic_info, false);
    } else {
      process_image_item->value = *value;
      process_image_item->timestamp = time_as_doble();
      wpcp_return_write_data(result, NULL, true);
      if (process_image_item->publish_handle)
        wpcp_publish_data(process_image_item->publish_handle, &process_image_item->value, process_image_item->timestamp, 0, NULL, 0);
    }
  } else {
    const char error_message[] = "id not found";
    struct wpcp_value_t diagnostic_info;
    diagnostic_info.type = WPCP_VALUE_TYPE_TEXT_STRING;
    diagnostic_info.data.text_string = error_message;
    diagnostic_info.value.length = sizeof(error_message) - 1;

    wpcp_return_write_data(result, &diagnostic_info, false);
  }
}

static void subscribe_data(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription, const struct wpcp_value_t* id)
{
  struct example_process_image_item_t* process_image_item = find_process_image_item(id);

  (void) user;

  if (process_image_item) {
    struct wpcp_publish_handle_t* publish_handle;
    size_t usage_count = wpcp_subscription_get_usage_count(subscription);

    if (!usage_count) {
      assert(!wpcp_subscription_get_user(subscription));
      wpcp_subscription_set_user(subscription, process_image_item);
    } else {
      assert(wpcp_subscription_get_user(subscription) == process_image_item);
    }

    publish_handle = wpcp_return_subscribe_accept(result, NULL, subscription);

    if (!usage_count) {
      assert(!process_image_item->publish_handle);
      process_image_item->publish_handle = publish_handle;
    } else {
      assert(process_image_item->publish_handle == publish_handle);
    }
  } else {
    const char error_message[] = "id not found";
    struct wpcp_value_t diagnostic_info;
    diagnostic_info.type = WPCP_VALUE_TYPE_TEXT_STRING;
    diagnostic_info.data.text_string = error_message;
    diagnostic_info.value.length = sizeof(error_message) - 1;

    wpcp_return_subscribe_reject(result, &diagnostic_info, subscription);
  }
}

static void unsubscribe(void* user, struct wpcp_result_t* result, struct wpcp_subscription_t* subscription)
{
  size_t usage_count = wpcp_subscription_get_usage_count(subscription);
  struct example_process_image_item_t* process_image_item = wpcp_subscription_get_user(subscription);

  (void) user;

  if (!usage_count)
    process_image_item->publish_handle = NULL;

  wpcp_return_unsubscribe(result, NULL, subscription);
}

static void republish(void* user, struct wpcp_publish_handle_t* publish_handle, struct wpcp_subscription_t* subscription)
{
  struct example_process_image_item_t* process_image_item = wpcp_subscription_get_user(subscription);

  (void) user;

  wpcp_publish_data(publish_handle, &process_image_item->value, process_image_item->timestamp, 0, NULL, 0);
  wpcp_return_republish(publish_handle);
}

static void init(struct wpcp_lws_main_t* lws)
{
  lws->options.browse.cb = browse;
  lws->options.read_data.cb = read_data;
  lws->options.write_data.cb = write_data;
  lws->options.subscribe_data.cb = subscribe_data;
  lws->options.unsubscribe.cb = unsubscribe;
  lws->options.republish.cb = republish;
}

static void cleanup(struct wpcp_lws_main_t* lws)
{
  (void) lws;
}

WPCP_LWS_MAIN(init, cleanup)
