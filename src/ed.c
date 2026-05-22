#include <linux/limits.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dlfcn.h>
#include <stdio.h>

#include "constants.h"
#include "buffer.h"
#include "cmds.h"

#include "ed.h"

struct winsize w;

bool ed_use_highlighter(ed_editor *ed, const char *filetype) {
  if (ed->settings.highlighter && ed->settings.highlighter->lib) {
    dlclose(ed->settings.highlighter->lib);
  } else if (!ed->settings.highlighter) {
    ed->settings.highlighter = malloc(sizeof(ed_highlighter));
  }

  snprintf(ed->settings.highlighter->name, sizeof(ed->settings.highlighter->name), "%s",
      filetype);

  if (strcmp(filetype, "plain") == 0) {
    ed->settings.highlighter->lib = NULL;
    ed->settings.highlighter->fn = NULL;
  } else {
    char so[PATH_MAX];
    snprintf(so, sizeof(so), "%s/highlighters/%s.so", getenv("MADLAD_INSTALL"), filetype);
    ed->settings.highlighter->lib = dlopen(so, RTLD_LAZY);
    if (ed->settings.highlighter->lib) {
      ed->settings.highlighter->fn = dlsym(ed->settings.highlighter->lib, "highlight");
    } else {
      ed->settings.highlighter->fn = NULL;
      return false;
    }
  }

  return true;
}

void ed_draw(ed_editor *ed) {
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  // move cursor to start and hide it
  printf("\033[H\033[?25l");

  int cur_row;
  int cur_col;

  buf_printall(&ed->buf, w.ws_row-1,
      ANSI(GREY) "%5d " RESET, ANSI(GREY) "~" RESET,
      &cur_row, &cur_col, ed->settings.highlighter->fn,
      ed->settings.highlight_col);

  printf("\033[K");

  if (ed->mode == COMMAND) {
    // clear the line and write status (beginning with ':')
    printf(":%.*s", w.ws_col-2, ed->status);
  } else {
    // it looks worse than it is, honest!
    // 1. bold
    // 2. use the current status color
    // 3. write the status (padded with spaces)
    // 4. move to cursor position in buffer
    //    (7 is the width of the line numbers)
    printf("\033[1m\033[%um%.*s\033[0m\033[%d;%dH", ed->stat_col,
        w.ws_col, ed->status, cur_row, cur_col+7);
  }

  // show the cursor
  printf("\033[?25h");
}

void ed_chmode(ed_editor *ed, ed_mode mode) {
  ed->mode = mode;

  switch (mode) {
    case NORMAL:
      ed->stat_col = DEFAULT;
      cmd_run(ed, BUF_STATUS_CMD);
      printf("\033[2 q");
      break;
    case INSERT:
      buf_flush_changes(&ed->buf);
      ed->stat_col = YELLOW;
      snprintf(ed->status, sizeof(ed->status), "-- INSERT --");
      printf("\033[6 q");
      break;
    case COMMAND:
      ed->stat_col = DEFAULT;
      ed->status[0] = '\0';
      break;
    case REPLACE:
      ed->stat_col = YELLOW;
      snprintf(ed->status, sizeof(ed->status), "-- REPLACE --");
      printf("\033[4 q");
      break;
  }
}

void ed_handle_key(ed_editor *ed, char c) {
  switch (ed->mode) {
    case NORMAL: {
      switch (c) {
        case 'a':
          buf_cursor_r(&ed->buf, 1);
          [[fallthrough]];
        case 'i':
          ed_chmode(ed, INSERT);
          break;
        case 'r':
          ed_chmode(ed, REPLACE);
          break;
        case ':':
          ed_chmode(ed, COMMAND);
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
        case 'd':
          buf_delete_l(&ed->buf, 1);
          break;

        case '$':
          buf_cursor_e(&ed->buf);
          break;
        case '0':
          buf_cursor_s(&ed->buf);
          break;
        case '^':
          buf_cursor_s(&ed->buf);
          while (isspace(buf_line_char(&ed->buf)) &&
              buf_cursor_x(&ed->buf) < buf_line_len(&ed->buf)) {
            buf_cursor_r(&ed->buf, 1);
          }
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
      }
    } break;

    case INSERT: {
      switch (c) {
        case '\033':
          ed_chmode(ed, NORMAL);
          break;
        case '\b':
        case '\177':
          buf_delete_c(&ed->buf, 1);
          break;
        default:
          buf_insert_c(&ed->buf, c);
      }
    } break;

    case COMMAND: {
      if (c == '\n') {
        char *cmd = strdup(ed->status);
        ed_chmode(ed, NORMAL);
        cmd_run(ed, cmd);
        free(cmd);
        return;
      }

      if (c == ESC) {
        ed_chmode(ed, NORMAL);
        return;
      }

      size_t len = strlen(ed->status);

      if (c == '\b' || c == '\177') {
        len--;
        ed->status[len] = '\0';
        return;
      }

      ed->status[len] = c;
      len++;
      ed->status[len] = '\0';
    } break;

    case REPLACE: {
      if (c != '\033') {
        buf_cursor_r(&ed->buf, 1);
        buf_delete_c(&ed->buf, 1);
        buf_insert_c(&ed->buf, c);
        buf_cursor_l(&ed->buf, 1);
      }
      ed_chmode(ed, NORMAL);
    } break;
  }
}

void ed_default_settings(ed_editor *ed) {
  ed_use_highlighter(ed, "plain");
  ed->settings.highlight_col = -1;
}
