#include "crossrun.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
//#define _GNU_SOURCE
#include <unistd.h>
//#include <fcntl.h>
#include <errno.h>
#include <signal.h>
//#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

//#define SHOWERROR(format, ...) fprintf(stderr, format "\n" __VA_OPT__(,) __VA_ARGS__);
#define SHOWERROR(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");

//#define WITH_STDERR

#define PIPE_READ  0
#define PIPE_WRITE 1

DLL_EXPORT_CROSSRUN void crossrun_get_version (int* pmajor, int* pminor, int* pmicro)
{
  if (pmajor)
    *pmajor = CROSSRUN_VERSION_MAJOR;
  if (pminor)
    *pminor = CROSSRUN_VERSION_MINOR;
  if (pmicro)
    *pmicro = CROSSRUN_VERSION_MICRO;
}

DLL_EXPORT_CROSSRUN const char* crossrun_get_version_string ()
{
  return CROSSRUN_VERSION_STRING;
}

struct crossrun_data {
#ifdef _WIN32
  HANDLE stdin_pipe[2];           //pipe for process standard input
  HANDLE stdout_pipe[2];          //pipe for process standard output
#ifdef WITH_STDERR
  HANDLE stderr_pipe[2];          //pipe for process error output
#endif
  PROCESS_INFORMATION proc_info;  //Windows process information structure
  DWORD exitcode;                 //exit code after process exited
#else
  int stdin_pipe[2];              //pipe for process standard input
  int stdout_pipe[2];             //pipe for process standard output
#ifdef WITH_STDERR
  int stderr_pipe[2];             //pipe for process error output
#endif
  pid_t pid;                      //process ID
  int exitcode;                   //exit code after process exited
#endif
  int exited;
};

#ifndef _WIN32
#ifdef _WIN32
static inline char* strndup (const char* s, size_t n)
{
  char* result;
  size_t len = strlen(s);
  if (len > n)
    len = n;
  result = (char*)malloc(len + 1);
  if (!result)
    return 0;
  result[len] = 0;
  return (char*)memcpy(result, s, len);
}
#endif

int command_to_argv (const char* command, char*** argv)
{
  const char* p;
  const char* q;
  char* buf;
  size_t len;
  char quote = 0;
  size_t argc = 0;
  if (!command || !*command || !argv)
    return -1;
  *argv = NULL;
  p = command;
  //process command line
  while (*p) {
    //find next space
    q = p;
    while (*q && !((*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n') && !quote)) {
      if (*q == quote)
        quote = 0;
      else if (*q == '"')
        quote = *q;
      q++;
    }
    //allocate additional space for new pointer
    if ((*argv = (char**)realloc(*argv, (argc + 2) * sizeof(char*))) == NULL)
      return -1;
    //create copy of data
    len = q - p;
    buf = strndup(p, len);
    //remove surrounding quotes if needed
    if (len >= 2 && buf[0] == '"' && buf[len - 1] == '"') {
      memmove(buf, buf + 1, len - 2);
      buf[len - 2] = 0;
    }
    //set pointer to copy of data
    (*argv)[argc++] = buf;
    //abort if end of command line reached
    if (!*q)
      break;
    p = q + 1;
  }
  if (*argv)
    (*argv)[argc] = NULL;
  return 0;
}

void free_argv (char** argv)
{
  char** arg;
  if (!argv)
    return;
  arg = argv;
  while (*arg) {
    free(*arg);
    arg++;
  }
  free(argv);
}
#endif

DLL_EXPORT_CROSSRUN crossrun crossrun_open (const char* command, crossrunenv environment, int priority, crossrun_cpumask affinity)
{
  crossrun handle;
  //allocate data structure
  if ((handle = (struct crossrun_data*)malloc(sizeof(struct crossrun_data))) == NULL) {
    SHOWERROR("Memory allocation error")
    return NULL;
  }
  handle->exitcode = 0;
  handle->exited = 0;
#ifdef _WIN32
  SECURITY_ATTRIBUTES sattr;
  //allocate data structure
  //create pipes and make the end for the shell process inheritable
  sattr.nLength = sizeof(SECURITY_ATTRIBUTES);
  sattr.lpSecurityDescriptor = NULL;
  sattr.bInheritHandle = TRUE;
  if (!CreatePipe(&handle->stdin_pipe[PIPE_READ], &handle->stdin_pipe[PIPE_WRITE], &sattr, 0)) {
    SHOWERROR("Error in CreatePipe()")
    free(handle);
    return NULL;
  }
  if (!SetHandleInformation(handle->stdin_pipe[PIPE_WRITE], HANDLE_FLAG_INHERIT, 0)) {
    SHOWERROR("Error in SetHandleInformation()")
    CloseHandle(handle->stdin_pipe[PIPE_READ]);
    CloseHandle(handle->stdin_pipe[PIPE_WRITE]);
    free(handle);
    return NULL;
  }
  if (!CreatePipe(&handle->stdout_pipe[PIPE_READ], &handle->stdout_pipe[PIPE_WRITE], &sattr, 0)) {
    SHOWERROR("Error in CreatePipe()")
    CloseHandle(handle->stdin_pipe[PIPE_READ]);
    CloseHandle(handle->stdin_pipe[PIPE_WRITE]);
    free(handle);
    return NULL;
  }
  if (!SetHandleInformation(handle->stdout_pipe[PIPE_READ], HANDLE_FLAG_INHERIT, 0)) {
    SHOWERROR("Error in SetHandleInformation()")
    CloseHandle(handle->stdin_pipe[PIPE_READ]);
    CloseHandle(handle->stdin_pipe[PIPE_WRITE]);
    CloseHandle(handle->stdout_pipe[PIPE_READ]);
    CloseHandle(handle->stdout_pipe[PIPE_WRITE]);
    free(handle);
    return NULL;
  }
#ifdef WITH_STDERR
  if (!CreatePipe(&handle->stderr_pipe[PIPE_READ], &handle->stderr_pipe[PIPE_WRITE], &sattr, 0)) {
    SHOWERROR("Error in CreatePipe()")
    CloseHandle(handle->stdin_pipe[PIPE_READ]);
    CloseHandle(handle->stdin_pipe[PIPE_WRITE]);
    CloseHandle(handle->stdout_pipe[PIPE_READ]);
    CloseHandle(handle->stdout_pipe[PIPE_WRITE]);
    free(handle);
    return NULL;
  }
  if (!SetHandleInformation(handle->stderr_pipe[PIPE_READ], HANDLE_FLAG_INHERIT, 0)) {
    SHOWERROR("Error in SetHandleInformation()")
    CloseHandle(handle->stdin_pipe[PIPE_READ]);
    CloseHandle(handle->stdin_pipe[PIPE_WRITE]);
    CloseHandle(handle->stdout_pipe[PIPE_READ]);
    CloseHandle(handle->stdout_pipe[PIPE_WRITE]);
    CloseHandle(handle->stderr_pipe[PIPE_READ]);
    CloseHandle(handle->stderr_pipe[PIPE_WRITE]);
    free(handle);
    return NULL;
  }
#endif
  //create process
  char* cmd = strdup(command);
  char* envbuf = (environment ? crossrunenv_generate(environment) : NULL);
  STARTUPINFO startupinfo;
  ZeroMemory(&startupinfo, sizeof(startupinfo));
  startupinfo.cb = sizeof(startupinfo);
  startupinfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
  startupinfo.wShowWindow = SW_HIDE;
  startupinfo.hStdInput = handle->stdin_pipe[PIPE_READ];
  startupinfo.hStdOutput = handle->stdout_pipe[PIPE_WRITE];
#ifdef WITH_STDERR
  startupinfo.hStdError = handle->stderr_pipe[PIPE_WRITE];
#else
  startupinfo.hStdError = handle->stdout_pipe[PIPE_WRITE];
#endif
  if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, (priority > 0 && priority <= CROSSRUN_PRIO_HIGH ? crossrun_prio_os_value[priority] : 0) /*| CREATE_NEW_CONSOLE*/ | CREATE_NO_WINDOW, envbuf, NULL, &startupinfo, &handle->proc_info)) {
    SHOWERROR("Error in CreateProcess()")
    CloseHandle(handle->stdin_pipe[PIPE_READ]);
    CloseHandle(handle->stdin_pipe[PIPE_WRITE]);
    CloseHandle(handle->stdout_pipe[PIPE_READ]);
    CloseHandle(handle->stdout_pipe[PIPE_WRITE]);
#ifdef WITH_STDERR
    CloseHandle(handle->stderr_pipe[PIPE_READ]);
    CloseHandle(handle->stderr_pipe[PIPE_WRITE]);
#endif
    free(handle);
    return NULL;
  }
  //set requested process affinity
  if (affinity) {
    SetProcessAffinityMask(handle->proc_info.hProcess, crossrun_cpumask_get_os_mask(affinity));
  }
  //clean up
  free(cmd);
  crossrunenv_free_generated(envbuf);
  //close thread handle (no longer needed)
  CloseHandle(handle->proc_info.hThread);
  //close pipe handles only used by the process
  CloseHandle(handle->stdin_pipe[PIPE_READ]);
  CloseHandle(handle->stdout_pipe[PIPE_WRITE]);
#ifdef WITH_STDERR
  CloseHandle(handle->stderr_pipe[PIPE_WRITE]);
#endif
#else
  char** argv;
  char** envbuf;
  //split command in separate arguments
  if (command_to_argv(command, &argv) != 0) {
    SHOWERROR("Error processing command line")
    return NULL;
  }
  //generate environment
  envbuf = (environment ? crossrunenv_generate(environment) : NULL);
  //create pipes
  if (pipe(handle->stdin_pipe) < 0) {
    SHOWERROR("Error in pipe()")
    return NULL;
  }
  if (pipe(handle->stdout_pipe) < 0) {
    SHOWERROR("Error in pipe()")
    close(handle->stdin_pipe[PIPE_READ]);
    close(handle->stdin_pipe[PIPE_WRITE]);
    free(handle);
    return NULL;
  }
#ifdef WITH_STDERR
  if (pipe(handle->stderr_pipe) < 0) {
    SHOWERROR("Error in pipe()")
    close(handle->stdin_pipe[PIPE_READ]);
    close(handle->stdin_pipe[PIPE_WRITE]);
    close(handle->stdout_pipe[PIPE_READ]);
    close(handle->stdout_pipe[PIPE_WRITE]);
    free(handle);
    return NULL;
  }
#endif
  //fork
  if ((handle->pid = fork()) < 0) {
    //fork failed
    SHOWERROR("Error in fork()")
    close(handle->stdin_pipe[PIPE_READ]);
    close(handle->stdin_pipe[PIPE_WRITE]);
    close(handle->stdout_pipe[PIPE_READ]);
    close(handle->stdout_pipe[PIPE_WRITE]);
#ifdef WITH_STDERR
    close(handle->stderr_pipe[PIPE_READ]);
    close(handle->stderr_pipe[PIPE_WRITE]);
#endif
    free(handle);
    return NULL;
  } else if (handle->pid == 0) {
    //child process
    //set requested priority
    if (priority > 0 && priority <= CROSSRUN_PRIO_HIGH)
      setpriority(PRIO_PROCESS, 0, crossrun_prio_os_value[priority]);
    //set requested process affinity
    if (affinity)
      crossrun_cpumask_set_current_affinity(affinity);
    //reroute standard input to read end of pipe (use loop to cover possibility of being interrupted by signal) and close other end of pipe
    while ((dup2(handle->stdin_pipe[PIPE_READ], STDIN_FILENO) == -1) && (errno == EINTR))
      ;
    close(handle->stdin_pipe[PIPE_READ]);
    close(handle->stdin_pipe[PIPE_WRITE]);
    //reroute standard output to write end of pipe (use loop to cover possibility of being interrupted by signal) and close other end of pipe
    while ((dup2(handle->stdout_pipe[PIPE_WRITE], STDOUT_FILENO) == -1) && (errno == EINTR))
      ;
    close(handle->stdout_pipe[PIPE_WRITE]);
    close(handle->stdout_pipe[PIPE_READ]);
#ifdef WITH_STDERR
    //reroute error output to write end of pipe (use loop to cover possibility of being interrupted by signal) and close other end of pipe
    while ((dup2(handle->stderr_pipe[PIPE_WRITE], STDOUT_FILENO) == -1) && (errno == EINTR))
      ;
    close(handle->stderr_pipe[PIPE_WRITE]);
    close(handle->stderr_pipe[PIPE_READ]);
#endif
    if (execve(*argv, argv, envbuf) < 0)
      SHOWERROR("Error in executing program")
    return NULL;
  } else {
    //parent process
    //close read end of standard input pipe
    close(handle->stdin_pipe[PIPE_READ]);
    //close write end of standard output pipe
    close(handle->stdout_pipe[PIPE_WRITE]);
#ifdef WITH_STDERR
    //close write end of error output pipe
    close(handle->stderr_pipe[PIPE_WRITE]);
#endif
  }
  //clean up
  crossrunenv_free_generated(envbuf);
  free_argv(argv);
#endif
  return handle;
}

DLL_EXPORT_CROSSRUN unsigned long crossrun_get_pid (crossrun handle)
{
  if (!handle)
    return 0;
#ifdef _WIN32
  return handle->proc_info.dwProcessId;
#else
  return handle->pid;
#endif
}

DLL_EXPORT_CROSSRUN int crossrun_stopped (crossrun handle)
{
  if (handle->exited)
    return 1;
#ifdef _WIN32
  if (handle->proc_info.hProcess == 0)
    return 1;
  if (WaitForSingleObject(handle->proc_info.hProcess, 0) == WAIT_TIMEOUT)
    return 0;
  if (!GetExitCodeProcess(handle->proc_info.hProcess, &handle->exitcode))
    handle->exitcode = ~0;
#else
  int status;
  if (waitpid(handle->pid, &status, WNOHANG | WUNTRACED) == -1) {
    handle->exitcode = ~0;
    return 0;
  }
  if (WIFEXITED(status))
    handle->exitcode = WEXITSTATUS(status);
  else
    handle->exitcode = ~0;
#endif
  handle->exited = 1;
  return 1;
}

DLL_EXPORT_CROSSRUN int crossrun_wait (crossrun handle)
{
  if (!handle || handle->exited)
    return 1;
#ifdef _WIN32
  if (handle->proc_info.hProcess != 0) {
    //if (WaitForSingleObject(handle->proc_info.hProcess, (miliseconds == 0 ? INFINITE : miliseconds)) == WAIT_TIMEOUT) {
    if (WaitForSingleObject(handle->proc_info.hProcess, INFINITE) == WAIT_TIMEOUT) {
      handle->exitcode = ~0;
      return 0;
    }
    if (!GetExitCodeProcess(handle->proc_info.hProcess, &handle->exitcode))
      handle->exitcode = ~0;
  }
#else
  int status;
  if (waitpid(handle->pid, &status, WUNTRACED) == -1) {
    handle->exitcode = ~0;
    return 0;
  }
  if (WIFEXITED(status))
    handle->exitcode = WEXITSTATUS(status);
  else
    handle->exitcode = ~0;
#endif
  handle->exited = 1;
  return 1;
}

DLL_EXPORT_CROSSRUN unsigned long crossrun_get_exit_code (crossrun handle)
{
  if (!handle->exited)
    crossrun_wait(handle);
  return (unsigned long)handle->exitcode;
}

DLL_EXPORT_CROSSRUN void crossrun_close (crossrun handle)
{
#ifdef _WIN32
  if (handle->stdin_pipe[PIPE_WRITE]) {
    CloseHandle(handle->stdin_pipe[PIPE_WRITE]);
    handle->stdin_pipe[PIPE_WRITE] = NULL;
  }
  if (handle->stdout_pipe[PIPE_READ]) {
    CloseHandle(handle->stdout_pipe[PIPE_READ]);
    handle->stdout_pipe[PIPE_READ] = NULL;
  }
#ifdef WITH_STDERR
  if (handle->stderr_pipe[PIPE_READ]) {
    CloseHandle(handle->stderr_pipe[PIPE_READ]);
    handle->stderr_pipe[PIPE_READ] = NULL;
  }
#endif
  if (handle->proc_info.hProcess != 0) {
    CloseHandle(handle->proc_info.hProcess);
    handle->proc_info.hProcess = 0;
  }
#else
  if (handle->stdin_pipe[PIPE_WRITE] >= 0) {
    close(handle->stdin_pipe[PIPE_WRITE]);
    handle->stdin_pipe[PIPE_WRITE] = -1;
  }
  if (handle->stdout_pipe[PIPE_READ] >= 0) {
    close(handle->stdout_pipe[PIPE_READ]);
    handle->stdout_pipe[PIPE_READ] = -1;
  }
#ifdef WITH_STDERR
  if (handle->stderr_pipe[PIPE_READ] >= 0) {
    close(handle->stderr_pipe[PIPE_READ]);
    handle->stderr_pipe[PIPE_READ] = -1;
  }
#endif
  //kill(handle->pid, SIGTERM);
#endif
}

DLL_EXPORT_CROSSRUN void crossrun_kill (crossrun handle)
{
#ifdef _WIN32
  TerminateProcess(handle->proc_info.hProcess, 256);
#else
  kill(handle->pid, SIGKILL);
#endif
}

DLL_EXPORT_CROSSRUN void crossrun_free (crossrun handle)
{
  if (!handle)
    return;
  crossrun_close(handle);
  free(handle);
}

DLL_EXPORT_CROSSRUN int crossrun_data_waiting (crossrun handle)
{
#ifdef _WIN32
  DWORD n = 0;
  if (!PeekNamedPipe(handle->stdout_pipe[PIPE_READ], NULL, 0, NULL, &n, NULL))
    return -1;
  return n;
#else
  int n;
/*
  fd_set rfds;
  fd_set xfds;
  struct timeval tv;
  FD_ZERO(&rfds);
  FD_SET(handle->stdout_pipe[PIPE_READ], &rfds);
  FD_ZERO(&xfds);
  FD_SET(handle->stdout_pipe[PIPE_READ], &xfds);
  tv.tv_sec = 0;
  tv.tv_usec = 1;
  n = select(1, &rfds, NULL, &xfds, &tv);
  if (n == -1)
    perror("select()");
  //else if (n == 0)
  //  return 0;
  else
    printf("select() returned %i\n", n);
*/
/*
  struct pollfd pollinfo;
  pollinfo.fd = handle->stdout_pipe[PIPE_READ];
  pollinfo.events = POLLIN |
#ifdef POLLRDHUP
    POLLRDHUP |
#endif
    POLLERR | POLLHUP | POLLNVAL;
  pollinfo.revents = 0;
  if ((n = poll(&pollinfo, 1, 0)) <= 0)
    return n;
  if (pollinfo.events | POLLIN == 0)
    return -1;
*/
  n = -1;
  if (ioctl(handle->stdout_pipe[PIPE_READ], FIONREAD, &n) < 0)
    return -1;
  if (n == 0) {
    n = waitpid(handle->pid, NULL, WNOHANG | WUNTRACED);
    return (n < 0 || n == handle->pid ? -1 : 0);
  }
  return n;
#endif
}

DLL_EXPORT_CROSSRUN int crossrun_read (crossrun handle, char* buf, int buflen)
{
#ifdef _WIN32
  DWORD n;
  //read data
  if (!ReadFile(handle->stdout_pipe[PIPE_READ], buf, buflen, &n, NULL))
    return -1;
  return n;
#else
  ssize_t n;
  //read data
  if ((n = read(handle->stdout_pipe[PIPE_READ], buf, buflen)) < 0)
    return -1;
  return n;
#endif
}

DLL_EXPORT_CROSSRUN int crossrun_read_available (crossrun handle, char* buf, int buflen)
{
  int n;
  int bufpos = 0;
  if ((n = crossrun_data_waiting(handle)) < 0)
    return -1;
/*
  if (n == 0)
    return (crossrun_stopped(handle, NULL) ? -1 : 0);
*/
  do {
    if (n > buflen - bufpos)
      n = buflen - bufpos;
    if ((n = crossrun_read(handle, buf + bufpos, n)) <= 0)
      break;
    if ((bufpos += n) >= buflen)
      break;
  } while ((n = crossrun_data_waiting(handle)) > 0);
  return bufpos;
/*
#ifdef _WIN32
  DWORD n;
  BOOL status = TRUE;
  int bufpos = 0;
  //stop if no data is waiting to be read
  if (!PeekNamedPipe(handle->stdout_pipe[PIPE_READ], NULL, 0, NULL, &n, NULL))
    return -1;
  if (n == 0)
    return (crossrun_stopped(handle, NULL) ? -1 : 0);
  if (buflen <= 0)
    return 0;
  //read available data
  while ((status = ReadFile(handle->stdout_pipe[PIPE_READ], buf + bufpos, buflen - bufpos, &n, NULL)) && n > 0) {
    //keep track of bytes read and stop if buffer is full
    if ((bufpos += n) >= buflen)
      break;
    //stop if no more data is available
    if (!(status = PeekNamedPipe(handle->stdout_pipe[PIPE_READ], NULL, 0, NULL, &n, NULL)) || n == 0)
      break;
  }
  if (!status && bufpos == 0)
    return -1;
  return bufpos;
#else
  struct pollfd pollinfo;
  pollinfo.fd = handle->stdout_pipe[PIPE_READ];
  pollinfo.events = POLLIN |
#ifdef POLLRDHUP
    POLLRDHUP |
#endif
    POLLERR | POLLHUP | POLLNVAL;
  pollinfo.revents = 0;
  if (poll(&pollinfo, 1, 0) > 0) {
    if (pollinfo.events | POLLIN) {
      ssize_t n;
      //read data
      if ((n = read(handle->stdout_pipe[PIPE_READ], buf, 1)) < 0) /////not number of bytes available
        return -1;
      return n;
    }
  }
  return 0;
#endif
*/
}

DLL_EXPORT_CROSSRUN int crossrun_writedata (crossrun handle, const char* data, int datalen)
{
#ifdef _WIN32
  if (!handle->stdin_pipe[PIPE_WRITE])
    return -1;
  DWORD n;
  int pos = 0;
  int dataleft = datalen;
  do {
    if (!WriteFile(handle->stdin_pipe[PIPE_WRITE], data + pos, dataleft, &n, NULL))
      return -1;
    pos += n;
  } while (n < dataleft);
#else
  if (handle->stdin_pipe[PIPE_WRITE] < 0)
    return -1;
  ssize_t n;
  int pos = 0;
  int dataleft = datalen;
  do {
    if ((n = write(handle->stdin_pipe[PIPE_WRITE], data + pos, dataleft)) < 0)
      return -1;
    pos += n;
  } while (n < dataleft);
#endif
  return 0;
}

DLL_EXPORT_CROSSRUN int crossrun_write (crossrun handle, const char* data)
{
  return crossrun_writedata(handle, data, strlen(data));
}

DLL_EXPORT_CROSSRUN void crossrun_write_eof (crossrun handle)
{
#ifdef _WIN32
  CloseHandle(handle->stdin_pipe[PIPE_WRITE]);
  handle->stdin_pipe[PIPE_WRITE] = NULL;
#else
  close(handle->stdin_pipe[PIPE_WRITE]);
  handle->stdin_pipe[PIPE_WRITE] = -1;
#endif
}



#ifdef ___EXPERIMENTAL___

typedef int (*crossrun_read_callback_fn)(const char* data, size_t datalen, void* callbackdata);

DLL_EXPORT_CROSSRUN int crossrun_read_write (crossrun handle, crossrun_read_callback_fn readfn, void* readcallbackdata, const char* writedata, size_t writedatalen);

DLL_EXPORT_CROSSRUN int crossrun_read_write (crossrun handle, crossrun_read_callback_fn readfn, void* readcallbackdata, const char* writedata, size_t writedatalen)
{
#ifdef _WIN32
#ifdef WITH_STDERR
  HANDLE handles[2];
#else
  HANDLE handles[3];
#endif
  DWORD i;
  DWORD result;
  DWORD handlescount;
  size_t writedatapos = 0;
  size_t writedataleft = writedatalen;
  //set handles to wait for
  handlescount = 0;
  if (writedataleft && handle->stdin_pipe[PIPE_WRITE])
    handles[handlescount++] = handle->stdin_pipe[PIPE_WRITE];
#ifdef WITH_STDERR
  if (handle->stderr_pipe[PIPE_READ] && handle->stderr_pipe[PIPE_READ] != handle->stdout_pipe[PIPE_READ])
    handles[handlescount++] = handle->stderr_pipe[PIPE_READ];
#endif
  if (handle->stdout_pipe[PIPE_READ])
    handles[handlescount++] = handle->stdout_pipe[PIPE_READ];
  //abort if no more open handles to wait for
  if (handlescount == 0)
    return -1;
  //loop
  while (writedataleft > 0 || writedatalen == 0) {
    //wait for handles
    if ((result = WaitForMultipleObjects(handlescount, handles, FALSE, INFINITE)) == WAIT_FAILED)
      return -1;
    if (result == WAIT_TIMEOUT)
      return 0;
    for (i = 0; i < handlescount; i++) {
      if (result - WAIT_ABANDONED_0 == i) {
        fprintf(stderr, "Handle abandoned\n");
        return -1;
      }
      if (result - WAIT_OBJECT_0 == i) {
        if (handles[i] == handle->stdout_pipe[PIPE_READ]) {
          char readbuf[1024];
          DWORD n;
          n = 0;
          if (!PeekNamedPipe(handle->stdout_pipe[PIPE_READ], NULL, 0, NULL, &n, NULL))
            return -1;
          if (n == 0)
            break;
          if (!ReadFile(handle->stdout_pipe[PIPE_READ], readbuf, (n <= sizeof(readbuf) ? n : sizeof(readbuf)), &n, NULL)) {
            fprintf(stderr, "ReadFile() failed after WaitForMultipleObjects()\n");
            /////TO DO: close handle
            return -1;
          }
          if (readfn)
            (*readfn)(readbuf, n, readcallbackdata);
          break;
        }
#ifdef WITH_STDERR
        if (handles[i] == handle->stderr_pipe[PIPE_READ]) {

          break;
        }
#endif
        if (handles[i] == handle->stdin_pipe[PIPE_WRITE]) {
          if (writedataleft) {
            DWORD n;
            if (!WriteFile(handle->stdin_pipe[PIPE_WRITE], writedata + writedatapos, writedataleft, &n, NULL)) {
            //if (!WriteFile(handle->stdin_pipe[PIPE_WRITE], writedata + writedatapos, (writedataleft > 1024 ? 1024 : writedataleft), &n, NULL)) {
              fprintf(stderr, "WriteFile() failed after WaitForMultipleObjects()\n");
              /////TO DO: close handle
              return -1;
            }
            writedatapos += n;
            if ((writedataleft -= n) == 0)
              return 0;
          }
          break;
        }
        break;
      }
    }
  }
#else
#endif
  return -1;
}

#endif // ___EXPERIMENTAL___

/////See also: https://docs.microsoft.com/en-us/windows/win32/ipc/synchronous-and-overlapped-input-and-output?redirectedfrom=MSDN


/////See also: https://docs.microsoft.com/en-us/windows/win32/ProcThread/creating-a-child-process-with-redirected-input-and-output
