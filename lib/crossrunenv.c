#include "crossrunenv.h"
#ifdef _WIN32
#include <windows.h>
#else
#define _GNU_SOURCE
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct crossrunenv_data {
  char* variable;
  char* value;
  struct crossrunenv_data* next;
};

crossrunenv crossrunenv_create_empty ()
{
  return NULL;
}

crossrunenv crossrunenv_create_from_system ()
{
  struct crossrunenv_data* result = NULL;
#ifdef _WIN32
  char* sysenv;
  //get the system environment and build an environment chain
  if ((sysenv = GetEnvironmentStringsA()) != NULL) {
    struct crossrunenv_data** current = &result;
    const char* var = sysenv;
    size_t varlen;
    const char* pos;
    while ((varlen = strlen(var)) > 0) {
      if ((pos = strchr(var, '=')) != NULL) {
        size_t varnamelen = pos - var;
        *current = (struct crossrunenv_data*)malloc(sizeof(struct crossrunenv_data));
        (*current)->variable = (char*)malloc(varnamelen + 1);
        memcpy((*current)->variable, var, varnamelen);
        (*current)->variable[varnamelen] = 0;
        (*current)->value = strdup(var + varnamelen + 1);
        (*current)->next = NULL;
        current = &((*current)->next);
      }
      var += varlen + 1;
    }
    FreeEnvironmentStringsA(sysenv);
  }
#else
  const char** sysenv;
  if ((sysenv = (const char**)environ) != NULL) {
    struct crossrunenv_data** current = &result;
    size_t varlen;
    const char* pos;
    while (*sysenv) {
      if ((pos = strchr(*sysenv, '=')) != NULL) {
        size_t varnamelen = pos - *sysenv;
        *current = (struct crossrunenv_data*)malloc(sizeof(struct crossrunenv_data));
        (*current)->variable = (char*)malloc(varnamelen + 1);
        memcpy((*current)->variable, *sysenv, varnamelen);
        (*current)->variable[varnamelen] = 0;
        (*current)->value = strdup(*sysenv + varnamelen + 1);
        (*current)->next = NULL;
        current = &((*current)->next);
      }
      sysenv++;
    }
  }
#endif
  return result;
}

void crossrunenv_free (crossrunenv environment)
{
  struct crossrunenv_data* next;
  //free the environment chain
  while (environment) {
    next = environment->next;
    free(environment->variable);
    free(environment->value);
    free(environment);
    environment = next;
  }
}

void crossrunenv_set (crossrunenv* environment, const char* variable, const char* value)
{
  struct crossrunenv_data** current = environment;
  //find insert position
  while (*current && strcasecmp(variable, (*current)->variable) > 0) {
    current = &((*current)->next);
  }
  //check if there is an exact match
  if (*current && strcasecmp(variable, (*current)->variable) == 0) {
    //overwrite if there is an exact match
    free((*current)->value);
    //if value is NULL remove the entry and finish
    if (value == NULL) {
      struct crossrunenv_data* entry = *current;
      *current = (*current)->next;
      free(entry->variable);
      free(entry);
      return;
    }
    //overwrite variable name (in case of different case)
    strcpy((*current)->variable, variable);
  } else {
    //insert the new variable
    if (value == NULL)
      return;
    struct crossrunenv_data* entry = (struct crossrunenv_data*)malloc(sizeof(struct crossrunenv_data));
    entry->variable = strdup(variable);
    entry->next = (*current);
    *current = entry;
  }
  //set the value
  (*current)->value = strdup(value);
}

const char* crossrunenv_get (crossrunenv environment, const char* variable)
{
  struct crossrunenv_data* current = environment;
  while (current) {
    if (strcasecmp(variable, current->variable) == 0)
      return current->value;
    current = current->next;
  }
  return NULL;
}

#ifdef _WIN32
char* crossrunenv_generate (crossrunenv environment)
#else
char** crossrunenv_generate (crossrunenv environment)
#endif
{
#ifdef _WIN32
  char* result;
  struct crossrunenv_data* current;
  char* p;
  size_t len;
  size_t resultlen = 1;
  //abort if list of variables is empty
  if (!environment)
    return NULL;
  //determine data size
  current = environment;
  while (current) {
    resultlen += strlen(current->variable) + strlen(current->value) + 2;
    current = current->next;
  }
  //allocate memory
  result = (char*)malloc(resultlen);
  //copy data
  p = result;
  current = environment;
  while (current) {
    len = strlen(current->variable);
    memcpy(p, current->variable, len);
    p += len;
    *p++ = '=';
    len = strlen(current->value);
    memcpy(p, current->value, len);
    p += len;
    *p++ = 0;
    current = current->next;
  }
  *p = 0;
#else
  char** result;
  struct crossrunenv_data* current;
  char** p;
  char* q;
  size_t len = 0;
  size_t resultlen = sizeof(char*);
  //determine data size
  current = environment;
  while (current) {
    len++;
    resultlen += sizeof(char*) + strlen(current->variable) + strlen(current->value) + 2;
    current = current->next;
  }
  //allocate memory
  result = (char**)malloc(resultlen);
  p = result;
  q = (char*)result + (len + 1) * sizeof(char*);
  current = environment;
  while (current) {
    *p++ = q;
    len = strlen(current->variable);
    memcpy(q, current->variable, len);
    q += len;
    *q++ = '=';
    len = strlen(current->value);
    memcpy(q, current->value, len);
    q += len;
    *q++ = 0;
    current = current->next;
  }
  *p = NULL;
#endif
  return result;
}

#ifdef _WIN32
DLL_EXPORT_CROSSRUN void crossrunenv_free_generated (char* env)
#else
DLL_EXPORT_CROSSRUN void crossrunenv_free_generated (char** env)
#endif
{
  if (env)
    free(env);
}

DLL_EXPORT_CROSSRUN int crossrunenv_iterate (crossrunenv environment, crossrunenv_process_fn callback, void* callbackdata)
{
  int result = 0;
  struct crossrunenv_data* current = environment;
  while (current) {
    if ((result = (*callback)(current->variable, current->value, callbackdata)) != 0)
      break;
    current = current->next;
  }
  return result;
}
