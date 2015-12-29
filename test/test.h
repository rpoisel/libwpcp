#ifndef TEST_H
#define TEST_H

#ifdef _MSC_VER
#define pid_t int
#pragma warning(disable:4100 4127)
#endif

#include <check.h>
#include "wpcp.h"

WPCP_BEGIN_EXTERN_C

void testcase_set_malloc_fails(int* malloc_fails);
void testcase_setup(void);
void testcase_teardown(void);

TCase* testcase_cbor_read_buffer(void);
TCase* testcase_cbor_write(void);
TCase* testcase_cbor_write_buffer(void);
TCase* testcase_session(void);
TCase* testcase_wpcp(void);

WPCP_END_EXTERN_C

#endif
