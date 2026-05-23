#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "constants.h"
#include "cmds.h"
#include "ed.h"

struct termios oldt, newt;

void cleanup_terminal() {
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  // leave alternate screen
  printf("\033[?1049l");
}

void initialise_terminal() {
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  // enter alternate screen
  printf("\033[?1049h");

  atexit(cleanup_terminal);
}

void run_config_script(ed_editor *ed) {
  cmd_run(ed, "if [ -f ~/.madladrc ]; then"
               "    . ~/.madladrc;"
               "else"
               "   echo \"" WELCOME "\";"
               "fi");
}

int mainloop(ed_editor *ed) {
  while (1) {
    ed_draw(ed);

    char key = getchar();
    ed_handle_key(ed, key);
    if (ed->exit != -1) break;
  }

  buf_clear(&ed->buf);

  return ed->exit;
}

void repl(ed_editor *ed) {
  char cmd[1024];

  while (fgets(cmd, sizeof(cmd), stdin)) {
    if (strcmp(cmd, "q\n") == 0) break;
    cmd_run(ed, cmd);
    printf("%s\n", ed->status);
  }

  exit(0);
}

void process_args(ed_editor *ed, int argc, char **argv) {
  if (argc > 2) {
    fprintf(stderr, "madlad: too many arguments\n");
    fprintf(stderr, "Try 'madlad --help' for more information.\n");
    exit(1);
  } else if (argc == 2 &&
            (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
    printf(HELP_MSG);
    exit(0);
  } else if (argc == 2 && strcmp(argv[1], "--version") == 0) {
    printf("MADLAD (modal editor; line editor) " VERSION "\n");
    exit(0);
  } else if (argc == 2 && strcmp(argv[1], "--headless") == 0) {
    repl(ed);
  } else if (argc == 2) {
    char cmd[PATH_MAX];
    snprintf(cmd, sizeof(cmd), "edit \"%s\"", argv[1]);
    cmd_run(ed, cmd);
  }
}

int main(int argc, char **argv) {
  ed_editor ed = {0};

  ed.exit = -1;
  ed_chmode(&ed, NORMAL);
  ed_default_settings(&ed);
  snprintf(ed.status, sizeof(ed.status), WELCOME);

  cmd_init();
  atexit(cmd_finished);
  cmd_run(&ed, "eraseall");

  process_args(&ed, argc, argv);
  run_config_script(&ed);

  initialise_terminal();

  return mainloop(&ed);
}
