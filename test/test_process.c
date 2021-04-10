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
    "  x       exit with exit code 99\n"
    "  q       quit normally\n"
  );
}

int main (int argc, char* argv[])
{
  int i;
  int c;
  printf("Program started: %s\n", argv[0]);
  for (i = 1; i < argc; i++) {
    printf("- Command line parameter %i: \"%s\"\n", i, argv[i]);
  }
  fflush(stdout);
  while ((c = getchar()) != EOF) {
    if (c == 'q')
      break;
    if (c == 'x') {
      printf("Exiting with exit code 99\n");
      exit(99);
    }
    if (c >= '1' && c <= '9') {
      printf("Sleeping %i seconds", c - '0');
      fflush(stdout);
      sleep_seconds(c - '0');
      printf("\n");
    } else if (c == 'h' || c == '\n') {
      show_help();
    }
    fflush(stdout);
  }
  printf("Exiting normally\n");
  return 0;
}