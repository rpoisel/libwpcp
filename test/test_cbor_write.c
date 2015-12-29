#include "test.h"
#include "../src/wpcp_cbor.h"

static void tc_setup(void)
{
  testcase_setup();
}

static void tc_teardown(void)
{
  testcase_teardown();
}

START_TEST(array_header_0)
{
  uint8_t begin[1];
  uint8_t* end;

  end = wpcp_cbor_write_array_header(begin, 0);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x80);
}
END_TEST

START_TEST(array_header_1)
{
  uint8_t begin[1];
  uint8_t* end;

  end = wpcp_cbor_write_array_header(begin, 1);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x81);
}
END_TEST

START_TEST(array_header_10)
{
  uint8_t begin[1];
  uint8_t* end;

  end = wpcp_cbor_write_array_header(begin, 10);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x8a);
}
END_TEST

START_TEST(array_header_23)
{
  uint8_t begin[1];
  uint8_t* end;

  end = wpcp_cbor_write_array_header(begin, 23);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x97);
}
END_TEST

START_TEST(array_header_24)
{
  uint8_t begin[2];
  uint8_t* end;

  end = wpcp_cbor_write_array_header(begin, 24);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x98);
  ck_assert_int_eq(begin[1], 0x18);
}
END_TEST

START_TEST(array_header_25)
{
  uint8_t begin[2];
  uint8_t* end;

  end = wpcp_cbor_write_array_header(begin, 25);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x98);
  ck_assert_int_eq(begin[1], 0x19);
}
END_TEST

START_TEST(array_header_100)
{
  uint8_t begin[2];
  uint8_t* end;

  end = wpcp_cbor_write_array_header(begin, 100);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x98);
  ck_assert_int_eq(begin[1], 0x64);
}
END_TEST

START_TEST(array_header_255)
{
  uint8_t begin[2];
  uint8_t* end;

  end = wpcp_cbor_write_array_header(begin, 255);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x98);
  ck_assert_int_eq(begin[1], 0xff);
}
END_TEST

START_TEST(array_header_256)
{
  uint8_t begin[3];
  uint8_t* end;

  end = wpcp_cbor_write_array_header(begin, 256);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x99);
  ck_assert_int_eq(begin[1], 0x01);
  ck_assert_int_eq(begin[2], 0x00);
}
END_TEST

START_TEST(array_header_1000)
{
  uint8_t begin[3];
  uint8_t* end;

  end = wpcp_cbor_write_array_header(begin, 1000);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x99);
  ck_assert_int_eq(begin[1], 0x03);
  ck_assert_int_eq(begin[2], 0xe8);
}
END_TEST

START_TEST(array_header_1000000)
{
  uint8_t begin[5];
  uint8_t* end;

  end = wpcp_cbor_write_array_header(begin, 1000000);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x9a);
  ck_assert_int_eq(begin[1], 0x00);
  ck_assert_int_eq(begin[2], 0x0f);
  ck_assert_int_eq(begin[3], 0x42);
  ck_assert_int_eq(begin[4], 0x40);
}
END_TEST

START_TEST(unsigned_integer_0)
{
  uint8_t begin[1];
  uint8_t* end;

  end = wpcp_cbor_write_unsigned_integer(begin, 0);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x00);
}
END_TEST

START_TEST(unsigned_integer_1)
{
  uint8_t begin[1];
  uint8_t* end;

  end = wpcp_cbor_write_unsigned_integer(begin, 1);

  ck_assert_ptr_eq(end, begin + sizeof(begin));
  ck_assert_int_eq(begin[0], 0x01);
}
END_TEST

TCase* testcase_cbor_write(void)
{
  TCase* ret = tcase_create ("CBOR Write");
  tcase_add_checked_fixture(ret, tc_setup, tc_teardown);

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
  tcase_add_test (ret, unsigned_integer_0);
  tcase_add_test (ret, unsigned_integer_1);

  return ret;
}
