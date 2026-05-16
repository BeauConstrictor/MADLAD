#include <sys/ioctl.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "constants.h"
#include "buffer.h"

typedef enum {
  NORMAL,
  INSERT,
} ed_mode;

typedef struct {
  buf_buffer buf;
  ed_mode mode;
  char status[128];
} editor;

struct termios oldt, newt;

void cleanup_terminal() {
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  printf("\033[?1049l");
  printf("\033[?25h");
}

void initialise_terminal() {
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  printf("\033[?1049h");
  printf("\033[?25l");

  atexit(cleanup_terminal);
}

void draw_editor(editor *ed) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  printf("\033[H\033[2J");

  buf_printall(&ed->buf, w.ws_row-1, GREY "%5d " RESET, GREY "~" RESET);

  printf("%s", ed->status);
}

void chmode(editor *ed, ed_mode mode) {
  ed->mode = mode;

  switch (mode) {
    case NORMAL:
      ed->status[0] = '\0';
      break;
    case INSERT:
      snprintf(ed->status, sizeof(ed->status), BOLD YELLOW "-- INSERT --" RESET);
      break;
  }
}

void handle_key(editor *ed, char c) {
  if (ed->mode == NORMAL) {
    switch (c) {
      case 'i':
        chmode(ed, INSERT);
        break;
      case 'h':
        buf_cursor_l(&ed->buf, 1);
        break;
      case 'j':
        buf_cursor_d(&ed->buf, 1);
        break;
      case 'k':
        buf_cursor_u(&ed->buf, 1);
        break;
      case 'l':
        buf_cursor_r(&ed->buf, 1);
        break;
      case 'q':
        exit(0);
        break;
    }
  } else if (ed->mode == INSERT) {
    switch (c) {
      case '\033':
        chmode(ed, NORMAL);
        break;
      case '\b':
      case '\177':
        buf_backspace(&ed->buf);
        break;
      default:
        buf_insert_c(&ed->buf, c);
    }
  }
}

int main(void) {
  editor ed = {0};

  snprintf(ed.status, sizeof(ed.status), "Welcome to MADLAD " VERSION "!");

  buf_insert_s(&ed.buf, "Hello, world!\n");

  initialise_terminal();

  while (1) {
    draw_editor(&ed);

    char key = getchar();
    handle_key(&ed, key);
  }

  cleanup_terminal();

  return 0;
}
