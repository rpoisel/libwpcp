#include "test.h"
#include "../src/wpcp_cbor.h"

static char g_text_data[2048];
static const char* g_text = g_text_data;
static struct wpcp_cbor_write_buffer_t g_write_buffer;

static void tc_setup(void)
{
  size_t i;
  testcase_setup();
  wpcp_cbor_write_buffer_init(&g_write_buffer);

  for (i = 0; i < sizeof(g_text_data); ++i)
    g_text_data[i] = 'A' + i % 26;
  g_text = g_text_data;
}

static void tc_teardown(void)
{
  wpcp_cbor_write_buffer_clear(&g_write_buffer);
  testcase_teardown();
}

START_TEST(empty)
{
  uint8_t begin[1];
  uint8_t* end;

  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin);
}
END_TEST

START_TEST(array_header_0)
{
  uint8_t begin[1];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_array_header(&g_write_buffer, 0);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x80);
}
END_TEST

START_TEST(map_header_0)
{
  uint8_t begin[1];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_map_header(&g_write_buffer, 0);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xa0);
}
END_TEST

START_TEST(double_0)
{
  uint8_t begin[9];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_double(&g_write_buffer, 0.0);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xfb);
  ck_assert_int_eq(begin[1], 0x00);
  ck_assert_int_eq(begin[2], 0x00);
  ck_assert_int_eq(begin[3], 0x00);
  ck_assert_int_eq(begin[4], 0x00);
  ck_assert_int_eq(begin[5], 0x00);
  ck_assert_int_eq(begin[6], 0x00);
  ck_assert_int_eq(begin[7], 0x00);
  ck_assert_int_eq(begin[8], 0x00);
}
END_TEST

START_TEST(double_1_1)
{
  uint8_t begin[9];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_double(&g_write_buffer, 1.1);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xfb);
  ck_assert_int_eq(begin[1], 0x3f);
  ck_assert_int_eq(begin[2], 0xf1);
  ck_assert_int_eq(begin[3], 0x99);
  ck_assert_int_eq(begin[4], 0x99);
  ck_assert_int_eq(begin[5], 0x99);
  ck_assert_int_eq(begin[6], 0x99);
  ck_assert_int_eq(begin[7], 0x99);
  ck_assert_int_eq(begin[8], 0x9a);
}
END_TEST

START_TEST(array_header_1)
{
  uint8_t begin[1];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_array_header(&g_write_buffer, 1);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x81);
}
END_TEST

START_TEST(map_header_1)
{
  uint8_t begin[1];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_map_header(&g_write_buffer, 1);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xa1);
}
END_TEST

START_TEST(bool_false)
{
  uint8_t begin[1];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_boolean(&g_write_buffer, false);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xf4);
}
END_TEST

START_TEST(bool_true)
{
  uint8_t begin[1];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_boolean(&g_write_buffer, true);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xf5);
}
END_TEST

START_TEST(null)
{
  uint8_t begin[1];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_null(&g_write_buffer);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xf6);
}
END_TEST

START_TEST(text_string_0)
{
  uint8_t begin[1];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_text_string(&g_write_buffer, NULL, 0);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x60);
}
END_TEST

START_TEST(unsigned_integer_0)
{
  uint8_t begin[1];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_unsigned_integer(&g_write_buffer, 0);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x00);
}
END_TEST

START_TEST(unsigned_integer_1)
{
  uint8_t begin[1];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_unsigned_integer(&g_write_buffer, 1);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x01);
}
END_TEST

START_TEST(unsigned_integer_10)
{
  uint8_t begin[1];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_unsigned_integer(&g_write_buffer, 10);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x0a);
}
END_TEST

START_TEST(unsigned_integer_23)
{
  uint8_t begin[1];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_unsigned_integer(&g_write_buffer, 23);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x17);
}
END_TEST

START_TEST(unsigned_integer_24)
{
  uint8_t begin[2];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_unsigned_integer(&g_write_buffer, 24);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x18);
  ck_assert_int_eq(begin[1], 0x18);
}
END_TEST

START_TEST(unsigned_integer_25)
{
  uint8_t begin[2];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_unsigned_integer(&g_write_buffer, 25);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x18);
  ck_assert_int_eq(begin[1], 0x19);
}
END_TEST

START_TEST(unsigned_integer_100)
{
  uint8_t begin[2];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_unsigned_integer(&g_write_buffer, 100);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x18);
  ck_assert_int_eq(begin[1], 0x64);
}
END_TEST

START_TEST(unsigned_integer_255)
{
  uint8_t begin[2];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_unsigned_integer(&g_write_buffer, 255);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x18);
  ck_assert_int_eq(begin[1], 0xff);
}
END_TEST

START_TEST(unsigned_integer_256)
{
  uint8_t begin[3];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_unsigned_integer(&g_write_buffer, 256);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x19);
  ck_assert_int_eq(begin[1], 0x01);
  ck_assert_int_eq(begin[2], 0x00);
}
END_TEST

START_TEST(unsigned_integer_1000)
{
  uint8_t begin[3];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_unsigned_integer(&g_write_buffer, 1000);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x19);
  ck_assert_int_eq(begin[1], 0x03);
  ck_assert_int_eq(begin[2], 0xe8);
}
END_TEST

START_TEST(unsigned_integer_1000000)
{
  uint8_t begin[5];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_unsigned_integer(&g_write_buffer, 1000000);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x1a);
  ck_assert_int_eq(begin[1], 0x00);
  ck_assert_int_eq(begin[2], 0x0f);
  ck_assert_int_eq(begin[3], 0x42);
  ck_assert_int_eq(begin[4], 0x40);
}
END_TEST

START_TEST(text_string_1)
{
  uint8_t begin[2];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_text_string(&g_write_buffer, g_text, 1);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x61);
  ck_assert(!memcmp(g_text, begin + 1, 1));
}
END_TEST

START_TEST(text_string_10)
{
  uint8_t begin[11];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_text_string(&g_write_buffer, g_text, 10);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x6a);
  ck_assert(!memcmp(g_text, begin + 1, 10));
}
END_TEST

START_TEST(text_string_23)
{
  uint8_t begin[24];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_text_string(&g_write_buffer, g_text, 23);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x77);
  ck_assert(!memcmp(g_text, begin + 1, 23));
}
END_TEST

START_TEST(text_string_24)
{
  uint8_t begin[26];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_text_string(&g_write_buffer, g_text, 24);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x78);
  ck_assert_int_eq(begin[1], 0x18);
  ck_assert(!memcmp(g_text, begin + 2, 24));
}
END_TEST

START_TEST(text_string_25)
{
  uint8_t begin[27];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_text_string(&g_write_buffer, g_text, 25);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x78);
  ck_assert_int_eq(begin[1], 0x19);
  ck_assert(!memcmp(g_text, begin + 2, 25));
}
END_TEST

START_TEST(text_string_100)
{
  uint8_t begin[102];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_text_string(&g_write_buffer, g_text, 100);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x78);
  ck_assert_int_eq(begin[1], 0x64);
  ck_assert(!memcmp(g_text, begin + 2, 100));
}
END_TEST

START_TEST(text_string_1024)
{
  uint8_t begin[1027];
  uint8_t* end;



  wpcp_cbor_write_buffer_write_text_string(&g_write_buffer, g_text, 1024);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x79);
  ck_assert_int_eq(begin[1], 0x04);
  ck_assert_int_eq(begin[2], 0x00);
  ck_assert(!memcmp(g_text, begin + 3, 1024));
}
END_TEST

START_TEST(malloc_fail)
{
  uint8_t* end;
  int malloc_fails[] = {1, 0};
  testcase_set_malloc_fails(malloc_fails);
  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_UINT64;

  wpcp_cbor_write_buffer_write_text_string(&g_write_buffer, g_text, 100);
  value.value.uint = 0xf;
  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  value.value.uint = 0xff;
  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  value.value.uint = 0xffff;
  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  value.value.uint = 0xffffffff;
  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  value.value.uint = 0xffffffffffffffff;
  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(NULL, &g_write_buffer);

  ck_assert_ptr_eq(end, NULL);
}
END_TEST

START_TEST(value_uint_f)
{
  uint8_t begin[1];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_UINT64;
  value.value.uint = 0x0f;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x0f);
}
END_TEST

START_TEST(value_uint_ff)
{
  uint8_t begin[2];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_UINT64;
  value.value.uint = 0xff;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x18);
  ck_assert_int_eq(begin[1], 0xff);
}
END_TEST

START_TEST(value_uint_ffff)
{
  uint8_t begin[3];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_UINT64;
  value.value.uint = 0xffff;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x19);
  ck_assert_int_eq(begin[1], 0xff);
  ck_assert_int_eq(begin[2], 0xff);
}
END_TEST

START_TEST(value_uint_ffffffff)
{
  uint8_t begin[5];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_UINT64;
  value.value.uint = 0xffffffff;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x1a);
  ck_assert_int_eq(begin[1], 0xff);
  ck_assert_int_eq(begin[2], 0xff);
  ck_assert_int_eq(begin[3], 0xff);
  ck_assert_int_eq(begin[4], 0xff);
}
END_TEST

START_TEST(value_uint_ffffffffffffffff)
{
  uint8_t begin[9];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_UINT64;
  value.value.uint = 0xffffffffffffffff;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x1b);
  ck_assert_int_eq(begin[1], 0xff);
  ck_assert_int_eq(begin[2], 0xff);
  ck_assert_int_eq(begin[3], 0xff);
  ck_assert_int_eq(begin[4], 0xff);
  ck_assert_int_eq(begin[5], 0xff);
  ck_assert_int_eq(begin[6], 0xff);
  ck_assert_int_eq(begin[7], 0xff);
  ck_assert_int_eq(begin[8], 0xff);
}
END_TEST

START_TEST(value_sint_p100)
{
  uint8_t begin[2];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_INT64;
  value.value.sint = 100;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x18);
  ck_assert_int_eq(begin[1], 0x64);
}
END_TEST

START_TEST(value_sint_n100)
{
  uint8_t begin[2];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_INT64;
  value.value.sint = -100;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x38);
  ck_assert_int_eq(begin[1], 0x63);
}
END_TEST

START_TEST(value_byte_string)
{
  uint8_t begin[2];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_BYTE_STRING;
  value.value.length = 1;
  value.data.byte_string = "a";

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x41);
  ck_assert_int_eq(begin[1], 0x61);
}
END_TEST

START_TEST(value_text_string)
{
  uint8_t begin[2];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_TEXT_STRING;
  value.value.length = 1;
  value.data.text_string = "a";

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x61);
  ck_assert_int_eq(begin[1], 0x61);
}
END_TEST

START_TEST(value_array_empty)
{
  uint8_t begin[1];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_ARRAY;
  value.value.length = 0;
  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x80);
}
END_TEST

START_TEST(value_array_123)
{
  uint8_t begin[4];
  uint8_t* end;

  struct wpcp_value_t value;
  struct wpcp_value_t subvalue[3];
  value.type = WPCP_VALUE_TYPE_ARRAY;
  value.value.length = 3;
  value.data.first_child = &subvalue[0];
  subvalue[0].type = WPCP_VALUE_TYPE_UINT64;
  subvalue[0].value.uint = 1;
  subvalue[0].next = &subvalue[1];
  subvalue[1].type = WPCP_VALUE_TYPE_UINT64;
  subvalue[1].value.uint = 2;
  subvalue[1].next = &subvalue[2];
  subvalue[2].type = WPCP_VALUE_TYPE_UINT64;
  subvalue[2].value.uint = 3;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x83);
  ck_assert_int_eq(begin[1], 0x01);
  ck_assert_int_eq(begin[2], 0x02);
  ck_assert_int_eq(begin[3], 0x03);
}
END_TEST

START_TEST(value_array_in_array)
{
  uint8_t begin[8];
  uint8_t* end;

  struct wpcp_value_t value;
  struct wpcp_value_t subvalue[7];
  value.type = WPCP_VALUE_TYPE_ARRAY;
  value.value.length = 3;
  value.data.first_child = &subvalue[0];
  subvalue[0].type = WPCP_VALUE_TYPE_UINT64;
  subvalue[0].value.uint = 1;
  subvalue[0].next = &subvalue[1];
  subvalue[1].type = WPCP_VALUE_TYPE_ARRAY;
  subvalue[1].value.length = 2;
  subvalue[1].data.first_child = &subvalue[2];
  subvalue[1].next = &subvalue[4];
  subvalue[2].type = WPCP_VALUE_TYPE_UINT64;
  subvalue[2].value.uint = 2;
  subvalue[2].next = &subvalue[3];
  subvalue[3].type = WPCP_VALUE_TYPE_UINT64;
  subvalue[3].value.uint = 3;
  subvalue[4].type = WPCP_VALUE_TYPE_ARRAY;
  subvalue[4].value.length = 2;
  subvalue[4].data.first_child = &subvalue[5];
  subvalue[5].type = WPCP_VALUE_TYPE_UINT64;
  subvalue[5].value.uint = 4;
  subvalue[5].next = &subvalue[6];
  subvalue[6].type = WPCP_VALUE_TYPE_UINT64;
  subvalue[6].value.uint = 5;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x83);
  ck_assert_int_eq(begin[1], 0x01);
  ck_assert_int_eq(begin[2], 0x82);
  ck_assert_int_eq(begin[3], 0x02);
  ck_assert_int_eq(begin[4], 0x03);
  ck_assert_int_eq(begin[5], 0x82);
  ck_assert_int_eq(begin[6], 0x04);
  ck_assert_int_eq(begin[7], 0x05);
}
END_TEST

START_TEST(value_map_empty)
{
  uint8_t begin[1];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_MAP;
  value.value.length = 0;
  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xa0);
}
END_TEST

START_TEST(value_map_1234)
{
  uint8_t begin[5];
  uint8_t* end;

  struct wpcp_value_t value;
  struct wpcp_value_t subvalue[4];
  value.type = WPCP_VALUE_TYPE_MAP;
  value.value.length = 2;
  value.data.first_child = &subvalue[0];
  subvalue[0].type = WPCP_VALUE_TYPE_UINT64;
  subvalue[0].value.uint = 1;
  subvalue[0].next = &subvalue[1];
  subvalue[1].type = WPCP_VALUE_TYPE_UINT64;
  subvalue[1].value.uint = 2;
  subvalue[1].next = &subvalue[2];
  subvalue[2].type = WPCP_VALUE_TYPE_UINT64;
  subvalue[2].value.uint = 3;
  subvalue[2].next = &subvalue[3];
  subvalue[3].type = WPCP_VALUE_TYPE_UINT64;
  subvalue[3].value.uint = 4;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xa2);
  ck_assert_int_eq(begin[1], 0x01);
  ck_assert_int_eq(begin[2], 0x02);
  ck_assert_int_eq(begin[3], 0x03);
  ck_assert_int_eq(begin[4], 0x04);
}
END_TEST

START_TEST(value_tag)
{
  uint8_t begin[6];
  uint8_t* end;

  struct wpcp_value_t value;
  struct wpcp_value_t subvalue;
  value.type = WPCP_VALUE_TYPE_TAG;
  value.value.uint = 1;
  value.data.first_child = &subvalue;
  subvalue.type = WPCP_VALUE_TYPE_UINT64;
  subvalue.value.uint = 1363896240;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xc1);
  ck_assert_int_eq(begin[1], 0x1a);
  ck_assert_int_eq(begin[2], 0x51);
  ck_assert_int_eq(begin[3], 0x4b);
  ck_assert_int_eq(begin[4], 0x67);
  ck_assert_int_eq(begin[5], 0xb0);
}
END_TEST

START_TEST(value_simple_value_16)
{
  uint8_t begin[1];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_SIMPLE_VALUE;
  value.value.uint = 16;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xf0);
}
END_TEST

START_TEST(value_simple_value_255)
{
  uint8_t begin[2];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_SIMPLE_VALUE;
  value.value.uint = 255;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xf8);
  ck_assert_int_eq(begin[1], 0xff);
}
END_TEST

START_TEST(value_float)
{
  uint8_t begin[5];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_FLOAT;
  value.value.flt = 100000.0;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xfa);
  ck_assert_int_eq(begin[1], 0x47);
  ck_assert_int_eq(begin[2], 0xc3);
  ck_assert_int_eq(begin[3], 0x50);
  ck_assert_int_eq(begin[4], 0x00);
}
END_TEST

START_TEST(value_double)
{
  uint8_t begin[9];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_DOUBLE;
  value.value.dbl = 1.1;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xfb);
  ck_assert_int_eq(begin[1], 0x3f);
  ck_assert_int_eq(begin[2], 0xf1);
  ck_assert_int_eq(begin[3], 0x99);
  ck_assert_int_eq(begin[4], 0x99);
  ck_assert_int_eq(begin[5], 0x99);
  ck_assert_int_eq(begin[6], 0x99);
  ck_assert_int_eq(begin[7], 0x99);
  ck_assert_int_eq(begin[8], 0x9a);
}
END_TEST

START_TEST(value_false)
{
  uint8_t begin[1];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_FALSE;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xf4);
}
END_TEST

START_TEST(value_true)
{
  uint8_t begin[1];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_TRUE;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xf5);
}
END_TEST

START_TEST(value_null)
{
  uint8_t begin[1];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_NULL;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xf6);
}
END_TEST

START_TEST(value_undefined)
{
  uint8_t begin[1];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_UNDEFINED;

  wpcp_cbor_write_buffer_write_wpcp_value(&g_write_buffer, &value);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xf7);
}
END_TEST

START_TEST(data_item_empty)
{
  uint8_t begin[1];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_data_item(&g_write_buffer, NULL, 0.0, 0, NULL, 0);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xa0);
}
END_TEST

START_TEST(data_item_value)
{
  uint8_t begin[40];
  uint8_t* end;

  struct wpcp_value_t value;
  value.type = WPCP_VALUE_TYPE_UNDEFINED;
  struct wpcp_key_value_pair_t pair;
  pair.key = "key";
  pair.key_length = 3;
  pair.value = &value;

  wpcp_cbor_write_buffer_write_data_item(&g_write_buffer, &value, 4.3, 2, &pair, 1);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xa4);
  ck_assert_int_eq(begin[1], 0x65);
  ck_assert_int_eq(begin[2], 0x76);
  ck_assert_int_eq(begin[3], 0x61);
  ck_assert_int_eq(begin[4], 0x6c);
  ck_assert_int_eq(begin[5], 0x75);
  ck_assert_int_eq(begin[6], 0x65);
  ck_assert_int_eq(begin[7], 0xf7);
  ck_assert_int_eq(begin[8], 0x69);
  ck_assert_int_eq(begin[9], 0x74);
  ck_assert_int_eq(begin[10], 0x69);
  ck_assert_int_eq(begin[11], 0x6d);
  ck_assert_int_eq(begin[12], 0x65);
  ck_assert_int_eq(begin[13], 0x73);
  ck_assert_int_eq(begin[14], 0x74);
  ck_assert_int_eq(begin[15], 0x61);
  ck_assert_int_eq(begin[16], 0x6d);
  ck_assert_int_eq(begin[17], 0x70);
  ck_assert_int_eq(begin[18], 0xfb);
  ck_assert_int_eq(begin[19], 0x40);
  ck_assert_int_eq(begin[20], 0x11);
  ck_assert_int_eq(begin[21], 0x33);
  ck_assert_int_eq(begin[22], 0x33);
  ck_assert_int_eq(begin[23], 0x33);
  ck_assert_int_eq(begin[24], 0x33);
  ck_assert_int_eq(begin[25], 0x33);
  ck_assert_int_eq(begin[26], 0x33);
  ck_assert_int_eq(begin[27], 0x66);
  ck_assert_int_eq(begin[28], 0x73);
  ck_assert_int_eq(begin[29], 0x74);
  ck_assert_int_eq(begin[30], 0x61);
  ck_assert_int_eq(begin[31], 0x74);
  ck_assert_int_eq(begin[32], 0x75);
  ck_assert_int_eq(begin[33], 0x73);
  ck_assert_int_eq(begin[34], 0x02);
  ck_assert_int_eq(begin[35], 0x63);
  ck_assert_int_eq(begin[36], 0x6b);
  ck_assert_int_eq(begin[37], 0x65);
  ck_assert_int_eq(begin[38], 0x79);
  ck_assert_int_eq(begin[39], 0xf7);
}
END_TEST

START_TEST(alarm_item_empty)
{
  uint8_t begin[28];
  uint8_t* end;

  wpcp_cbor_write_buffer_write_alarm_item(&g_write_buffer, NULL, 0, false, NULL, NULL, 0.0, 0, NULL, 0, true, NULL, 0);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xa3);
  ck_assert_int_eq(begin[1], 0x63);
  ck_assert_int_eq(begin[2], 0x6b);
  ck_assert_int_eq(begin[3], 0x65);
  ck_assert_int_eq(begin[4], 0x79);
  ck_assert_int_eq(begin[5], 0x60);
  ck_assert_int_eq(begin[6], 0x66);
  ck_assert_int_eq(begin[7], 0x72);
  ck_assert_int_eq(begin[8], 0x65);
  ck_assert_int_eq(begin[9], 0x74);
  ck_assert_int_eq(begin[10], 0x61);
  ck_assert_int_eq(begin[11], 0x69);
  ck_assert_int_eq(begin[12], 0x6e);
  ck_assert_int_eq(begin[13], 0xf4);
  ck_assert_int_eq(begin[14], 0x6c);
  ck_assert_int_eq(begin[15], 0x61);
  ck_assert_int_eq(begin[16], 0x63);
  ck_assert_int_eq(begin[17], 0x6b);
  ck_assert_int_eq(begin[18], 0x6e);
  ck_assert_int_eq(begin[19], 0x6f);
  ck_assert_int_eq(begin[20], 0x77);
  ck_assert_int_eq(begin[21], 0x6c);
  ck_assert_int_eq(begin[22], 0x65);
  ck_assert_int_eq(begin[23], 0x64);
  ck_assert_int_eq(begin[24], 0x67);
  ck_assert_int_eq(begin[25], 0x65);
  ck_assert_int_eq(begin[26], 0x64);
  ck_assert_int_eq(begin[27], 0xf5);
}
END_TEST

START_TEST(alarm_item_value)
{
  uint8_t begin[86];
  uint8_t* end;

  struct wpcp_value_t token;
  token.type = WPCP_VALUE_TYPE_UNDEFINED;
  struct wpcp_value_t id;
  token.type = WPCP_VALUE_TYPE_NULL;
  struct wpcp_key_value_pair_t pair;
  pair.key = "add";
  pair.key_length = 3;
  pair.value = &token;

  wpcp_cbor_write_buffer_write_alarm_item(&g_write_buffer, "KY", 2, true, &token, &id, 6.5, 4, "MSG", 3, false, &pair, 1);
  end = wpcp_cbor_write_write_buffer(begin, &g_write_buffer);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0xa9);
  ck_assert_int_eq(begin[1], 0x63);
  ck_assert_int_eq(begin[2], 0x6b);
  ck_assert_int_eq(begin[3], 0x65);
  ck_assert_int_eq(begin[4], 0x79);
  ck_assert_int_eq(begin[5], 0x62);
  ck_assert_int_eq(begin[6], 0x4b);
  ck_assert_int_eq(begin[7], 0x59);
  ck_assert_int_eq(begin[8], 0x66);
  ck_assert_int_eq(begin[9], 0x72);
  ck_assert_int_eq(begin[10], 0x65);
  ck_assert_int_eq(begin[11], 0x74);
  ck_assert_int_eq(begin[12], 0x61);
  ck_assert_int_eq(begin[13], 0x69);
  ck_assert_int_eq(begin[14], 0x6e);
  ck_assert_int_eq(begin[15], 0xf5);
  ck_assert_int_eq(begin[16], 0x65);
  ck_assert_int_eq(begin[17], 0x74);
  ck_assert_int_eq(begin[18], 0x6f);
  ck_assert_int_eq(begin[19], 0x6b);
  ck_assert_int_eq(begin[20], 0x65);
  ck_assert_int_eq(begin[21], 0x6e);
  ck_assert_int_eq(begin[22], 0xf6);
  ck_assert_int_eq(begin[23], 0x62);
  ck_assert_int_eq(begin[24], 0x69);
  ck_assert_int_eq(begin[25], 0x64);
  ck_assert_int_eq(begin[26], 0x69);
  ck_assert_int_eq(begin[27], 0x74);
  ck_assert_int_eq(begin[28], 0x69);
  ck_assert_int_eq(begin[29], 0x6d);
  ck_assert_int_eq(begin[30], 0x65);
  ck_assert_int_eq(begin[31], 0x73);
  ck_assert_int_eq(begin[32], 0x74);
  ck_assert_int_eq(begin[33], 0x61);
  ck_assert_int_eq(begin[34], 0x6d);
  ck_assert_int_eq(begin[35], 0x70);
  ck_assert_int_eq(begin[36], 0xfb);
  ck_assert_int_eq(begin[37], 0x40);
  ck_assert_int_eq(begin[38], 0x1a);
  ck_assert_int_eq(begin[39], 0x00);
  ck_assert_int_eq(begin[40], 0x00);
  ck_assert_int_eq(begin[40], 0x00);
  ck_assert_int_eq(begin[41], 0x00);
  ck_assert_int_eq(begin[42], 0x00);
  ck_assert_int_eq(begin[43], 0x00);
  ck_assert_int_eq(begin[44], 0x00);
  ck_assert_int_eq(begin[45], 0x68);
  ck_assert_int_eq(begin[46], 0x70);
  ck_assert_int_eq(begin[47], 0x72);
  ck_assert_int_eq(begin[48], 0x69);
  ck_assert_int_eq(begin[49], 0x6f);
  ck_assert_int_eq(begin[50], 0x72);
  ck_assert_int_eq(begin[51], 0x69);
  ck_assert_int_eq(begin[52], 0x74);
  ck_assert_int_eq(begin[53], 0x79);
  ck_assert_int_eq(begin[54], 0x04);
  ck_assert_int_eq(begin[55], 0x67);
  ck_assert_int_eq(begin[56], 0x6d);
  ck_assert_int_eq(begin[57], 0x65);
  ck_assert_int_eq(begin[58], 0x73);
  ck_assert_int_eq(begin[59], 0x73);
  ck_assert_int_eq(begin[60], 0x61);
  ck_assert_int_eq(begin[61], 0x67);
  ck_assert_int_eq(begin[62], 0x65);
  ck_assert_int_eq(begin[63], 0x63);
  ck_assert_int_eq(begin[64], 0x4d);
  ck_assert_int_eq(begin[65], 0x53);
  ck_assert_int_eq(begin[66], 0x47);
  ck_assert_int_eq(begin[67], 0x6c);
  ck_assert_int_eq(begin[68], 0x61);
  ck_assert_int_eq(begin[69], 0x63);
  ck_assert_int_eq(begin[70], 0x6b);
  ck_assert_int_eq(begin[71], 0x6e);
  ck_assert_int_eq(begin[72], 0x6f);
  ck_assert_int_eq(begin[73], 0x77);
  ck_assert_int_eq(begin[74], 0x6c);
  ck_assert_int_eq(begin[75], 0x65);
  ck_assert_int_eq(begin[76], 0x64);
  ck_assert_int_eq(begin[77], 0x67);
  ck_assert_int_eq(begin[78], 0x65);
  ck_assert_int_eq(begin[79], 0x64);
  ck_assert_int_eq(begin[80], 0xf4);
  ck_assert_int_eq(begin[81], 0x63);
  ck_assert_int_eq(begin[82], 0x61);
  ck_assert_int_eq(begin[83], 0x64);
  ck_assert_int_eq(begin[84], 0x64);
  ck_assert_int_eq(begin[85], 0xf6);
}
END_TEST

TCase* testcase_cbor_write_buffer(void)
{
  TCase* ret = tcase_create ("CBOR Write Buffer");
  tcase_add_checked_fixture(ret, tc_setup, tc_teardown);

  tcase_add_test (ret, empty);
  tcase_add_test (ret, double_0);
  tcase_add_test (ret, double_1_1);
  tcase_add_test (ret, array_header_0);
  tcase_add_test (ret, array_header_1);
  tcase_add_test (ret, map_header_0);
  tcase_add_test (ret, map_header_1);
  tcase_add_test (ret, bool_false);
  tcase_add_test (ret, bool_true);
  tcase_add_test (ret, null);
  tcase_add_test (ret, text_string_0);
  tcase_add_test (ret, text_string_1);
  tcase_add_test (ret, text_string_10);
  tcase_add_test (ret, text_string_23);
  tcase_add_test (ret, text_string_24);
  tcase_add_test (ret, text_string_25);
  tcase_add_test (ret, text_string_100);
  tcase_add_test (ret, text_string_1024);
  tcase_add_test (ret, unsigned_integer_0);
  tcase_add_test (ret, unsigned_integer_1);
  tcase_add_test (ret, unsigned_integer_10);
  tcase_add_test (ret, unsigned_integer_23);
  tcase_add_test (ret, unsigned_integer_24);
  tcase_add_test (ret, unsigned_integer_25);
  tcase_add_test (ret, unsigned_integer_100);
  tcase_add_test (ret, unsigned_integer_255);
  tcase_add_test (ret, unsigned_integer_256);
  tcase_add_test (ret, unsigned_integer_1000);
  tcase_add_test (ret, unsigned_integer_1000000);
  tcase_add_test (ret, malloc_fail);
  tcase_add_test (ret, value_sint_n100);
  tcase_add_test (ret, value_sint_p100);
  tcase_add_test (ret, value_uint_f);
  tcase_add_test (ret, value_uint_ff);
  tcase_add_test (ret, value_uint_ffff);
  tcase_add_test (ret, value_uint_ffffffff);
  tcase_add_test (ret, value_uint_ffffffffffffffff);
  tcase_add_test (ret, value_byte_string);
  tcase_add_test (ret, value_text_string);
  tcase_add_test (ret, value_array_empty);
  tcase_add_test (ret, value_array_123);
  tcase_add_test (ret, value_array_in_array);
  tcase_add_test (ret, value_map_empty);
  tcase_add_test (ret, value_map_1234);
  tcase_add_test (ret, value_tag);
  tcase_add_test (ret, value_simple_value_16);
  tcase_add_test (ret, value_simple_value_255);
  tcase_add_test (ret, value_float);
  tcase_add_test (ret, value_double);
  tcase_add_test (ret, value_false);
  tcase_add_test (ret, value_true);
  tcase_add_test (ret, value_null);
  tcase_add_test (ret, value_undefined);
  tcase_add_test (ret, data_item_empty);
  tcase_add_test (ret, data_item_value);
  tcase_add_test (ret, alarm_item_empty);
  tcase_add_test (ret, alarm_item_value);

  return ret;
}
