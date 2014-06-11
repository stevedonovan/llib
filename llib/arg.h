#ifndef __LLIB_ARG_H
#define __LLIB_ARG_H
#include "str.h"
#include "value.h"

typedef struct ArgState_ {
    SMap cmd_map;
    void *cmds;
    bool has_commands;
    const char *error;
} ArgState;

ArgState *arg_parse_spec(PValue *flagspec);
void arg_functions_as_commands(ArgState *cmds);
void arg_get_values(PValue *vals,...);
PValue arg_process(ArgState *cmds,  const char **argv);
ArgState *arg_command_line(PValue *argspec, const char **argv);

#endif
