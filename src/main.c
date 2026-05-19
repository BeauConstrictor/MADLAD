#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "constants.h"
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

int main(void) {
  ed_editor ed = {0};

  ed.exit = -1;
  ed_chmode(&ed, NORMAL);
  ed_default_settings(&ed);
  snprintf(ed.status, sizeof(ed.status), WELCOME);

  FILE *f = fopen("src/main.c", "r");
  buf_insert_f(&ed.buf, f);
  fclose(f);
  buf_cursor_u(&ed.buf, UINT_MAX);

  initialise_terminal();

  while (1) {
    ed_draw(&ed);

    char key = getchar();
    ed_handle_key(&ed, key);
    if (ed.exit != -1) exit(ed.exit);
  }
}
