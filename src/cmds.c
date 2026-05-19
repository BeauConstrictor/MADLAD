#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "csrpc.h"
#include "ed.h"

#include "cmds.h"

#define RPC_FUNC(name) static struct csrpc_resp rpc_##name(ed_editor *ed, \
    unsigned int argc, char **args)

#define ENSURE_ENOUGH_ARGS(count)                             \
  if (argc < count)                                           \
      return (struct csrpc_resp){"missing argument(s)\n", 1};

#define RESPOND(msg, status) return (struct csrpc_resp){(msg), (status)};
#define SUCCESS() RESPOND("", 0);

#define ADD_FUNC(name) { #name, rpc_##name }

char *res_buf = NULL;

char *temp_buf(char *s) {
  if (res_buf)
    free(res_buf);
  res_buf = strdup(s);
  return res_buf;
}

RPC_FUNC(quit) {
  long code = argc == 1 ? 0 : strtol(args[1], NULL, 10);
  ed->exit = code;
  SUCCESS();
}

RPC_FUNC(setv) {
  ENSURE_ENOUGH_ARGS(3);

  char *var = args[1];
  char *val = args[2];

  // buf_buffer *buf = &ed->buf;

  if        (0 == strcmp(var, "filetype")) {
    if (!ed_use_highlighter(ed, val))
      RESPOND("unsupported filetype", 1);
  } else if (0 == strcmp(var, "highlightcol")) {
    char *end;
    long highlightcol = strtol(val, &end, 10);

    if (end == val)
      RESPOND("invalid number", 1);

    ed->settings.highlight_col = (int)highlightcol;
  }

  else {
    RESPOND("undefined setting", 1);
  }

  SUCCESS();
}

RPC_FUNC(getv) {
  ENSURE_ENOUGH_ARGS(2);

  char *var = args[1];

  // buf_buffer *buf = &ed->buf;

  if        (0 == strcmp(var, "filetype")) {
    RESPOND(ed->settings.highlighter->name, 0);
  } else if (0 == strcmp(var, "highlightcol")) {
    char res[32];
    snprintf(res, sizeof(res), "%d", ed->settings.highlight_col);
    RESPOND(temp_buf(res), 0);
  }

  else {
    RESPOND("undefined setting", 1);
  }
}

typedef struct csrpc_resp (*rpc_cmd_handler)(ed_editor *ed,
  unsigned int argc, char **args);

struct rpc_cmd {
  char *name;
  rpc_cmd_handler handler;
};

static struct rpc_cmd cmds[] = {
  { "quit",                   rpc_quit },
  { "setv",                   rpc_setv },
  { "getv",                   rpc_getv },
};

static const size_t cmd_count = sizeof(cmds) / sizeof(struct rpc_cmd);

static struct csrpc_resp handle_rpc_call(struct csrpc_call *call, void *ed) {
  ed = (ed_editor*)ed;
  unsigned int argc = call->argc;
  char **args = call->args;
  char *cmd = call->args[0];

  for (unsigned int i = 0; i < cmd_count; i++) {
    if (strcmp(cmd, cmds[i].name) == 0) {
      return cmds[i].handler(ed, argc, args);
    }
  }

  return (struct csrpc_resp){"csrpc: command not found\n", 1};
}

void cmd_run(ed_editor *ed, char *cmd) {
  ed->status[0] = '\0';

  char madlad_sh[] = "build/dsh/madlad.sh";

  FILE *f = csrpc_run(cmd, madlad_sh, handle_rpc_call, ed);

  size_t n = fread(ed->status, 1, sizeof(ed->status)-1, f);
  ed->status[n] = '\0';
  fclose(f);

  // replace newlines with spaces
  char *s = ed->status;
  while (*s) {
    if (*s == '\n') *s = ' ';
    s++;
  }

  if (res_buf)
    free(res_buf);
  res_buf = NULL;
}
