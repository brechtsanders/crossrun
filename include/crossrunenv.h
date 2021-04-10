#ifndef __INCLUDED_CROSSRUNENVIRONMENT_H
#define __INCLUDED_CROSSRUNENVIRONMENT_H

#ifdef __WIN32__
#if defined(DLL_EXPORT) || defined(BUILD_CROSSRUN_DLL)
#define DLL_EXPORT_CROSSRUN __declspec(dllexport)
#elif !defined(STATIC) && !defined(BUILD_CROSSRUNSTATIC)
#define DLL_EXPORT_CROSSRUN __declspec(dllimport)
#else
#define DLL_EXPORT_CROSSRUN
#endif
#else
#define DLL_EXPORT_CROSSRUN
#endif

#ifdef __cplusplus
extern "C" {
#endif

//!data type for set of environment variables
typedef struct crossrunenv_data* crossrunenv;

//!create empty set of environment variables
/*!
  \return structure with empty set of environment variables
*/
DLL_EXPORT_CROSSRUN crossrunenv crossrunenv_create_empty ();

//!create set of environment variables with current system values
/*!
  \return structure with set of environment variables with system values
*/
DLL_EXPORT_CROSSRUN crossrunenv crossrunenv_create_from_system ();

//!free set of environment variables from memory
/*!
  \param  environment   set of environment variables
*/
DLL_EXPORT_CROSSRUN void crossrunenv_free (crossrunenv environment);

//!set value of a variable in set of environment variables
/*!
  \param  environment   set of environment variables
  \param  variable      name of the environment variable
  \param  value         value of the environment variable
*/
DLL_EXPORT_CROSSRUN void crossrunenv_set (crossrunenv* environment, const char* variable, const char* value);

//!get the value a variable in a set of environment variables
/*!
  \param  environment   set of environment variables
  \param  variable      name of the environment variable
  \return value of the environment variable
*/
DLL_EXPORT_CROSSRUN const char* crossrunenv_get (crossrunenv environment, const char* variable);

//!generate an environment block for use with CreateProcess() on Windows or execve() on other platforms
/*!
  \param  environment   set of environment variables
  \return environment block, the caller is responsible for calling crossrunenv_free_generated()
*/
#ifdef _WIN32
DLL_EXPORT_CROSSRUN char* crossrunenv_generate (crossrunenv environment);
#else
DLL_EXPORT_CROSSRUN char** crossrunenv_generate (crossrunenv environment);
#endif

//!free memory structure allocated by crossrunenv_generate()
/*!
  \param  env           memory structure allocated by crossrunenv_generate()
*/
#ifdef _WIN32
DLL_EXPORT_CROSSRUN void crossrunenv_free_generated (char* env);
#else
DLL_EXPORT_CROSSRUN void crossrunenv_free_generated (char** env);
#endif

//!callback function type used when iterating through set of environment variables
/*!
  \param  name          variable name
  \param  value         variable value
  \param  callbackdata  user data
  \return 0 to continue processing, any other value to abort
*/
typedef int (*crossrunenv_process_fn) (const char* name, const char* value, void* callbackdata);

//!iterate through set of environment variables
/*!
  \param  environment   set of environment variables
  \param  callback      callback function to call for each environment variable
  \param  callbackdata  user data to pass to callback function
  \return 0 if all entries were processed or the result code of the callback function if aborted
*/
DLL_EXPORT_CROSSRUN int crossrunenv_iterate (crossrunenv environment, crossrunenv_process_fn callback, void* callbackdata);

#ifdef __cplusplus
}
#endif

#endif //__INCLUDED_CROSSRUNENVIRONMENT_H
