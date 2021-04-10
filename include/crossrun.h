#ifndef __INCLUDED_CROSSRUN_H
#define __INCLUDED_CROSSRUN_H

#include "crossrunenv.h"

#ifdef __cplusplus
extern "C" {
#endif

//!data type for handling a shell process
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

//!wait for a shell process to finish
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

//!kill a shell process
/*!
  \param  handle      shell process handle
*/
DLL_EXPORT_CROSSRUN void crossrun_kill (crossrun handle);

//!tell shell process to close down (you should wait for it or kill it) and free handle
/*!
  \param  handle      shell process handle
*/
DLL_EXPORT_CROSSRUN void crossrun_close (crossrun handle);

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
