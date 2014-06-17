#ifndef __LLIB_ARG_H
#define __LLIB_ARG_H
#include "str.h"
#include "value.h"

#ifdef INSIDE_ARG_C
struct FlagEntry_;
#else
struct FlagEntry_ {
    str_t name, help, tname;
};
#endif
typedef struct FlagEntry_ FlagEntry;

typedef struct ArgState_ {
    SMap cmd_map;
    FlagEntry **cmds;
    bool has_commands;
    str_t error, usage;
} ArgState;

ArgState *arg_parse_spec(PValue *flagspec);
void arg_get_values(PValue *vals,...);
void arg_reset_used(ArgState *cmds);
PValue arg_bind_values(ArgState *cmds, SMap sm);
PValue arg_process(ArgState *cmds,  const char **argv);
void arg_quit(ArgState *cmds, str_t msg, bool show_help);
ArgState *arg_command_line(void**argspec, const char **argv);

#endif
