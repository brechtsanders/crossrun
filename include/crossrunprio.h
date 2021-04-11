/**
 * @file crossrunprio.h
 * @brief crossrun library header file with process priority definitions and functions
 * @author Brecht Sanders
 *
 * This header file defines the definitions and functions for managing process priority used by the crossrun library
 */

#ifndef __INCLUDED_CROSSRUNPRIORITY_H
#define __INCLUDED_CROSSRUNPRIORITY_H

#include "crossrunenv.h"
#ifdef _WIN32
#include <windows.h>
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
 * \sa     crossrun_get_current_prio()
 */
DLL_EXPORT_CROSSRUN int crossrun_get_current_prio ();

#ifdef __cplusplus
}
#endif

#endif //__INCLUDED_CROSSRUNPRIORITY_H
