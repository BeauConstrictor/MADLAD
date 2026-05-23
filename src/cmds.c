#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

#include "constants.h"
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

char *res_buf = NULL;

char *install_dir;

char *temp_buf(char *s) {
  if (res_buf)
    free(res_buf);
  res_buf = strdup(s);
  return res_buf;
}

typedef enum {
  V_UNKNOWN,
  V_FILETYPE,
  V_HIGHLIGHTCOL,
  V_VERSION,

  V_BUF_PATH,
  V_BUF_LINES,
} cmds_var;

cmds_var read_var_name(const char *name) {
  #define CHECK_VAR(var)                    \
    if      (0 == strcmp(uppername, #var)) \
      return V_##var;

  size_t i;
  char uppername[64];
  for (i = 0; name[i] != '\0' && i < sizeof(uppername) - 1; i++) {
        uppername[i] = toupper((unsigned char)name[i]);
        if (name[i] == ':') uppername[i] = '_';
  }
  uppername[i] = '\0';

  CHECK_VAR(FILETYPE);
  CHECK_VAR(HIGHLIGHTCOL);
  CHECK_VAR(VERSION);
  CHECK_VAR(BUF_PATH);
  CHECK_VAR(BUF_LINES);

  return V_UNKNOWN;

  #undef CHECK_VAR
}

RPC_FUNC(quit) {
  long code = argc == 1 ? 0 : strtol(args[1], NULL, 10);
  ed->exit = code;
  SUCCESS();
}

RPC_FUNC(setv) {
  ENSURE_ENOUGH_ARGS(3);

  cmds_var var = read_var_name(args[1]);
  char *val = args[2];

  buf_buffer *buf = &ed->buf;

  switch (var) {
    case V_UNKNOWN: {
      RESPOND("undefined variable", 1);
    } break;

    case V_FILETYPE: {
      if (!ed_use_highlighter(ed, val))
        RESPOND("unsupported filetype", 1);
    } break;

    case V_HIGHLIGHTCOL: {
      char *end;
      long highlightcol = strtol(val, &end, 10);

      if (end == val)
        RESPOND("invalid number", 1);

      ed->settings.highlight_col = (int)highlightcol;
    } break;

    case V_BUF_PATH: {
      snprintf(buf->path, sizeof(buf->path), "%s", val);
    } break;

    default:
      RESPOND("variable is not settable", 1);
  }

  SUCCESS();
}

RPC_FUNC(getv) {
  ENSURE_ENOUGH_ARGS(2);

  cmds_var var = read_var_name(args[1]);

  buf_buffer *buf = &ed->buf;
  
  switch (var) {
    case V_UNKNOWN:
      RESPOND("undefined variable", 1);

    case V_FILETYPE:
      RESPOND(ed->settings.highlighter.name, 0);

    case V_HIGHLIGHTCOL: {
      char res[32];
      snprintf(res, sizeof(res), "%d", ed->settings.highlight_col);
      RESPOND(temp_buf(res), 0);
    }

    case V_VERSION:
      RESPOND(VERSION, 0);

    case V_BUF_PATH:
      RESPOND(buf->path, 0);
    case V_BUF_LINES: {
      char res[32];
      snprintf(res, sizeof(res), "%zu", buf->lines);
      RESPOND(temp_buf(res), 0);
    }

    default:
      RESPOND("variable is not gettable", 1);
  }
}

RPC_FUNC(insert) {
  ENSURE_ENOUGH_ARGS(2);

  buf_insert_s(&ed->buf, args[1]);

  SUCCESS();
}

RPC_FUNC(finsert) {
  ENSURE_ENOUGH_ARGS(2);

  FILE *f = fopen(args[1], "r");
  if (!f) RESPOND(strerror(errno), 1);
  buf_insert_f(&ed->buf, f);
  fclose(f);

  SUCCESS();
}

RPC_FUNC(fwrite) {
  FILE *f;
  if (argc == 1) {
    char *path = ed->buf.path;
    size_t len = strlen(path);
    if (len > 0) f = fopen(path, "w");
    else RESPOND("No file name", 1);
  } else {
    f = fopen(args[1], "w");
  }

  if (!f) RESPOND(strerror(errno), 1);
  buf_fwrite(&ed->buf, f);
  fclose(f);

  SUCCESS();
}

RPC_FUNC(eraseall) {
  (void)argc;
  (void)args;
  buf_clear(&ed->buf);

  // creates the initial line
  buf_insert_c(&ed->buf, ' ');
  buf_delete_c(&ed->buf, 1);

  SUCCESS();
}

// TODO: somehow reduce repetition in the cursor funcs?

RPC_FUNC(cursor_u) {
  ENSURE_ENOUGH_ARGS(2);

  char *end;
  long count = strtol(args[1], &end, 10);
  if (end == args[1])
    RESPOND("invalid number", 1);

  buf_cursor_u(&ed->buf, count);

  SUCCESS();
}

RPC_FUNC(cursor_d) {
  ENSURE_ENOUGH_ARGS(2);

  char *end;
  long count = strtol(args[1], &end, 10);
  if (end == args[1])
    RESPOND("invalid number", 1);

  buf_cursor_d(&ed->buf, count);

  SUCCESS();
}

RPC_FUNC(cursor_l) {
  ENSURE_ENOUGH_ARGS(2);

  char *end;
  long count = strtol(args[1], &end, 10);
  if (end == args[1])
    RESPOND("invalid number", 1);

  buf_cursor_l(&ed->buf, count);

  SUCCESS();
}

RPC_FUNC(cursor_r) {
  ENSURE_ENOUGH_ARGS(2);

  char *end;
  long count = strtol(args[1], &end, 10);
  if (end == args[1])
    RESPOND("invalid number", 1);

  buf_cursor_r(&ed->buf, count);

  SUCCESS();
}

typedef struct csrpc_resp (*rpc_cmd_handler)(ed_editor *ed,
  unsigned int argc, char **args);

struct rpc_cmd {
  char *name;
  rpc_cmd_handler handler;
};

#define ADD_FUNC(fn) { #fn, rpc_##fn }

static struct rpc_cmd cmds[] = {
  ADD_FUNC(quit),
  ADD_FUNC(setv),
  ADD_FUNC(getv),
  ADD_FUNC(insert),
  ADD_FUNC(finsert),
  ADD_FUNC(fwrite),
  ADD_FUNC(eraseall),
  ADD_FUNC(cursor_u),
  ADD_FUNC(cursor_d),
  ADD_FUNC(cursor_l),
  ADD_FUNC(cursor_r)
};

#undef ADD_FUNC

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

  char madlad_sh[4096];
  snprintf(madlad_sh, sizeof(madlad_sh), "%s/dsh/madlad.sh",
      install_dir);

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

void cmd_init() {
  install_dir = getenv("MADLAD_INSTALL");

  if (!install_dir) {
    fprintf(stderr, "madlad: MADLAD_INSTALL is not set\n"
      "Make sure to add this to your system's '~/.bashrc' equivalent:\n"
      "\texport MADLAD_INSTALL=~/.local/share/madlad/\n");
    exit(1);
  }


  setenv("IMPORT_MADLAD", ". \"$(command -v madlad.sh)\"", 1);
}

void cmd_finished() {
  free(res_buf);
}
