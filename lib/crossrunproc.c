#ifdef _WIN32
#include <windows.h>
#else
#include <sys/resource.h>
#define __USE_XOPEN
#include <limits.h>
#define _GNU_SOURCE
#define __USE_GNU
#include <sched.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <stdint.h>
#include <stdlib.h>
#include "crossrunproc.h"

DLL_EXPORT_CROSSRUN const char* crossrun_prio_name[] = {
  "ERROR",
  "low",
  "below normal",
  "normal",
  "above normal",
  "high"
};

#ifdef _WIN32
DLL_EXPORT_CROSSRUN DWORD crossrun_prio_os_value[] = {
  0,
  IDLE_PRIORITY_CLASS,
  BELOW_NORMAL_PRIORITY_CLASS,
  NORMAL_PRIORITY_CLASS,
  ABOVE_NORMAL_PRIORITY_CLASS,
  HIGH_PRIORITY_CLASS
};
#else
DLL_EXPORT_CROSSRUN int crossrun_prio_os_value[] = {
  0,
  NZERO - 1,
  NZERO / 2 - 1,
  0,
  -(NZERO / 2),
  -NZERO
};
#endif

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

DLL_EXPORT_CROSSRUN int crossrun_set_current_prio (int priority)
{
  if (priority <= 0 || priority > CROSSRUN_PRIO_HIGH)
    return -1;
#ifdef _WIN32
  return (SetPriorityClass(GetCurrentProcess(), crossrun_prio_os_value[priority]) ? 0 : -1);
#else
  errno = 0;
  if (setpriority(PRIO_PROCESS, 0, crossrun_prio_os_value[priority]) == -1 && errno != 0)
    return -1;
  return 0;
#endif
}

DLL_EXPORT_CROSSRUN unsigned long crossrun_get_current_pid ()
{
#ifdef _WIN32
  return GetCurrentProcessId();
#else
  return getpid();
#endif
}

DLL_EXPORT_CROSSRUN unsigned long crossrun_get_logical_processors ()
{
#ifdef _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
#else
  return sysconf(_SC_NPROCESSORS_CONF);
  //return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

/*
DLL_EXPORT_CROSSRUN uint64_t crossrun_get_logical_processor_mask ()
{
  uint64_t result = 0;
#ifdef _WIN32
  size_t n;
  size_t i;
  SYSTEM_LOGICAL_PROCESSOR_INFORMATION* cpuinfo;
  DWORD cpuinfolen;
  cpuinfolen = 0;
  if (!(GetLogicalProcessorInformation(NULL, &cpuinfolen) == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    return 1;
  if ((cpuinfo = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)malloc(cpuinfolen)) == NULL)
    return 2;
  if (!GetLogicalProcessorInformation(cpuinfo, &cpuinfolen))
    return 3;
  n = cpuinfolen / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
  for (i = 0; i < n; i++) {
    if (cpuinfo[i].Relationship == RelationProcessorCore)
      result |= cpuinfo[i].ProcessorMask;
  }
  free(cpuinfo);
#else
//#error TO DO
  result = sysconf(_SC_NPROCESSORS_CONF);     // processors configured
  //result = sysconf(_SC_NPROCESSORS_ONLN);     // processors available
  //man CPU_SET
  //man sched_getaffinity
  //https://www.programmersought.com/article/7610238950/
#endif
  return result;
}
*/

struct crossrun_cpumask_struct {
  size_t cpucount;
#ifdef _WIN32
  DWORD_PTR cpuset;
#else
  cpu_set_t* cpuset;
#endif
};

DLL_EXPORT_CROSSRUN crossrun_cpumask crossrun_cpumask_create ()
{
  struct crossrun_cpumask_struct* cpumask;
  if ((cpumask = (struct crossrun_cpumask_struct*)malloc(sizeof(struct crossrun_cpumask_struct))) == NULL)
    return NULL;
  if ((cpumask->cpucount = crossrun_get_logical_processors()) == 0) {
    //unable to determine number of logical processors
    free(cpumask);
    return NULL;
  }
#ifdef _WIN32
  cpumask->cpuset = 0;
#else
  if ((cpumask->cpuset = CPU_ALLOC(cpumask->cpucount)) == NULL) {
    //unable to allocate CPU set
    free(cpumask);
    return NULL;
  }
  CPU_ZERO_S(CPU_ALLOC_SIZE(cpumask->cpucount), cpumask->cpuset);
#endif
  return cpumask;
}

DLL_EXPORT_CROSSRUN void crossrun_cpumask_free (crossrun_cpumask cpumask)
{
  if (!cpumask)
    return;
#ifndef _WIN32
  CPU_FREE(cpumask->cpuset);
#endif
  free(cpumask);
}

DLL_EXPORT_CROSSRUN size_t crossrun_cpumask_get_cpus (crossrun_cpumask cpumask)
{
  if (!cpumask)
    return 0;
  return cpumask->cpucount;
}

DLL_EXPORT_CROSSRUN void crossrun_cpumask_clear_all (crossrun_cpumask cpumask)
{
  if (!cpumask)
    return;
#ifdef _WIN32
  cpumask->cpuset = 0;
#else
  CPU_ZERO_S(CPU_ALLOC_SIZE(cpumask->cpucount), cpumask->cpuset);
#endif
}

DLL_EXPORT_CROSSRUN void crossrun_cpumask_set_all (crossrun_cpumask cpumask)
{
  if (!cpumask)
    return;
#ifdef _WIN32
  cpumask->cpuset = ((DWORD_PTR)1 << cpumask->cpucount) - 1;
#else
  int i;
  for (i = 0; i < cpumask->cpucount; i++)
    CPU_SET_S(i, CPU_ALLOC_SIZE(cpumask->cpucount), cpumask->cpuset);
#endif
}

DLL_EXPORT_CROSSRUN void crossrun_cpumask_set (crossrun_cpumask cpumask, int cpuindex)
{
  if (!cpumask)
    return;
#ifdef _WIN32
  cpumask->cpuset |= ((DWORD_PTR)1 << cpuindex);
#else
  CPU_SET_S(cpuindex, CPU_ALLOC_SIZE(cpumask->cpucount), cpumask->cpuset);
#endif
}

DLL_EXPORT_CROSSRUN int crossrun_cpumask_is_set (crossrun_cpumask cpumask, int cpuindex)
{
  if (!cpumask)
    return 0;
#ifdef _WIN32
  return (cpumask->cpuset & ((DWORD_PTR)1 << cpuindex));
#else
  return CPU_ISSET_S(cpuindex, CPU_ALLOC_SIZE(cpumask->cpucount), cpumask->cpuset);
#endif
}

DLL_EXPORT_CROSSRUN size_t crossrun_cpumask_count (crossrun_cpumask cpumask)
{
  if (!cpumask)
    return 0;
#ifdef _WIN32
  size_t count = 0;
  DWORD_PTR mask = cpumask->cpuset;
  while (mask) {
    count++;
    mask &= (mask - 1);
  }
  return count;
#else
  return CPU_COUNT_S(CPU_ALLOC_SIZE(cpumask->cpucount), cpumask->cpuset);
#endif
}

#ifdef _WIN32
DLL_EXPORT_CROSSRUN DWORD_PTR crossrun_cpumask_get_os_mask (crossrun_cpumask cpumask)
#else
DLL_EXPORT_CROSSRUN cpu_set_t* crossrun_cpumask_get_os_mask (crossrun_cpumask cpumask)
#endif
{
  return cpumask->cpuset;
}

DLL_EXPORT_CROSSRUN int crossrun_get_current_affinity (crossrun_cpumask cpumask)
{
  if (!cpumask)
    return -1;
#ifdef _WIN32
  DWORD_PTR systemmask;
  cpumask->cpuset = 0;
  return (GetProcessAffinityMask(GetCurrentProcess(), &cpumask->cpuset, &systemmask) ? 0 : -1);
#else
  return sched_getaffinity(0, CPU_ALLOC_SIZE(cpumask->cpucount), cpumask->cpuset);
#endif
}

DLL_EXPORT_CROSSRUN int crossrun_set_current_affinity (crossrun_cpumask cpumask)
{
  if (!cpumask)
    return -1;
#ifdef _WIN32
  return (SetProcessAffinityMask(GetCurrentProcess(), cpumask->cpuset) ? 0 : -1);
#else
  return sched_setaffinity(0, CPU_ALLOC_SIZE(cpumask->cpucount), cpumask->cpuset);
#endif
}

/////See also: https://linux.die.net/man/3/cpu_set
/////See also: https://stackoverflow.com/questions/67565658/how-to-determine-which-cpus-are-online-on-linux
