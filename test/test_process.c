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

void show_help ()
{
  printf("Help:\n"
    "  h       show help\n"
    "  [1-9]   sleep specified number of seconds\n"
    "  e       show value environment variable TEST\n"
    "  i       show process ID\n"
    "  p       show process priority\n"
    "  n       show number of logical processors\n"
    "  a       get processor affinity mask\n"
    "  l       set low CPU affinity and process priority\n"
    "  m       set high CPU affinity and process priority\n"
    "  x       exit with exit code 99\n"
    "  q       quit normally\n"
  );
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
      case 'i':
        printf("PID: %lu\n", crossrun_get_current_pid());
        break;
      case 'p':
        printf("Priority: %s\n", crossrun_prio_name[crossrun_get_current_prio()]);
        break;
      case 'n':
        printf("Logical processors: %lu\n", crossrun_get_logical_processors());
        break;
      case 'a':
        {
          crossrun_cpumask cpumask = crossrun_cpumask_create();
          if (crossrun_get_current_affinity(cpumask) != 0) {
            printf("Error getting affinity mask\n");
          } else {
            int i;
            int n = crossrun_cpumask_get_cpus(cpumask);
            printf("Affinity mask: ");
            for (i = n; i-- > 0; ) {
              printf("%i", (crossrun_cpumask_is_set(cpumask, i) ? 1 : 0));
            }
            printf("\n");
          }
          crossrun_cpumask_free(cpumask);
        }
        break;
      case 'l':
        {
          crossrun_cpumask cpumask = crossrun_cpumask_create();
          crossrun_cpumask_clear_all(cpumask);
          crossrun_cpumask_set(cpumask, crossrun_cpumask_get_cpus(cpumask) - 1);
          if (crossrun_set_current_affinity(cpumask) != 0) {
            printf("Error setting processor affinity\n");
          }
          crossrun_cpumask_free(cpumask);
          if (crossrun_set_current_prio(CROSSRUN_PRIO_LOW) != 0) {
            printf("Error setting process priority\n");
          }
        }
        break;
      case 'm':
        {
          crossrun_cpumask cpumask = crossrun_cpumask_create();
          crossrun_cpumask_set_all(cpumask);
          if (crossrun_set_current_affinity(cpumask) != 0) {
            printf("Error setting processor affinity\n");
          }
          crossrun_cpumask_free(cpumask);
          if (crossrun_set_current_prio(CROSSRUN_PRIO_HIGH) != 0) {
            printf("Error setting process priority\n");
          }
        }
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
