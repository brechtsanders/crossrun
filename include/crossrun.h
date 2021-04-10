#ifndef __INCLUDED_CROSSRUN_H
#define __INCLUDED_CROSSRUN_H

#include "crossrunenv.h"

/*! \brief version number constants
 * \sa     crossrun_get_version()
 * \sa     crossrun_get_version_string()
 * \name   CROSSRUN_VERSION_*
 * \{
 */
/*! \brief major version number */
#define CROSSRUN_VERSION_MAJOR 0
/*! \brief minor version number */
#define CROSSRUN_VERSION_MINOR 1
/*! \brief micro version number */
#define CROSSRUN_VERSION_MICRO 0
/*! @} */

/*! \brief packed version number */
#define CROSSRUN_VERSION (CROSSRUN_VERSION_MAJOR * 0x01000000 + CROSSRUN_VERSION_MINOR * 0x00010000 + CROSSRUN_VERSION_MICRO * 0x00000100)

/*! \cond PRIVATE */
#define CROSSRUN_VERSION_STRINGIZE_(major, minor, micro) #major"."#minor"."#micro
#define CROSSRUN_VERSION_STRINGIZE(major, minor, micro) CROSSRUN_VERSION_STRINGIZE_(major, minor, micro)
/*! \endcond */

/*! \brief string with dotted version number \hideinitializer */
#define CROSSRUN_VERSION_STRING CROSSRUN_VERSION_STRINGIZE(CROSSRUN_VERSION_MAJOR, CROSSRUN_VERSION_MINOR, CROSSRUN_VERSION_MICRO)

/*! \brief string with name of mylibrary library */
#define CROSSRUN_NAME "crossrun"

/*! \brief string with name and version of mylibrary library \hideinitializer */
#define CROSSRUN_FULLNAME CROSSRUN_NAME " " CROSSRUN_VERSION_STRING

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief get crossrun library version string
 * \param  pmajor        pointer to integer that will receive major version number
 * \param  pminor        pointer to integer that will receive minor version number
 * \param  pmicro        pointer to integer that will receive micro version number
 * \sa     crossrun_get_version_string()
 */
DLL_EXPORT_CROSSRUN void crossrun_get_version (int* pmajor, int* pminor, int* pmicro);

/*! \brief get crossrun library version string
 * \return version string
 * \sa     crossrun_get_version()
 */
DLL_EXPORT_CROSSRUN const char* crossrun_get_version_string ();

//!data type for handling shell process
typedef struct crossrun_data* crossrun;

//!open a shell process
/*!
  \param  command     shell command to execute
  \return shell process handle or NULL on error
*/
DLL_EXPORT_CROSSRUN crossrun crossrun_open (const char* command, crossrunenv environment);

//!check if shell process finished
/*!
  \param  handle      shell process handle
  \return 0 if process is still runing, nonzero if process is no longer running
*/
DLL_EXPORT_CROSSRUN int crossrun_stopped (crossrun handle);

//!wait for shell process to finish
/*!
  \param  handle      shell process handle
  \return 0 if process is still running, nonzero if process is no longer running
*/
DLL_EXPORT_CROSSRUN int crossrun_wait (crossrun handle);

//!get exit code of finished process
/*!
  \param  handle      shell process handle
  \return exit code of finished process (or undefined if not finished)
*/
DLL_EXPORT_CROSSRUN unsigned long crossrun_get_exit_code (crossrun handle);

//!tell shell process to close down (you should wait for it or kill it) and free handle
/*!
  \param  handle      shell process handle
*/
DLL_EXPORT_CROSSRUN void crossrun_close (crossrun handle);

//!kill shell process
/*!
  \param  handle      shell process handle
*/
DLL_EXPORT_CROSSRUN void crossrun_kill (crossrun handle);

//!clean up shell process handle
/*!
  \param  handle      shell process handle
*/
DLL_EXPORT_CROSSRUN void crossrun_free (crossrun handle);

//!check if data is waiting to be read from a shell process
/*!
  \param  handle      shell process handle
  \return number of bytes waiting to be read or -1 on error
*/
DLL_EXPORT_CROSSRUN int crossrun_data_waiting (crossrun handle);

//!read data from a shell process
/*!
  \param  handle      shell process handle
  \param  buf         buffer
  \param  buflen      size of buffer in bytes
  \return number of bytes read (can be less than buflen), 0 on end of file or -1 on error
*/
DLL_EXPORT_CROSSRUN int crossrun_read (crossrun handle, char* buf, int buflen);

//!read data from a shell process if data is available
/*!
  \param  handle      shell process handle
  \param  buf         buffer
  \param  buflen      size of buffer in bytes
  \return number of bytes read (can be 0 if no data was available yet) or -1 on error
*/
DLL_EXPORT_CROSSRUN int crossrun_read_available (crossrun handle, char* buf, int buflen);

//!write to shell process
/*!
  \param  handle      shell process handle
  \param  data        data buffer to write
  \param  datalen     size of data buffer to write
  \return 0 on success
*/
DLL_EXPORT_CROSSRUN int crossrun_writedata (crossrun handle, const char* data, int datalen);

//!write string to shell process
/*!
  \param  handle      shell process handle
  \param  data        string to write
  \return 0 on success
*/
DLL_EXPORT_CROSSRUN int crossrun_write (crossrun handle, const char* data);

//!close the standard input of a shell process
/*!
  \param  handle      shell process handle
*/
DLL_EXPORT_CROSSRUN void crossrun_write_eof (crossrun handle);

#ifdef __cplusplus
}
#endif

#endif //__INCLUDED_CROSSRUN_H
