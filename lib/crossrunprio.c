#include "crossrunprio.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/resource.h>
#define __USE_XOPEN
#include <limits.h>
#include <errno.h>
#endif

DLL_EXPORT_CROSSRUN const char* crossrun_prio_name[] = {
  "ERROR",
  "low",
  "below normal",
  "normal",
  "above normal",
  "high"
};

DLL_EXPORT_CROSSRUN int crossrun_get_current_prio ()
{
#ifdef _WIN32
  switch (GetPriorityClass(GetCurrentProcess())) {
    case 0:
      return CROSSRUN_PRIO_ERROR;
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
  return CROSSRUN_PRIO_ERROR;
}

