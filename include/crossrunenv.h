/**
 * @file crossrunenv.h
 * @brief crossrun library header file with environment functions
 * @author Brecht Sanders
 *
 * This header file defines the functions for managing environment variables used by the crossrun library
 */

#ifndef __INCLUDED_CROSSRUNENV_H
#define __INCLUDED_CROSSRUNENV_H

/*! \cond PRIVATE */
#ifdef __WIN32__
#if defined(DLL_EXPORT) || defined(BUILD_CROSSRUN_DLL)
#define DLL_EXPORT_CROSSRUN __declspec(dllexport)
#elif !defined(STATIC) && !defined(BUILD_CROSSRUN_STATIC)
#define DLL_EXPORT_CROSSRUN __declspec(dllimport)
#else
#define DLL_EXPORT_CROSSRUN
#endif
#else
#define DLL_EXPORT_CROSSRUN
#endif
/*! \endcond */

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief data type for set of environment variables
 * \sa     crossrunenv_create_empty()
 * \sa     crossrunenv_create_from_system()
 * \sa     crossrunenv_free()
 * \sa     crossrun_open()
 */
typedef struct crossrunenv_data* crossrunenv;

/*! \brief create empty set of environment variables
 * \return structure with empty set of environment variables
 * \sa     crossrunenv_free()
 */
DLL_EXPORT_CROSSRUN crossrunenv crossrunenv_create_empty ();

/*! \brief create set of environment variables with current system values
 * \return structure with set of environment variables with system values
 * \sa     crossrunenv_free()
 */
DLL_EXPORT_CROSSRUN crossrunenv crossrunenv_create_from_system ();

/*! \brief free set of environment variables from memory
 * \param  environment   set of environment variables
 * \sa     crossrunenv_create_empty()
 * \sa     crossrunenv_create_from_system()
 */
DLL_EXPORT_CROSSRUN void crossrunenv_free (crossrunenv environment);

/*! \brief set value of a variable in set of environment variables
 * \param  environment   set of environment variables
 * \param  variable      name of the environment variable
 * \param  value         value of the environment variable
 * \sa     crossrunenv_get()
 * \sa     crossrunenv_create_empty()
 * \sa     crossrunenv_create_from_system()
 */
DLL_EXPORT_CROSSRUN void crossrunenv_set (crossrunenv* environment, const char* variable, const char* value);

/*! \brief get the value a variable in a set of environment variables
 * \param  environment   set of environment variables
 * \param  variable      name of the environment variable
 * \return value of the environment variable
 * \sa     crossrunenv_set()
 * \sa     crossrunenv_iterate()
 * \sa     crossrunenv_create_empty()
 * \sa     crossrunenv_create_from_system()
 */
DLL_EXPORT_CROSSRUN const char* crossrunenv_get (crossrunenv environment, const char* variable);

/*! \brief generate an environment block for use with CreateProcess() on Windows or execve() on other platforms
 * \param  environment   set of environment variables
 * \return environment block, the caller is responsible for calling crossrunenv_free_generated()
 * \sa     crossrun_open()
 */
#ifdef _WIN32
DLL_EXPORT_CROSSRUN char* crossrunenv_generate (crossrunenv environment);
#else
DLL_EXPORT_CROSSRUN char** crossrunenv_generate (crossrunenv environment);
#endif

/*! \brief free memory structure allocated by crossrunenv_generate()
 * \param  env           memory structure allocated by crossrunenv_generate()
 * \sa     crossrun_open()
 */
#ifdef _WIN32
DLL_EXPORT_CROSSRUN void crossrunenv_free_generated (char* env);
#else
DLL_EXPORT_CROSSRUN void crossrunenv_free_generated (char** env);
#endif

/*! \brief callback function type used when iterating through set of environment variables
 * \param  name          variable name
 * \param  value         variable value
 * \param  callbackdata  user data
 * \return 0 to continue processing, any other value to abort
 * \sa     crossrunenv_iterate()
 */
typedef int (*crossrunenv_process_fn) (const char* name, const char* value, void* callbackdata);

/*! \brief iterate through set of environment variables
 * \param  environment   set of environment variables
 * \param  callback      callback function to call for each environment variable
 * \param  callbackdata  user data to pass to callback function
 * \return 0 if all entries were processed or the result code of the callback function if aborted
 * \sa     crossrunenv_get()
 * \sa     crossrunenv_create_empty()
 * \sa     crossrunenv_create_from_system()
 */
DLL_EXPORT_CROSSRUN int crossrunenv_iterate (crossrunenv environment, crossrunenv_process_fn callback, void* callbackdata);

#ifdef __cplusplus
}
#endif

#endif //__INCLUDED_CROSSRUNENV_H
