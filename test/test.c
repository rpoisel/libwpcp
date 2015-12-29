#include "test.h"
#include <stdlib.h>

int* g_malloc_fails;
int g_malloc_count;

static bool fail_alloc(void)
{
  if (g_malloc_fails) {
    int* mf;
    g_malloc_count += 1;
    for (mf = g_malloc_fails; *mf; ++mf) {
      if (*mf == g_malloc_count)
        return true;
    }
  }
  return false;
}

void testcase_set_malloc_fails(int* malloc_fails)
{
  g_malloc_fails = malloc_fails;
  g_malloc_count = 0;
}

void* wpcp_calloc(size_t count, size_t size)
{
  if (fail_alloc())
    return NULL;
  return calloc(count, size);
}

void* wpcp_malloc(size_t size)
{
  if (fail_alloc())
    return NULL;
  return malloc(size);
}

void* wpcp_realloc(void* data, size_t size)
{
  if (fail_alloc())
    return NULL;
  if (size)
    return realloc(data, size);
  free(data);
  return NULL;
}

void wpcp_free(void* data)
{
  free(data);
}


void testcase_setup(void)
{
  testcase_set_malloc_fails(NULL);
}

void testcase_teardown(void)
{
}


int main(void)
{
  int number_failed;
  SRunner* sr;
  Suite* s = suite_create("WPCP");

  suite_add_tcase(s, testcase_cbor_read_buffer());
  suite_add_tcase(s, testcase_cbor_write());
  suite_add_tcase(s, testcase_cbor_write_buffer());
  suite_add_tcase(s, testcase_session());
  suite_add_tcase(s, testcase_wpcp());

  sr = srunner_create(s);
  srunner_set_fork_status(sr, CK_NOFORK);
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? 0 : 1;
}
