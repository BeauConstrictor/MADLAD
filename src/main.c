#include <linux/limits.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dlfcn.h>
#include <stdio.h>

#include "constants.h"
#include "buffer.h"

typedef enum {
  NORMAL,
  INSERT,
  REPLACE,
} ed_mode;

typedef struct {
  void *lib;
  buf_highlighter fn;
} highlighter_lib;

typedef struct {
  buf_buffer buf;
  ed_mode mode;
  char status[128];
  unsigned int stat_col;
  highlighter_lib *highlighter;
} editor;

struct termios oldt, newt;
struct winsize w;

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

void use_highlighter(editor *ed, const char *filetype) {
  if (ed->highlighter) {
    dlclose(ed->highlighter->lib);
    free(ed->highlighter);
  } else {
    ed->highlighter = malloc(sizeof(highlighter_lib));
  }

  char so[PATH_MAX];
  snprintf(so, sizeof(so), "build/highlighters/%s.so", filetype);
  ed->highlighter->lib = dlopen(so, RTLD_LAZY);
  ed->highlighter->fn = dlsym(ed->highlighter->lib, "highlight");
}

void draw_editor(editor *ed) {
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  // move cursor to start and hide it
  printf("\033[H\033[?25l");

  int cur_row;
  int cur_col;

  buf_printall(&ed->buf, w.ws_row-1,
      ANSI(GREY) "%5d " RESET, ANSI(GREY) "~" RESET,
      &cur_row, &cur_col, ed->highlighter->fn);

  // it looks worse than it is, honest!
  // 1. bold
  // 2. use the current status color
  // 3. invert color
  // 4. move cursor to last line
  // 5. write the status (padded with spaces)
  printf("\033[1m\033[%um\033[7m\033[%d;1H%-*s\033[0m", ed->stat_col,
      w.ws_row, w.ws_col, ed->status);

  if (cur_row >= 0) {
    // 1. show the cursor
    // 2. move to cursor position in buffer
    // 7 is the width of the line numbers
    printf("\033[?25h\033[%d;%dH", cur_row, cur_col+7);
  }
}

void chmode(editor *ed, ed_mode mode) {
  ed->mode = mode;

  switch (mode) {
    case NORMAL:
      ed->stat_col = DEFAULT;
      ed->status[0] = '\0';
      printf("\033[2 q");
      break;
    case INSERT:
      buf_flush_changes(&ed->buf);
      ed->stat_col = YELLOW;
      snprintf(ed->status, sizeof(ed->status), "-- INSERT --");
      printf("\033[6 q");
      break;
    case REPLACE:
      ed->stat_col = YELLOW;
      snprintf(ed->status, sizeof(ed->status), "-- REPLACE --");
      printf("\033[4 q");
      break;
  }
}

void handle_key(editor *ed, char c) {
  switch (ed->mode) {
    case NORMAL: {
      switch (c) {
        case 'a':
          buf_cursor_r(&ed->buf, 1);
          [[fallthrough]];
        case 'i':
          chmode(ed, INSERT);
          break;
        case 'r':
          chmode(ed, REPLACE);
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

        case 'w':
        case 'b':
          do {
            if (c == 'w') buf_cursor_r(&ed->buf, 1);
            if (c == 'b') buf_cursor_l(&ed->buf, 1);
          } while (strchr(word_CHARS, buf_line_char(&ed->buf))
              && !buf_at_eol(&ed->buf) && !buf_at_sol(&ed->buf));
          break;

        case SHIFT_('w'):
        case SHIFT_('b'):
          do {
            if (c == 'W') buf_cursor_r(&ed->buf, 1);
            if (c == 'B') buf_cursor_l(&ed->buf, 1);
          } while (!strchr(WORD_DELIM, buf_line_char(&ed->buf))
              && !buf_at_eol(&ed->buf) && !buf_at_sol(&ed->buf));
          break;

        case 'u':
          if (!buf_undo(&ed->buf))
            snprintf(ed->status, sizeof(ed->status),
              "Nothing to undo.");
          break;

        case 'x':
          buf_cursor_r(&ed->buf, 1);
          buf_delete_c(&ed->buf, 1);
          break;

        case SHIFT_('k'):
          buf_scroll_u(&ed->buf, 1, true);
          break;
        case SHIFT_('j'):
          buf_scroll_d(&ed->buf, 1, true);
          break;
        case CTRL_('u'):
          buf_scroll_u(&ed->buf, w.ws_row/2, true);
          break;
        case CTRL_('d'):
          buf_scroll_d(&ed->buf, w.ws_row/2, true);
          break;
        case CTRL_('b'):
          buf_scroll_u(&ed->buf, w.ws_row, true);
          break;
        case CTRL_('f'): // <c-f>
          buf_scroll_d(&ed->buf, w.ws_row, true);
          break;

        case '{':
        case '}':
          do {
            if (c == '{') buf_cursor_u(&ed->buf, 1);
            if (c == '}') buf_cursor_d(&ed->buf, 1);
          } while (!buf_line_empty(&ed->buf)
              && !buf_at_sof(&ed->buf) && !buf_at_eof(&ed->buf));
          break;

        case 'q':
          exit(0);
          break;
      }
    } break;

    case INSERT: {
      switch (c) {
        case '\033':
          chmode(ed, NORMAL);
          break;
        case '\b':
        case '\177':
          buf_delete_c(&ed->buf, 1);
          break;
        default:
          buf_insert_c(&ed->buf, c);
      }
    } break;

    case REPLACE: {
      if (c != '\033') {
        buf_cursor_r(&ed->buf, 1);
        buf_delete_c(&ed->buf, 1);
        buf_insert_c(&ed->buf, c);
        buf_cursor_l(&ed->buf, 1);
      }
      chmode(ed, NORMAL);
    } break;
  }
}

int main(void) {
  editor ed = {0};

  snprintf(ed.status, sizeof(ed.status), "Welcome to MADLAD " VERSION "!");
  ed.stat_col = DEFAULT;
  use_highlighter(&ed, "c");

  FILE *f = fopen("src/main.c", "r");
  buf_insert_f(&ed.buf, f);
  fclose(f);
  buf_cursor_u(&ed.buf, UINT_MAX);

  initialise_terminal();

  while (1) {
    draw_editor(&ed);

    char key = getchar();
    handle_key(&ed, key);
  }
}
