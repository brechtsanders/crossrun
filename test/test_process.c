#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
//#include <sys/time.h>
#include <sys/resource.h>
#define __USE_XOPEN
#include <limits.h>
#include <errno.h>
#endif
#include "crossrun.h"

#ifdef _WIN32
#define sleep_seconds(n) Sleep((n) * 1000)
#else
#define sleep_seconds(n) sleep(n)
#endif

void show_help ()
{
  printf("Help:\n"
    "  h       show help\n"
    "  [1-9]   sleep specified number of seconds\n"
    "  e       show value environment variable TEST\n"
    "  p       show process priority\n"
    "  x       exit with exit code 99\n"
    "  q       quit normally\n"
  );
}

#define CROSSRUN_PRIO_ERROR             0
#define CROSSRUN_PRIO_LOW               1
#define CROSSRUN_PRIO_BELOW_NORMAL      2
#define CROSSRUN_PRIO_NORMAL            3
#define CROSSRUN_PRIO_ABOVE_NORMAL      4
#define CROSSRUN_PRIO_HIGH              5

const char* cross_run_prio_name[] = {
  "ERROR",
  "low",
  "below normal",
  "normal",
  "above normal",
  "high"
};

int get_process_priority ()
{

#ifdef _WIN32
  switch (GetPriorityClass(GetCurrentProcess())) {
    case IDLE_PRIORITY_CLASS:
      return CROSSRUN_PRIO_LOW;
    case BELOW_NORMAL_PRIORITY_CLASS:
      return CROSSRUN_PRIO_BELOW_NORMAL;
    case NORMAL_PRIORITY_CLASS:
      return CROSSRUN_PRIO_NORMAL;
    case ABOVE_NORMAL_PRIORITY_CLASS:
      return CROSSRUN_PRIO_NORMAL;
    case HIGH_PRIORITY_CLASS:
    case REALTIME_PRIORITY_CLASS:
      return CROSSRUN_PRIO_HIGH;
    case 0:
    default:
      return CROSSRUN_PRIO_ERROR;
  }
#else
  int prio;
  errno = 0;
  prio = getpriority(PRIO_PROCESS, 0);
  if ((prio == -1 && errno != 0) || prio < -NZERO || prio >= NZERO)
    return CROSSRUN_PRIO_ERROR;
  if (prio == NZERO - 1)
    return CROSSRUN_PRIO_LOW;
  if (prio == 0)
    return CROSSRUN_PRIO_NORMAL;
  if (prio == -NZERO)
    return CROSSRUN_PRIO_HIGH;
  if (prio > 0)
    return CROSSRUN_PRIO_BELOW_NORMAL;
  if (prio < 0)
    return CROSSRUN_PRIO_NORMAL;
#endif
}

int main (int argc, char* argv[])
{
  int i;
  int c;
  char* s;
  printf("Program started: %s\n", argv[0]);
  for (i = 1; i < argc; i++) {
    printf("- Command line parameter %i: \"%s\"\n", i, argv[i]);
  }
  fflush(stdout);
  while ((c = getchar()) != EOF) {
    if (c == 'q')
      break;
    if (c >= '1' && c <= '9') {
      printf("Sleeping %i seconds", c - '0');
      fflush(stdout);
      sleep_seconds(c - '0');
      printf("\n");
    } else switch (c) {
      case 'h':
        show_help();
        break;
      case 'e':
        s = getenv("TEST");
        printf("Value of environment variable TEST: %s\n", (s ? s : "(not set)"));
        break;
      case 'p':
        printf("Priority: %s\n", cross_run_prio_name[get_process_priority()]);
        break;
      case 'x':
        printf("Exiting with exit code 99\n");
        exit(99);
        break;
    }
    fflush(stdout);
  }
  printf("Exiting normally\n");
  return 0;
}
