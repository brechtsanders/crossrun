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
#define sleep_milliseconds(n) sleep((n) * 1000)
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

void test_result (int index, int successcondition)
{
  printf("Test %i: %s\n", index, (successcondition ? "PASS" : "FAIL"));
}

int main (int argc, char* argv[])
{
  char* test_process_path;
  crossrun handle;
  unsigned long exitcode;
  char buf[128];
  int n;
  int index = 0;

  //determine path to test process to run
  if ((test_process_path = get_test_process_path(argv[0])) == NULL) {
    fprintf(stderr, "Unable to determine path of test process\n");
    return 255;
  }
  printf("Test process: %s\n", test_process_path);

  //run test
  announce_test(++index, "Basic execute and check if exit code is 0");
  if ((handle = crossrun_open(test_process_path, NULL)) == NULL) {
    fprintf(stderr, "Error launching process\n");
    return index;
  }
  crossrun_write(handle, "q\n");
  while ((n = crossrun_read(handle, buf, sizeof(buf))) > 0) {
    printf("%.*s", n, buf);
  }
  crossrun_wait(handle);
  exitcode = crossrun_get_exit_code(handle);
  crossrun_close(handle);
  crossrun_free(handle);
  test_result(index, (exitcode == 0));

  //run test
  announce_test(++index, "Basic execute and check if exit code is 99");
  if ((handle = crossrun_open(test_process_path, NULL)) == NULL) {
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
  test_result(index, (handle && exitcode == 99));

  //run test
  announce_test(++index, "Basic execute and close");
  if ((handle = crossrun_open(test_process_path, NULL)) == NULL) {
    fprintf(stderr, "Error launching process\n");
  } else {
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
  test_result(index, (handle && n != 0));

  //run test
  announce_test(++index, "Basic execute and kill");
  if ((handle = crossrun_open(test_process_path, NULL)) == NULL) {
    fprintf(stderr, "Error launching process\n");
  } else {
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
  test_result(index, (handle && n != 0));

  //run test
  announce_test(++index, "Basic execute and non-blocking read");
  if ((handle = crossrun_open(test_process_path, NULL)) == NULL) {
    fprintf(stderr, "Error launching process\n");
  } else {
    crossrun_write(handle, "3\n");
    crossrun_write(handle, "q\n");
printf("<");/////
    while ((n = crossrun_read_available(handle, buf, sizeof(buf))) >= 0) {
printf(">");/////
      if (n) {
        printf("%.*s", n, buf);
      } else {
printf(".");/////
//        printf("[sleeping 500ms]\n");
        sleep_milliseconds(100);
      }
printf("<");/////
    }
    crossrun_wait(handle);
    exitcode = crossrun_get_exit_code(handle);
    crossrun_close(handle);
    crossrun_free(handle);
  }
  test_result(index, (handle && exitcode == 0));

  //clean up
  free(test_process_path);
  return 0;

#if 0
#ifdef _WIN32
  const char* cmd = "D:\\Prog\\msys64\\usr\\bin\\bash.exe --noprofile --rcfile D:\\Prog\\winlibs32_stage\\profile_settings.sh -i";
  //const char* cmd = "D:\\msys64\\usr\\bin\\bash.exe --noprofile --rcfile D:\\winlibs64-10.2.0-8.0.0\\profile_settings.sh -i";
#else
  const char* cmd = "/bin/bash -i";
#endif
#ifdef _WIN32
  //support ANSI/VT100 control codes in output
  SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
#endif
  //prepare environment variables
  crossrunenv env = crossrunenv_create_from_system();
  crossrunenv_set(&env, "TERM", "VT100");
  crossrunenv_set(&env, "TESTVAR", "Test value");
/*
  //show environment variables
  crossrunenv_iterate(env, show_var, NULL);
*/
  //start process
  if ((handle = crossrun_open(cmd, env)) == NULL) {
    fprintf(stderr, "Error launching process\n");
    return 1;
  }
  //clean up environment variables
  crossrunenv_free(env);
  //write data
  crossrun_write(handle, "pwd\n");
  crossrun_write(handle, "ls -l\n");
  crossrun_write(handle, "echo TESTVAR=$TESTVAR\n");
  crossrun_write(handle, "sleep 2\n");
  //crossrun_write(handle, "~/buildstatus.sh Test status\n");
  crossrun_write(handle, "echo \"Test status\"\n");
  crossrun_write(handle, "exit\n");
  crossrun_write_eof(handle);
  //read data
/*/
  while ((n = crossrun_read(handle, buf, sizeof(buf))) > 0) {
    //printf("[%.*s]", n, buf);
    printf("%.*s", n, buf);
  }
/*/
  while ((n = crossrun_read_available(handle, buf, sizeof(buf))) >= 0) {
    if (n) {
      printf("%.*s", n, buf);
    } else {
      printf("[sleeping 1s]\n");
      sleep_seconds(1);
    }
  }
/**/
  //wait for end of process
  crossrun_wait(handle);
  printf("Exit code: %lu\n", crossrun_get_exit_code(handle));
  crossrun_close(handle);
  return 0;
#endif
}

/////See also: https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

////TO DO: process priority: setpriority(PRIO_PROCESS, 0, -?)
