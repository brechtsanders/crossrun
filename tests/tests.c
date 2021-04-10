#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "crossrun.h"

#ifdef _WIN32
#define sleep_seconds(n) Sleep((n) * 1000)
#else
#define sleep_seconds(n) sleep(n)
#endif

int show_var (const char* name, const char* value, void* callbackdata)
{
  printf("%s = \"%s\"\n", name, value);
  return 0;
}

int main ()
{
  crossrun handle;
  int n;
  char buf[128];
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
}

/////See also: https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

////TO DO: process priority: setpriority(PRIO_PROCESS, 0, -?)
