
//
// commander.h
//
// Copyright (c) 2012 TJ Holowaychuk <tj@vision-media.ca>
//

#ifndef COMMANDER_H
#define COMMANDER_H

/*
 * Max options that can be defined.
 */

#ifndef COMMANDER_MAX_OPTIONS
#define COMMANDER_MAX_OPTIONS 32
#endif

/*
 * Max arguments that can be passed.
 */

#ifndef COMMANDER_MAX_ARGS
#define COMMANDER_MAX_ARGS 32
#endif

#include "cstring.h"
#include "chash.h"
#include "cvector.h"

/*
 * Command option.
 */

typedef struct {
  int optional_arg;
  int required_arg;
  char *argname;
  char *large;
  const char *small;
  const char *large_with_arg;
  const char *description;
} cli_cmd_opt_t;

/*
 * Command.
 */
typedef struct cli_cmd {
  void *data;
  const char *usage;
  const char *arg;
  const char *name;
  const char *version;
  int option_count;
  cli_cmd_opt_t options[COMMANDER_MAX_OPTIONS];
  int argc;
  char *argv[COMMANDER_MAX_ARGS];
  char **nargv;
  chash     *opts;
  cvector   *args;
} cli_cmd_t;

// prototypes

void command_init(cli_cmd_t *self, const char *name, const char *version);
void command_free(cli_cmd_t *self);
void command_help(cli_cmd_t *self);
void command_option(cli_cmd_t *self, const char *small, const char *large, const char *desc);
void command_parse(cli_cmd_t *self, int argc, char **argv);

#endif /* COMMANDER_H */
