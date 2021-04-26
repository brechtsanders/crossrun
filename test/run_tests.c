#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "crossrun.h"

#ifdef _WIN32
#define EXE_SUFFIX ".exe"
#else
#define EXE_SUFFIX ""
#endif
#define TEST_PROCESS "test_process" EXE_SUFFIX

#ifdef _WIN32
#define sleep_milliseconds(n) Sleep(n)
#else
#define sleep_milliseconds(n) usleep((n) * 1000)
#endif

int show_var (const char* name, const char* value, void* callbackdata)
{
  printf("%s = \"%s\"\n", name, value);
  return 0;
}

char* get_test_process_path (const char* argv0)
{
  size_t i;
  char* result;
  i = strlen(argv0);
  while (i > 0 && argv0[i - 1] != '/'
#ifdef _WIN32
    && argv0[i - 1] != '\\' && argv0[i - 1] != ':'
#endif
  )
    i--;
  if ((result = (char*)malloc(i + strlen(TEST_PROCESS) + 1)) != NULL) {
    memcpy(result, argv0, i);
    strcpy(result + i, TEST_PROCESS);
  }
  return result;
}

void announce_test (int index, const char* description)
{
  printf("[Test %i] - %s\n", index, description);
}

static int tests_succeeded = 0;
static int tests_failed = 0;

void test_result (int index, int successcondition)
{
  if (successcondition)
    tests_succeeded++;
  else
    tests_failed++;
  printf("Test %i: %s\n", index, (successcondition ? "PASS" : "FAIL"));
}

int read_data (const char* data, size_t datalen, void* callbackdata)
{
  printf("%.*s", (int)datalen, data);
  return 0;
}

int main (int argc, char* argv[])
{
  char* test_process_path;
  crossrun handle;
  crossrunenv env;
  unsigned long exitcode;
  char buf[128];
  int n;
  char* p;
  int index = 0;

  //determine path to test process to run
  if ((test_process_path = get_test_process_path(argv[0])) == NULL) {
    fprintf(stderr, "Unable to determine path of test process\n");
    return 255;
  }
  printf("Test process: %s\n", test_process_path);

  //run test
  announce_test(++index, "Execute and check if exit code is 0");
  if ((handle = crossrun_open(test_process_path, NULL, CROSSRUN_PRIO_BELOW_NORMAL)) == NULL) {
    fprintf(stderr, "Error launching process\n");
    exitcode = ~0;
  } else {
    printf("started PID %lu\n", crossrun_get_pid(handle));
    crossrun_write(handle, "ipq\n");
    while ((n = crossrun_read(handle, buf, sizeof(buf))) > 0) {
      printf("%.*s", n, buf);
    }
    crossrun_wait(handle);
    exitcode = crossrun_get_exit_code(handle);
    crossrun_close(handle);
    crossrun_free(handle);
  }
  test_result(index, (handle != NULL && exitcode == 0));

  //run test
  announce_test(++index, "Execute and check if exit code is 99");
  if ((handle = crossrun_open(test_process_path, NULL, CROSSRUN_PRIO_NORMAL)) == NULL) {
    fprintf(stderr, "Error launching process\n");
  } else {
    crossrun_write(handle, "x\n");
    while ((n = crossrun_read(handle, buf, sizeof(buf))) > 0) {
      printf("%.*s", n, buf);
    }
    crossrun_wait(handle);
    exitcode = crossrun_get_exit_code(handle);
    crossrun_close(handle);
    crossrun_free(handle);
  }
  test_result(index, (handle != NULL && exitcode == 99));

  //run test
  announce_test(++index, "Execute and close");
  if ((handle = crossrun_open(test_process_path, NULL, CROSSRUN_PRIO_NORMAL)) == NULL) {
    fprintf(stderr, "Error launching process\n");
    n = 0;
  } else {
    crossrun_write(handle, "5x\n");
    sleep_milliseconds(200);
    crossrun_close(handle);
    n = crossrun_stopped(handle);
    if (!n) {
      sleep_milliseconds(200);
      n = crossrun_stopped(handle);
    }
    crossrun_wait(handle);
    exitcode = crossrun_get_exit_code(handle);
    crossrun_close(handle);
    crossrun_free(handle);
  }
  test_result(index, (handle != NULL && n != 0 && exitcode == 0));

  //run test
  announce_test(++index, "Execute and kill");
  if ((handle = crossrun_open(test_process_path, NULL, CROSSRUN_PRIO_NORMAL)) == NULL) {
    fprintf(stderr, "Error launching process\n");
  } else {
    crossrun_write(handle, "5x\n");
    sleep_milliseconds(200);
    crossrun_kill(handle);
    n = crossrun_stopped(handle);
    if (!n) {
      sleep_milliseconds(200);
      n = crossrun_stopped(handle);
    }
    crossrun_wait(handle);
    exitcode = crossrun_get_exit_code(handle);
    crossrun_close(handle);
    crossrun_free(handle);
  }
  test_result(index, (handle != NULL && n != 0));

  //run test
  announce_test(++index, "Execute and non-blocking read");
  if ((handle = crossrun_open(test_process_path, NULL, CROSSRUN_PRIO_NORMAL)) == NULL) {
    fprintf(stderr, "Error launching process\n");
  } else {
    crossrun_write(handle, "3q\n");
printf("<");/////
    while ((n = crossrun_read_available(handle, buf, sizeof(buf))) >= 0) {
printf(">");/////
      if (n) {
        printf("%.*s", n, buf);
      } else {
printf(".");/////
        sleep_milliseconds(200);
      }
printf("<");/////
    }
    n = crossrun_wait(handle);
    exitcode = crossrun_get_exit_code(handle);
    crossrun_close(handle);
    crossrun_free(handle);
    printf("exitcode: %lu\n", exitcode);
  }
  test_result(index, (handle != NULL /*&& exitcode == 0*/));

  printf("Tests succeeded:  %i\n", tests_succeeded);
  printf("Tests failed:     %i\n", tests_failed);

  //run test
  announce_test(++index, "Execute with unmodified system environment");
  env = crossrunenv_create_from_system();
  if ((handle = crossrun_open(test_process_path, env, CROSSRUN_PRIO_NORMAL)) == NULL) {
    fprintf(stderr, "Error launching process\n");
    exitcode = ~0;
  } else {
    crossrun_write(handle, "q\n");
    while ((n = crossrun_read(handle, buf, sizeof(buf))) > 0) {
      printf("%.*s", n, buf);
    }
    crossrun_wait(handle);
    exitcode = crossrun_get_exit_code(handle);
    crossrun_close(handle);
    crossrun_free(handle);
  }
  test_result(index, (handle != NULL && env != NULL && exitcode == 0));
  crossrunenv_free(env);

  //run test
  announce_test(++index, "Execute with modified system environment");
  env = crossrunenv_create_from_system();
  crossrunenv_set(&env, "TEST", "TestData");
  p = NULL;
  if ((handle = crossrun_open(test_process_path, env, CROSSRUN_PRIO_NORMAL)) == NULL) {
    fprintf(stderr, "Error launching process\n");
    exitcode = ~0;
  } else {
    crossrun_write(handle, "eq\n");
    while ((n = crossrun_read(handle, buf, sizeof(buf) - 1)) > 0) {
      buf[n] = 0;
      if (!p)
        p = strstr(buf, "TEST: TestData");
      printf("[%.*s]", n, buf);
    }
    crossrun_wait(handle);
    exitcode = crossrun_get_exit_code(handle);
    crossrun_close(handle);
    crossrun_free(handle);
  }
  test_result(index, (handle != NULL && env != NULL && p != NULL && exitcode == 0));
  crossrunenv_free(env);

/*
  //run test
  announce_test(++index, "Execute and send large block of input");
  if ((handle = crossrun_open(test_process_path, NULL, CROSSRUN_PRIO_BELOW_NORMAL)) == NULL) {
    fprintf(stderr, "Error launching process\n");
    exitcode = ~0;
  } else {
    char* buf;
    size_t buflen = 128 * 1024;
    if ((buf = (char*)malloc(buflen)) == NULL) {
      exitcode = ~0;
      crossrun_kill(handle);
      crossrun_wait(handle);
    } else {
      memset(buf, 'i', buflen);
      crossrun_writedata(handle, buf, buflen);
      crossrun_write(handle, "q\n");
      while ((n = crossrun_read(handle, buf, sizeof(buf))) > 0) {
        printf("%.*s", n, buf);
      }
      crossrun_wait(handle);
      exitcode = crossrun_get_exit_code(handle);
      crossrun_close(handle);
    }
    crossrun_free(handle);
  }
  test_result(index, (handle != NULL && exitcode == 0));
*/

  //clean up
  free(test_process_path);
  return tests_failed;
}

/////See also: https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

////TO DO: process priority: setpriority(PRIO_PROCESS, 0, -?)
