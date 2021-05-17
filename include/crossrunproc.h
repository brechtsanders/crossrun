/**
 * @file crossrunproc.h
 * @brief crossrun library header file with process priority definitions and functions
 * @author Brecht Sanders
 *
 * This header file defines the definitions and functions for managing process priority used by the crossrun library
 */

#ifndef __INCLUDED_CROSSRUNPROC_H
#define __INCLUDED_CROSSRUNPROC_H

#include "crossrunenv.h"
#include <stddef.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sched.h>
#endif


/*! \brief version number constants
 * \sa     crossrun_open()
 * \sa     crossrun_get_current_prio()
 * \name   CROSSRUN_PRIO_*
 * \{
 */
/*! \brief invalid process priority */
#define CROSSRUN_PRIO_ERROR             0
/*! \brief low process priority */
#define CROSSRUN_PRIO_LOW               1
/*! \brief below normal process priority */
#define CROSSRUN_PRIO_BELOW_NORMAL      2
/*! \brief normal process priority */
#define CROSSRUN_PRIO_NORMAL            3
/*! \brief above normal process priority */
#define CROSSRUN_PRIO_ABOVE_NORMAL      4
/*! \brief high process priority */
#define CROSSRUN_PRIO_HIGH              5
/*! @} */

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief text descriptions of the different CROSSRUN_PRIO_* priority levels
 * \sa     CROSSRUN_PRIO_*
 * \sa     crossrun_get_current_prio()
 * \sa     crossrun_open()
 */
DLL_EXPORT_CROSSRUN extern const char* crossrun_prio_name[];

/*! \brief operating specific value representing the different CROSSRUN_PRIO_* priority levels
 * \sa     CROSSRUN_PRIO_*
 * \sa     crossrun_open()
 */
#ifdef _WIN32
DLL_EXPORT_CROSSRUN extern DWORD crossrun_prio_os_value[];
#else
DLL_EXPORT_CROSSRUN extern int crossrun_prio_os_value[];
#endif

/*! \brief get priority of current process
 * \return process priority value as CROSSRUN_PRIO_*
 * \sa     CROSSRUN_PRIO_*
 * \sa     crossrun_set_current_prio()
 */
DLL_EXPORT_CROSSRUN int crossrun_get_current_prio ();

/*! \brief set priority of current process
 * \param  priority      desired process priority value as CROSSRUN_PRIO_* (note that most operating systems only allow current or lower priority)
 * \return zero on success, non-zero on error
 * \sa     CROSSRUN_PRIO_*
 * \sa     crossrun_get_current_prio()
 */
DLL_EXPORT_CROSSRUN int crossrun_set_current_prio (int priority);

/*! \brief get current process ID
 * \return process ID of current process
 */
DLL_EXPORT_CROSSRUN unsigned long crossrun_get_current_pid ();

/*! \brief get number of logical processors
 * \return number of logical processors
 */
DLL_EXPORT_CROSSRUN unsigned long crossrun_get_logical_processors ();

/*! \brief data type for logical processor mask
 * \sa     crossrun_cpumask_create
 * \sa     crossrun_cpumask_free
 */
typedef struct crossrun_cpumask_struct* crossrun_cpumask;

/*! \brief create data structure for logical processor mask
 * \return data structure for logical processor mask or NULL on error (for example on platforms where affinity is not supported)
 * \sa     crossrun_cpumask
 * \sa     crossrun_cpumask_free
 */
DLL_EXPORT_CROSSRUN crossrun_cpumask crossrun_cpumask_create ();

/*! \brief destroy data structure for logical processor mask
 * \param  cpumask       logical processor mask
 * \sa     crossrun_cpumask
 * \sa     crossrun_cpumask_create
 */
DLL_EXPORT_CROSSRUN void crossrun_cpumask_free (crossrun_cpumask cpumask);

/*! \brief get number of available logical processors
 * \param  cpumask       logical processor mask
 * \return number of logical processors
 * \sa     crossrun_cpumask
 */
DLL_EXPORT_CROSSRUN size_t crossrun_cpumask_get_cpus (crossrun_cpumask cpumask);

/*! \brief clear all processors in logical processor mask
 * \param  cpumask       logical processor mask
 * \sa     crossrun_cpumask
 */
DLL_EXPORT_CROSSRUN void crossrun_cpumask_clear_all (crossrun_cpumask cpumask);

/*! \brief set all processors in logical processor mask
 * \param  cpumask       logical processor mask
 * \sa     crossrun_cpumask
 */
DLL_EXPORT_CROSSRUN void crossrun_cpumask_set_all (crossrun_cpumask cpumask);

/*! \brief set specific processor in logical processor mask
 * \param  cpumask       logical processor mask
 * \param  cpuindex      logical processor number (0-based index)
 * \sa     crossrun_cpumask
 */
DLL_EXPORT_CROSSRUN void crossrun_cpumask_set (crossrun_cpumask cpumask, int cpuindex);

/*! \brief check specific processor in logical processor mask
 * \param  cpumask       logical processor mask
 * \param  cpuindex      logical processor number (0-based index)
 * \return non-zero if set or zero if not set
 * \sa     crossrun_cpumask
 */
DLL_EXPORT_CROSSRUN int crossrun_cpumask_is_set (crossrun_cpumask cpumask, int cpuindex);

/*! \brief count processors set in logical processor mask
 * \param  cpumask       logical processor mask
 * \return number of processors set
 * \sa     crossrun_cpumask
 */
DLL_EXPORT_CROSSRUN size_t crossrun_cpumask_count (crossrun_cpumask cpumask);

/*! \brief get OS-specific format of logical processor mask
 * \param  cpumask       logical processor mask
 * \return OS-specific data
 */
#if defined(_WIN32)
DLL_EXPORT_CROSSRUN DWORD_PTR crossrun_cpumask_get_os_mask (crossrun_cpumask cpumask);
#elif defined(__APPLE__)
DLL_EXPORT_CROSSRUN unsigned long crossrun_cpumask_get_os_mask (crossrun_cpumask cpumask);
#else
DLL_EXPORT_CROSSRUN cpu_set_t* crossrun_cpumask_get_os_mask (crossrun_cpumask cpumask);
#endif

/*! \brief set logical processor mask to affinity mask of current process
 * \param  cpumask       logical processor mask
 * \return zero on success or non-zero on error
 * \sa     crossrun_cpumask
 */
DLL_EXPORT_CROSSRUN int crossrun_get_current_affinity (crossrun_cpumask cpumask);

/*! \brief set affinity mask of current process to logical processor mask
 * \param  cpumask       logical processor mask
 * \return zero on success or non-zero on error
 * \sa     crossrun_cpumask
 */
DLL_EXPORT_CROSSRUN int crossrun_set_current_affinity (crossrun_cpumask cpumask);

#ifdef __cplusplus
}
#endif

#endif //__INCLUDED_CROSSRUNPROC_H
