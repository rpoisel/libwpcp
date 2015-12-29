#include "test.h"
#include "../src/wpcp_cbor.h"
#include <math.h>

//static struct wpcp_cbor_read_buffer_t g_read_buffer;

static void tc_setup(void)
{
  testcase_setup();
}

static void tc_teardown(void)
{
  testcase_teardown();
}

START_TEST(empty)
{
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = NULL;
  buffer.end = NULL;

  uint32_t value;
  ck_assert(!wpcp_cbor_read_buffer_read_unsigned_integer(&buffer, &value));
  ck_assert(!wpcp_cbor_read_buffer_read_text_string(&buffer, &value));
  ck_assert(!wpcp_cbor_read_buffer_read_array_header(&buffer, &value));
  ck_assert(!wpcp_cbor_read_buffer_read_map_header(&buffer, &value));
  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(array_header_wrong_type)
{
  uint8_t begin[] = { 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(!wpcp_cbor_read_buffer_read_array_header(&buffer, &length));
}
END_TEST

START_TEST(array_header_0)
{
  uint8_t begin[] = { 0x80 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(wpcp_cbor_read_buffer_read_array_header(&buffer, &length));
  ck_assert_int_eq(length, 0);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(array_header_1)
{
  uint8_t begin[] = { 0x81 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(wpcp_cbor_read_buffer_read_array_header(&buffer, &length));
  ck_assert_int_eq(length, 1);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(array_header_10)
{
  uint8_t begin[] = { 0x8a };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(wpcp_cbor_read_buffer_read_array_header(&buffer, &length));
  ck_assert_int_eq(length, 10);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(array_header_23)
{
  uint8_t begin[] = { 0x97 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(wpcp_cbor_read_buffer_read_array_header(&buffer, &length));
  ck_assert_int_eq(length, 23);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(array_header_24)
{
  uint8_t begin[] = { 0x98, 0x18 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(wpcp_cbor_read_buffer_read_array_header(&buffer, &length));
  ck_assert_int_eq(length, 24);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(array_header_25)
{
  uint8_t begin[] = { 0x98, 0x19 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(wpcp_cbor_read_buffer_read_array_header(&buffer, &length));
  ck_assert_int_eq(length, 25);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(array_header_100)
{
  uint8_t begin[] = { 0x98, 0x64 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(wpcp_cbor_read_buffer_read_array_header(&buffer, &length));
  ck_assert_int_eq(length, 100);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(array_header_255)
{
  uint8_t begin[] = { 0x98, 0xff };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(wpcp_cbor_read_buffer_read_array_header(&buffer, &length));
  ck_assert_int_eq(length, 255);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(array_header_256)
{
  uint8_t begin[] = { 0x99, 0x01, 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(wpcp_cbor_read_buffer_read_array_header(&buffer, &length));
  ck_assert_int_eq(length, 256);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(array_header_1000)
{
  uint8_t begin[] = { 0x99, 0x03, 0xe8 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(wpcp_cbor_read_buffer_read_array_header(&buffer, &length));
  ck_assert_int_eq(length, 1000);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(array_header_1000000)
{
  uint8_t begin[] = { 0x9a, 0x00, 0x0f, 0x42, 0x40 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(wpcp_cbor_read_buffer_read_array_header(&buffer, &length));
  ck_assert_int_eq(length, 1000000);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(map_header_0)
{
  uint8_t begin[] = { 0xa0 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(wpcp_cbor_read_buffer_read_map_header(&buffer, &length));
  ck_assert_int_eq(length, 0);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(map_header_1)
{
  uint8_t begin[] = { 0xa1 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(wpcp_cbor_read_buffer_read_map_header(&buffer, &length));
  ck_assert_int_eq(length, 1);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(map_header_wrong_type)
{
  uint8_t begin[] = { 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(!wpcp_cbor_read_buffer_read_map_header(&buffer, &length));
}
END_TEST

START_TEST(unsigned_integer_0)
{
  uint8_t begin[] = { 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t value;
  ck_assert(wpcp_cbor_read_buffer_read_unsigned_integer(&buffer, &value));
  ck_assert_int_eq(value, 0);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(unsigned_integer_1)
{
  uint8_t begin[] = { 0x01 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t value;
  ck_assert(wpcp_cbor_read_buffer_read_unsigned_integer(&buffer, &value));
  ck_assert_int_eq(value, 1);

  ck_assert_ptr_eq(buffer.begin, buffer.end);
}
END_TEST

START_TEST(unsigned_integer_wrong_type)
{
  uint8_t begin[] = { 0x20 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t value;
  ck_assert(!wpcp_cbor_read_buffer_read_unsigned_integer(&buffer, &value));
}
END_TEST

START_TEST(unsigned_integer_invalid_size_1)
{
  uint8_t begin[] = { 0x18 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t value;
  ck_assert(!wpcp_cbor_read_buffer_read_unsigned_integer(&buffer, &value));
}
END_TEST

START_TEST(unsigned_integer_invalid_size_2)
{
  uint8_t begin[] = { 0x19 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t value;
  ck_assert(!wpcp_cbor_read_buffer_read_unsigned_integer(&buffer, &value));
}
END_TEST

START_TEST(unsigned_integer_invalid_size_4)
{
  uint8_t begin[] = { 0x1a };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t value;
  ck_assert(!wpcp_cbor_read_buffer_read_unsigned_integer(&buffer, &value));
}
END_TEST

START_TEST(unsigned_integer_invalid_size_8)
{
  uint8_t begin[] = { 0x1b };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t value;
  ck_assert(!wpcp_cbor_read_buffer_read_unsigned_integer(&buffer, &value));
}
END_TEST

START_TEST(text_string_0)
{
  uint8_t begin[] = { 0x60 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  const char* str = wpcp_cbor_read_buffer_read_text_string(&buffer, &length);
  ck_assert_int_eq(length, 0);
  ck_assert_ptr_ne(str, NULL);
}
END_TEST

START_TEST(text_string_1)
{
  uint8_t begin[] = { 0x61, 0x61 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  const char* str = wpcp_cbor_read_buffer_read_text_string(&buffer, &length);
  ck_assert_int_eq(length, 1);
  ck_assert_int_eq(str[0], 'a');
}
END_TEST

START_TEST(text_string_wrong_type)
{
  uint8_t begin[] = { 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  uint32_t length;
  ck_assert(!wpcp_cbor_read_buffer_read_text_string(&buffer, &length));
}
END_TEST

START_TEST(value_unsigned_integer_0)
{
  uint8_t begin[] = { 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(value->value.uint, 0);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_unsigned_integer_1)
{
  uint8_t begin[] = { 0x01 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(value->value.uint, 1);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_unsigned_integer_10)
{
  uint8_t begin[] = { 0x0a };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(value->value.uint, 10);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_unsigned_integer_100)
{
  uint8_t begin[] = { 0x18, 0x64 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(value->value.uint, 100);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_unsigned_integer_1000)
{
  uint8_t begin[] = { 0x19, 0x03, 0xe8 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(value->value.uint, 1000);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_unsigned_integer_1000000)
{
  uint8_t begin[] = { 0x1a, 0x00, 0x0f, 0x42, 0x40 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(value->value.uint, 1000000);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_unsigned_integer_1000000000000)
{
  uint8_t begin[] = { 0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(value->value.uint, 1000000000000);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_unsigned_integer_invalid_18)
{
  uint8_t begin[] = { 0x18 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_unsigned_integer_invalid_19)
{
  uint8_t begin[] = { 0x19 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_unsigned_integer_invalid_1a)
{
  uint8_t begin[] = { 0x1a };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_unsigned_integer_invalid_1b)
{
  uint8_t begin[] = { 0x1b };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_unsigned_integer_invalid_1c)
{
  uint8_t begin[] = { 0x1c };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_unsigned_integer_invalid_indefinite)
{
  uint8_t begin[] = { 0x1f };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST


START_TEST(value_negativ_integer_1)
{
  uint8_t begin[] = { 0x20 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_INT64);
  ck_assert_int_eq(value->value.sint, -1);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_negativ_integer_100)
{
  uint8_t begin[] = { 0x38, 0x63 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_INT64);
  ck_assert_int_eq(value->value.sint, -100);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_negativ_integer_1000)
{
  uint8_t begin[] = { 0x39, 0x03, 0xe7 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_INT64);
  ck_assert_int_eq(value->value.sint, -1000);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_negativ_integer_invalid_indefinite)
{
  uint8_t begin[] = { 0x3f };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_byte_string_0)
{
  uint8_t begin[] = { 0x40 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_BYTE_STRING);
  ck_assert_int_eq(value->value.length, 0);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_byte_string_4)
{
  uint8_t begin[] = { 0x44, 0x01, 0x02, 0x03, 0x04 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_BYTE_STRING);
  ck_assert_int_eq(value->value.length, 4);
  const uint8_t* byte_string = value->data.byte_string;
  ck_assert_int_eq(byte_string[0], 0x01);
  ck_assert_int_eq(byte_string[1], 0x02);
  ck_assert_int_eq(byte_string[2], 0x03);
  ck_assert_int_eq(byte_string[3], 0x04);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_byte_string_invalid)
{
  uint8_t begin[] = { 0x41 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_byte_string_indefinite_0)
{
  uint8_t begin[] = { 0x5f, 0xff };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_BYTE_STRING);
  ck_assert_int_eq(value->value.length, 0);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_byte_string_indefinite_000)
{
  uint8_t begin[] = { 0x5f, 0x40, 0x5f, 0x40, 0x40, 0xff, 0x40, 0xff };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_BYTE_STRING);
  ck_assert_int_eq(value->value.length, 0);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_byte_string_indefinite)
{
  uint8_t begin[] = { 0x5f, 0x41, 0x01, 0x5f, 0x41, 0x02, 0x41, 0x03, 0xff, 0x42, 0x04, 0x05, 0x40, 0xff };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_BYTE_STRING);
  ck_assert_int_eq(value->value.length, 5);
  const uint8_t* byte_string = value->data.byte_string;
  ck_assert_int_eq(byte_string[0], 0x01);
  ck_assert_int_eq(byte_string[1], 0x02);
  ck_assert_int_eq(byte_string[2], 0x03);
  ck_assert_int_eq(byte_string[3], 0x04);
  ck_assert_int_eq(byte_string[4], 0x05);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_byte_string_indefinite_invalid_length)
{
  uint8_t begin[] = { 0x5f };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_byte_string_indefinite_invalid_type)
{
  uint8_t begin[] = { 0x5f, 0x00, 0xff };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_byte_string_indefinite_malloc_fail)
{
  uint8_t begin[] = { 0x5f, 0x41, 0x01, 0xff };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  int malloc_fails[] = {3, 0};
  testcase_set_malloc_fails(malloc_fails);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_text_string_0)
{
  uint8_t begin[] = { 0x60 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_TEXT_STRING);
  ck_assert_int_eq(value->value.length, 0);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_text_string_1)
{
  uint8_t begin[] = { 0x61, 0x61 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_TEXT_STRING);
  ck_assert_int_eq(value->value.length, 1);
  const uint8_t* text_string = value->data.byte_string;
  ck_assert_int_eq(text_string[0], 'a');
  wpcp_value_free(value);
}
END_TEST
START_TEST(value_text_string_invalid)
{
  uint8_t begin[] = { 0x61 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_array_0)
{
  uint8_t begin[] = { 0x80 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_ARRAY);
  ck_assert_int_eq(value->value.length, 0);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_array_3)
{
  uint8_t begin[] = { 0x83, 0x01, 0x02, 0x03 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_ARRAY);
  ck_assert_int_eq(value->value.length, 3);
  struct wpcp_value_t* value1 = value->data.first_child;
  ck_assert_int_eq(value1->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(value1->value.uint, 1);
  struct wpcp_value_t* value2 = value1->next;
  ck_assert_int_eq(value2->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(value2->value.uint, 2);
  struct wpcp_value_t* value3 = value2->next;
  ck_assert_int_eq(value3->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(value3->value.uint, 3);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_array_invalid)
{
  uint8_t begin[] = { 0x81 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_array_invalid_indefinite)
{
  uint8_t begin[] = { 0x9f };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_map_0)
{
  uint8_t begin[] = { 0xa0 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_MAP);
  ck_assert_int_eq(value->value.length, 0);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_map_2)
{
  uint8_t begin[] = { 0xa2, 0x01, 0x02, 0x03, 0x04 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_MAP);
  ck_assert_int_eq(value->value.length, 2);
  struct wpcp_value_t* key1 = value->data.first_child;
  ck_assert_int_eq(key1->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(key1->value.uint, 1);
  struct wpcp_value_t* value1 = key1->next;
  ck_assert_int_eq(value1->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(value1->value.uint, 2);
  struct wpcp_value_t* key2 = value1->next;
  ck_assert_int_eq(key2->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(key2->value.uint, 3);
  struct wpcp_value_t* value2 = key2->next;
  ck_assert_int_eq(value2->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(value2->value.uint, 4);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_map_invalid)
{
  uint8_t begin[] = { 0xa1, 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_map_invalid_indefinite)
{
  uint8_t begin[] = { 0xbf, 0x00, 0xff };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_tag)
{
  uint8_t begin[] = { 0xc1, 0x1a, 0x51, 0x4b, 0x67, 0xb0 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_TAG);
  ck_assert_int_eq(value->value.uint, 1);
  struct wpcp_value_t* first_child = value->data.first_child;
  ck_assert_int_eq(first_child->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(first_child->value.uint, 1363896240);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_tag_invalid)
{
  uint8_t begin[] = { 0xc1, 0x18 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_tag_invalid_indefinite)
{
  uint8_t begin[] = { 0xdf };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_simple_value_16)
{
  uint8_t begin[] = { 0xf0 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_SIMPLE_VALUE);
  ck_assert_int_eq(value->value.uint, 16);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_simple_value_255)
{
  uint8_t begin[] = { 0xf8, 0xff };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_SIMPLE_VALUE);
  ck_assert_int_eq(value->value.uint, 255);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_false)
{
  uint8_t begin[] = { 0xf4 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_FALSE);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_true)
{
  uint8_t begin[] = { 0xf5 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_TRUE);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_null)
{
  uint8_t begin[] = { 0xf6 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_NULL);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_undefined)
{
  uint8_t begin[] = { 0xf7 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_UNDEFINED);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_half_0000)
{
  uint8_t begin[] = { 0xf9, 0x00, 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_FLOAT);
  ck_assert(value->value.flt == 0.0f);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_half_8000)
{
  uint8_t begin[] = { 0xf9, 0x80, 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_FLOAT);
  ck_assert(value->value.flt == -0.0f);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_half_3c00)
{
  uint8_t begin[] = { 0xf9, 0x3c, 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_FLOAT);
  ck_assert(value->value.flt == 1.0f);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_half_3e00)
{
  uint8_t begin[] = { 0xf9, 0x3e, 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_FLOAT);
  ck_assert(value->value.flt == 1.5f);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_half_7bff)
{
  uint8_t begin[] = { 0xf9, 0x7b, 0xff };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_FLOAT);
  ck_assert(value->value.flt == 65504.0f);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_half_0001)
{
  uint8_t begin[] = { 0xf9, 0x00, 0x01 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_FLOAT);
  ck_assert(value->value.flt == 5.960464477539063e-8f);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_half_0400)
{
  uint8_t begin[] = { 0xf9, 0x04, 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_FLOAT);
  ck_assert(value->value.flt == 0.00006103515625f);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_half_c400)
{
  uint8_t begin[] = { 0xf9, 0xc4, 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_FLOAT);
  ck_assert(value->value.flt == -4.0f);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_half_7c00)
{
  uint8_t begin[] = { 0xf9, 0x7c, 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_FLOAT);
  ck_assert(isinf(value->value.flt));
  ck_assert(value->value.flt > 0);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_half_7e00)
{
  uint8_t begin[] = { 0xf9, 0x7e, 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_FLOAT);
  ck_assert(isnan(value->value.flt));
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_half_fc00)
{
  uint8_t begin[] = { 0xf9, 0xfc, 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_FLOAT);
  ck_assert(isinf(value->value.flt));
  ck_assert(value->value.flt < 0);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_float)
{
  uint8_t begin[] = { 0xfa, 0x47, 0xc3, 0x50, 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_FLOAT);
  ck_assert(value->value.flt == 100000.0f);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_double)
{
  uint8_t begin[] = { 0xfb, 0x3f, 0xf1, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9a };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_value_t* value = wpcp_cbor_read_buffer_read_wpcp_value(&buffer);
  ck_assert_int_eq(value->type, WPCP_VALUE_TYPE_DOUBLE);
  ck_assert(value->value.dbl == 1.1);
  wpcp_value_free(value);
}
END_TEST

START_TEST(value_break)
{
  uint8_t begin[] = { 0xff };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(value_malloc_fail)
{
  uint8_t begin[] = { 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  int malloc_fails[] = {1, 0};
  testcase_set_malloc_fails(malloc_fails);

  ck_assert(!wpcp_cbor_read_buffer_read_wpcp_value(&buffer));
}
END_TEST

START_TEST(map_invalid_type)
{
  uint8_t begin[] = { 0x00 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_key_value_pair_t* additional;
  uint32_t additional_count;


  ck_assert(!wpcp_cbor_read_buffer_read_map_items(&buffer, NULL, 0, &additional, &additional_count));
  wpcp_clear_map_items(NULL, 0, additional, additional_count);
}
END_TEST

START_TEST(map_0)
{
  uint8_t begin[] = { 0xa0 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_key_value_pair_t* additional;
  uint32_t additional_count;

  ck_assert(wpcp_cbor_read_buffer_read_map_items(&buffer, NULL, 0, &additional, &additional_count));
  wpcp_clear_map_items(NULL, 0, additional, additional_count);
}
END_TEST

START_TEST(map_invalid_pair)
{
  uint8_t begin[] = { 0xa1, 0x60 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_key_value_pair_t* additional;
  uint32_t additional_count;

  ck_assert(!wpcp_cbor_read_buffer_read_map_items(&buffer, NULL, 0, &additional, &additional_count));
  wpcp_clear_map_items(NULL, 0, additional, additional_count);
}
END_TEST

START_TEST(map_1)
{
  uint8_t begin[] = { 0xa1, 0x60, 0x01 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_key_value_pair_t* additional;
  uint32_t additional_count;

  ck_assert(wpcp_cbor_read_buffer_read_map_items(&buffer, NULL, 0, &additional, &additional_count));
  ck_assert_int_eq(additional_count, 1);
  ck_assert_int_eq(additional[0].key_length, 0);
  ck_assert_int_eq(additional[0].value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(additional[0].value->value.uint, 1);
  wpcp_clear_map_items(NULL, 0, additional, additional_count);
}
END_TEST

START_TEST(map_2_invalid_key_type)
{
  uint8_t begin[] = { 0xa2, 0x60, 0x01, 0x02, 0x03 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_key_value_pair_t* additional;
  uint32_t additional_count;

  ck_assert(!wpcp_cbor_read_buffer_read_map_items(&buffer, NULL, 0, &additional, &additional_count));
  wpcp_clear_map_items(NULL, 0, additional, additional_count);
}
END_TEST

START_TEST(map_2)
{
  uint8_t begin[] = { 0xa2, 0x60, 0x01, 0x60, 0x02 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_key_value_pair_t map[] = { { NULL, 0, NULL } };
  struct wpcp_key_value_pair_t* additional;
  uint32_t additional_count;

  ck_assert(wpcp_cbor_read_buffer_read_map_items(&buffer, map, 1, &additional, &additional_count));
  ck_assert_int_eq(additional_count, 0);
  ck_assert_int_eq(map[0].value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(map[0].value->value.uint, 2);
  wpcp_clear_map_items(map, 1, additional, additional_count);
}
END_TEST

START_TEST(map_1_different_key_length)
{
  uint8_t begin[] = { 0xa1, 0x60, 0x01 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_key_value_pair_t map[] = { { " ", 1, NULL } };
  struct wpcp_key_value_pair_t* additional;
  uint32_t additional_count;

  ck_assert(wpcp_cbor_read_buffer_read_map_items(&buffer, map, 1, &additional, &additional_count));
  ck_assert_ptr_eq(map[0].value, NULL);
  ck_assert_int_eq(additional_count, 1);
  ck_assert_int_eq(additional[0].key_length, 0);
  ck_assert_int_eq(additional[0].value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(additional[0].value->value.uint, 1);
  wpcp_clear_map_items(map, 1, additional, additional_count);
}
END_TEST

START_TEST(map_1_different_key)
{
  uint8_t begin[] = { 0xa1, 0x62, 0x61, 0x62, 0x01 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_key_value_pair_t map[] = { { "aa", 2, NULL } };
  struct wpcp_key_value_pair_t* additional;
  uint32_t additional_count;

  ck_assert(wpcp_cbor_read_buffer_read_map_items(&buffer, map, 1, &additional, &additional_count));
  ck_assert_ptr_eq(map[0].value, NULL);
  ck_assert_int_eq(additional_count, 1);
  ck_assert_int_eq(additional[0].key_length, 2);
  ck_assert_int_eq(additional[0].key[0], 0x61);
  ck_assert_int_eq(additional[0].key[1], 0x62);
  ck_assert_int_eq(additional[0].value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(additional[0].value->value.uint, 1);
  wpcp_clear_map_items(map, 1, additional, additional_count);
}
END_TEST

START_TEST(map_1_same_key)
{
  uint8_t begin[] = { 0xa1, 0x62, 0x61, 0x61, 0x01 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_key_value_pair_t map[] = { { "aa", 2, NULL } };
  struct wpcp_key_value_pair_t* additional;
  uint32_t additional_count;

  ck_assert(wpcp_cbor_read_buffer_read_map_items(&buffer, map, 1, &additional, &additional_count));
  ck_assert_int_eq(map[0].value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(map[0].value->value.uint, 1);
  ck_assert_int_eq(additional_count, 0);
  wpcp_clear_map_items(map, 1, additional, additional_count);
}
END_TEST

START_TEST(map_additional)
{
  uint8_t begin[] = { 0xa2, 0x61, 0x61, 0x01, 0x61, 0x62, 0x02 };
  struct wpcp_cbor_read_buffer_t buffer;
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_key_value_pair_t map[] = { { NULL, 0, NULL } };
  struct wpcp_key_value_pair_t* additional;
  uint32_t additional_count;

  ck_assert(wpcp_cbor_read_buffer_read_map_items(&buffer, map, 1, &additional, &additional_count));
  ck_assert_ptr_eq(map[0].value, NULL);
  ck_assert_int_eq(additional_count, 2);
  ck_assert_int_eq(additional[0].key_length, 1);
  ck_assert_int_eq(additional[0].key[0], 0x61);
  ck_assert_int_eq(additional[0].value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(additional[0].value->value.uint, 1);

  ck_assert_int_eq(additional[1].key_length, 1);
  ck_assert_int_eq(additional[1].key[0], 0x62);
  ck_assert_int_eq(additional[1].value->type, WPCP_VALUE_TYPE_UINT64);
  ck_assert_int_eq(additional[1].value->value.uint, 2);

  wpcp_clear_map_items(map, 1, additional, additional_count);
}
END_TEST

START_TEST(map_malloc_fail)
{
  uint8_t begin[] = { 0xa2, 0x60, 0x01 };
  struct wpcp_cbor_read_buffer_t buffer;
  int malloc_fails[] = {2, 0};
  testcase_set_malloc_fails(malloc_fails);
  buffer.begin = begin;
  buffer.end = begin + sizeof(begin);

  struct wpcp_key_value_pair_t* additional;
  uint32_t additional_count;

  ck_assert(!wpcp_cbor_read_buffer_read_map_items(&buffer, NULL, 0, &additional, &additional_count));
  wpcp_clear_map_items(NULL, 0, additional, additional_count);
}
END_TEST

TCase* testcase_cbor_read_buffer(void)
{
  TCase* ret = tcase_create ("CBOR Read Buffer");
  tcase_add_checked_fixture(ret, tc_setup, tc_teardown);

  tcase_add_test (ret, empty);
  tcase_add_test (ret, array_header_0);
  tcase_add_test (ret, array_header_1);
  tcase_add_test (ret, array_header_10);
  tcase_add_test (ret, array_header_23);
  tcase_add_test (ret, array_header_24);
  tcase_add_test (ret, array_header_25);
  tcase_add_test (ret, array_header_100);
  tcase_add_test (ret, array_header_255);
  tcase_add_test (ret, array_header_256);
  tcase_add_test (ret, array_header_1000);
  tcase_add_test (ret, array_header_1000000);
  tcase_add_test (ret, array_header_wrong_type);
  tcase_add_test (ret, map_header_0);
  tcase_add_test (ret, map_header_1);
  tcase_add_test (ret, map_header_wrong_type);
  tcase_add_test (ret, unsigned_integer_0);
  tcase_add_test (ret, unsigned_integer_1);
  tcase_add_test (ret, unsigned_integer_wrong_type);
  tcase_add_test (ret, unsigned_integer_invalid_size_1);
  tcase_add_test (ret, unsigned_integer_invalid_size_2);
  tcase_add_test (ret, unsigned_integer_invalid_size_4);
  tcase_add_test (ret, unsigned_integer_invalid_size_8);
  tcase_add_test (ret, text_string_0);
  tcase_add_test (ret, text_string_1);
  tcase_add_test (ret, text_string_wrong_type);
  tcase_add_test (ret, value_unsigned_integer_0);
  tcase_add_test (ret, value_unsigned_integer_1);
  tcase_add_test (ret, value_unsigned_integer_10);
  tcase_add_test (ret, value_unsigned_integer_100);
  tcase_add_test (ret, value_unsigned_integer_1000);
  tcase_add_test (ret, value_unsigned_integer_1000000);
  tcase_add_test (ret, value_unsigned_integer_1000000000000);
  tcase_add_test (ret, value_unsigned_integer_invalid_18);
  tcase_add_test (ret, value_unsigned_integer_invalid_19);
  tcase_add_test (ret, value_unsigned_integer_invalid_1a);
  tcase_add_test (ret, value_unsigned_integer_invalid_1b);
  tcase_add_test (ret, value_unsigned_integer_invalid_1c);
  tcase_add_test (ret, value_unsigned_integer_invalid_indefinite);
  tcase_add_test (ret, value_negativ_integer_1);
  tcase_add_test (ret, value_negativ_integer_100);
  tcase_add_test (ret, value_negativ_integer_1000);
  tcase_add_test (ret, value_negativ_integer_invalid_indefinite);
  tcase_add_test (ret, value_byte_string_0);
  tcase_add_test (ret, value_byte_string_4);
  tcase_add_test (ret, value_byte_string_invalid);
  tcase_add_test (ret, value_byte_string_indefinite_0);
  tcase_add_test (ret, value_byte_string_indefinite_000);
  tcase_add_test (ret, value_byte_string_indefinite);
  tcase_add_test (ret, value_byte_string_indefinite_invalid_length);
  tcase_add_test (ret, value_byte_string_indefinite_invalid_type);
  tcase_add_test (ret, value_byte_string_indefinite_malloc_fail);
  tcase_add_test (ret, value_text_string_0);
  tcase_add_test (ret, value_text_string_1);
  tcase_add_test (ret, value_text_string_invalid);
  tcase_add_test (ret, value_array_0);
  tcase_add_test (ret, value_array_3);
  tcase_add_test (ret, value_array_invalid);
  tcase_add_test (ret, value_array_invalid_indefinite);
  tcase_add_test (ret, value_map_0);
  tcase_add_test (ret, value_map_2);
  tcase_add_test (ret, value_map_invalid);
  tcase_add_test (ret, value_map_invalid_indefinite);
  tcase_add_test (ret, value_tag);
  tcase_add_test (ret, value_tag_invalid);
  tcase_add_test (ret, value_tag_invalid_indefinite);
  tcase_add_test (ret, value_false);
  tcase_add_test (ret, value_true);
  tcase_add_test (ret, value_null);
  tcase_add_test (ret, value_undefined);
  tcase_add_test (ret, value_simple_value_16);
  tcase_add_test (ret, value_simple_value_255);
  tcase_add_test (ret, value_half_0000);
  tcase_add_test (ret, value_half_0001);
  tcase_add_test (ret, value_half_0400);
  tcase_add_test (ret, value_half_3c00);
  tcase_add_test (ret, value_half_3e00);
  tcase_add_test (ret, value_half_7bff);
  tcase_add_test (ret, value_half_7c00);
  tcase_add_test (ret, value_half_7e00);
  tcase_add_test (ret, value_half_8000);
  tcase_add_test (ret, value_half_c400);
  tcase_add_test (ret, value_half_fc00);
  tcase_add_test (ret, value_float);
  tcase_add_test (ret, value_double);
  tcase_add_test (ret, value_break);
  tcase_add_test (ret, value_malloc_fail);
  tcase_add_test (ret, map_invalid_type);
  tcase_add_test (ret, map_0);
  tcase_add_test (ret, map_invalid_pair);
  tcase_add_test (ret, map_1);
  tcase_add_test (ret, map_2);
  tcase_add_test (ret, map_2_invalid_key_type);
  tcase_add_test (ret, map_malloc_fail);
  tcase_add_test (ret, map_1_different_key);
  tcase_add_test (ret, map_1_different_key_length);
  tcase_add_test (ret, map_1_same_key);
  tcase_add_test (ret, map_additional);

  return ret;
}
