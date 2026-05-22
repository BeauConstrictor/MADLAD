#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "buffer.h"

typedef struct {
  const char *bef_cur;
  size_t bef_cur_l;
  const char *aft_cur;
  size_t aft_cur_l;
  size_t all_l;
} buf_editing_info;

static void buf_get_editing_info(buf_editing_info *i, const buf_editing *e) {
  i->bef_cur   = e->text;
  i->bef_cur_l = e->cursor;
  i->aft_cur   = e->text + e->aftergap;
  i->aft_cur_l = sizeof(e->text) - e->aftergap;
  i->all_l = i->bef_cur_l + i->aft_cur_l;
}

static buf_line *buf_create_line(const char *s, buf_line *prev, buf_line *next) {
  buf_line *l = malloc(sizeof(buf_line));
  l->prev = prev;
  l->next = next;

  l->len = snprintf(l->text, sizeof(l->text), "%s", s);
  if (l->len >= sizeof(l->text))
    l->len = sizeof(l->text) - 1;

  return l;
}

static size_t buf_stringify_editable(char *s, size_t size, buf_editing *e) {
  buf_editing_info i;
  buf_get_editing_info(&i, e);

  // size should be big enough to hold the string
  assert(size >= i.all_l + 1);

  memcpy(s, i.bef_cur, i.bef_cur_l);
  memcpy(s+i.bef_cur_l, i.aft_cur, i.aft_cur_l);
  s[i.all_l] = '\0';

  return i.all_l;
}

static void buf_update_line(buf_line *l, buf_editing *e) {
  l->len = buf_stringify_editable(l->text, sizeof(l->text), e);
}

static buf_editing *buf_start_editing(buf_line *l, unsigned int cur) {
  if (cur > l->len) cur = l->len;

  buf_editing *e = malloc(sizeof(buf_editing));
  e->cursor = cur;
  e->aftergap = sizeof(e->text) - (l->len - cur);
  e->original = l;
  memcpy(e->text, l->text, cur);
  memcpy(e->text + e->aftergap, l->text+cur, l->len - cur);

  return e;
}

static void buf_editable_cur_l(buf_editing *e) {
  if (e->aftergap > sizeof(e->text)) return;
  if (e->cursor == 0) return;
  e->cursor--;
  char c = e->text[e->cursor];
  e->aftergap--;
  e->text[e->aftergap] = c;
}

static void buf_editable_cur_r(buf_editing *e) {
  if (e->aftergap >= sizeof(e->text)) return;
  char c = e->text[e->aftergap];
  e->text[e->cursor] = c;
  e->cursor++;
  e->aftergap++;
}

void buf_flush_changes(buf_buffer *buf) {
  if (!buf->edits) return;
  buf_update_line(buf->cur_l, buf->edits);
  buf->old_cursor = buf->edits->cursor;
  free(buf->edits);
  buf->edits = NULL;
}

void buf_insert_l(buf_buffer *buf) {
  buf_line *new_line;
  buf->old_cursor = 0;

  if (buf->first_l) {
    buf_flush_changes(buf);

    buf_line *prev_cur = buf->cur_l;
    new_line = buf_create_line("", prev_cur, prev_cur->next);
    prev_cur->next = new_line;
    new_line->prev = prev_cur;
    if (new_line->next)
      new_line->next->prev = new_line;

    if (prev_cur == buf->last_l)
      buf->last_l = new_line;
  } else {
    new_line = buf_create_line("", NULL, NULL);
    buf->first_l = new_line;
    buf->last_l = new_line;
    buf->scrolled_l = new_line;
  }

  new_line->len = 0;

  buf->cur_l = new_line;
  buf->lines++;
}

bool buf_at_sof(buf_buffer *buf) {
  return buf->cur_l == buf->first_l;
}

bool buf_at_sol(buf_buffer *buf) {
  return buf_cursor_x(buf) == 0;
}

bool buf_at_eof(buf_buffer *buf) {
  return buf->cur_l == buf->last_l;
}

bool buf_at_eol(buf_buffer *buf) {
  return buf_cursor_x(buf) == buf_line_len(buf);
}

bool buf_at_last_c(buf_buffer *buf) {
  return buf_cursor_x(buf) >= buf_line_len(buf) - 1;
}

bool buf_line_empty(buf_buffer *buf) {
  return buf_line_text(buf)[0] == '\0';
}

void buf_cursor_u(buf_buffer *buf, unsigned int n) {
  buf_flush_changes(buf);
  while (n) {
    if (buf->cur_l->prev) {
      if (buf->scrolled_l == buf->cur_l)
        buf->scrolled_l = buf->cur_l->prev;
      buf->cur_l = buf->cur_l->prev;
    } else break;
    n--;
  }
}

void buf_cursor_d(buf_buffer *buf, unsigned int n) {
  buf_flush_changes(buf);
  while (n) {
    if (buf->cur_l->next) {
      buf->cur_l = buf->cur_l->next;
    } else break;
    n--;
  }
}

void buf_cursor_l(buf_buffer *buf, unsigned int n) {
  while (n) {
    if (buf->edits) {
      buf_editable_cur_l(buf->edits);
    } else if (buf->cur_l->len == 0)
      buf->old_cursor = 0;
    else if (buf->old_cursor > buf->cur_l->len)
      buf->old_cursor = buf->cur_l->len - 1;
    else if (buf->old_cursor > 0)
      buf->old_cursor--;
    n--;
  }
}

void buf_cursor_r(buf_buffer *buf, unsigned int n) {
  while (n) {
    if (buf->edits) {
      buf_editable_cur_r(buf->edits);
    } else if (buf->cur_l->len == 0)
      buf->old_cursor = 0;
    else if (buf->old_cursor < buf->cur_l->len)
      buf->old_cursor++;
    n--;
  }
}

size_t buf_cursor_x(buf_buffer *buf) {
  if (buf->edits)
    return buf->edits->cursor;

  size_t cur = buf->old_cursor;
  if (cur > buf->cur_l->len) cur = buf->cur_l->len;
  return cur;
}

size_t buf_line_len(buf_buffer *buf) {
  if (buf->edits) {
    buf_editing_info i;
    buf_get_editing_info(&i, buf->edits);
    return i.all_l;
  } else
    return buf->cur_l->len;
}

static void buf_begin_editing(buf_buffer *buf) {
  if (buf->edits) return;
  buf->edits = buf_start_editing(buf->cur_l, buf->old_cursor);
  buf->old_cursor = 0;
}

void buf_cursor_s(buf_buffer *buf) {
  if (!buf->edits) {
    buf->old_cursor = 0;
    return;
  }

  buf_editing *e = buf->edits;
  buf_editing_info i;
  buf_get_editing_info(&i, e);

  char text[LINE_WIDTH];
  size_t len = snprintf(text, sizeof(text), "%.*s%s",
      (int)i.bef_cur_l, i.bef_cur, i.aft_cur);

  e->cursor = 0;
  e->aftergap = sizeof(e->text) - len;
  memcpy(e->text+e->aftergap, text, len);
}

void buf_cursor_e(buf_buffer *buf) {
  buf_cursor_r(buf, buf_line_len(buf));
}

void buf_scroll_u(buf_buffer *buf, unsigned int n, bool move_cur) {
  while (n && buf->scrolled_l->prev) {
    if (buf->cur_l->prev && move_cur) buf->cur_l = buf->cur_l->prev;
    buf->scrolled_l = buf->scrolled_l->prev;
    buf->scrolled_lno--;
    n--;
  }
}

void buf_scroll_d(buf_buffer *buf, unsigned int n, bool move_cur) {
  while (n && buf->scrolled_l->next) {
    if (buf->cur_l->next && move_cur) buf->cur_l = buf->cur_l->next;
    buf->scrolled_l = buf->scrolled_l->next;
    buf->scrolled_lno++;
    n--;
  }
}

void buf_replace_l(buf_buffer *buf, const char *s) {
  buf_flush_changes(buf);
  buf_line *l = buf->cur_l;
  size_t n = snprintf(l->text, sizeof(l->text), "%s", s);
  if (n > sizeof(l->text)) n = sizeof(l->text) - 1;
  l->len = n;
}

void buf_insert_c(buf_buffer *buf, char c) {
  if (!buf->first_l)
    buf_insert_l(buf);

  if (c == '\n') {
    buf_flush_changes(buf);

    char after_cur[LINE_WIDTH];
    snprintf(after_cur, sizeof(after_cur), "%s", buf->cur_l->text+buf->old_cursor);

    buf->cur_l->text[buf->old_cursor] = '\0';
    buf->cur_l->len -= buf->cur_l->len - buf->old_cursor;
    buf_insert_l(buf);
    buf_replace_l(buf, after_cur);
    buf_cursor_s(buf);
  } else {
    buf_begin_editing(buf);

    // the editable must have some space
    assert(buf->edits->cursor != buf->edits->aftergap);

    buf->edits->text[buf->edits->cursor] = c;
    buf->edits->cursor++;
  }
}

void buf_insert_s(buf_buffer *buf, const char *s) {
  while (*s) {
    buf_insert_c(buf, *s);
    s++;
  }
}

void buf_insert_f(buf_buffer *buf, FILE *f) {
  int c;
  while ((c = fgetc(f)) != EOF)
    buf_insert_c(buf, (char)c);
}

void buf_delete_c(buf_buffer *buf, unsigned int n) {
  buf_begin_editing(buf);

  while (n) {
    if (buf->edits->cursor == 0) {
      buf_delete_l(buf, 1);
      buf_cursor_e(buf);
    } else {
      buf->edits->cursor--;
    }
    n--;
  }
}

void buf_delete_l(buf_buffer *buf, unsigned int n) {
  buf_flush_changes(buf);

  while (n) {
    if (buf->lines == 1) {
      buf_replace_l(buf, "");
      break;
    }

    buf_line *l = buf->cur_l;
    buf->cur_l = l->next ? l->next : l->prev;

    if (buf->scrolled_l == l)
      buf->scrolled_l = l->next ? l->next: l->prev;

    if (!buf->cur_l) {
      buf->first_l = NULL;
      buf->last_l  = NULL;
    } else {
      if (buf->first_l == l) buf->first_l = l->next;
      if (buf->last_l  == l) buf->last_l = l->prev;
    }
    
    if (l->next) l->next->prev = l->prev;
    if (l->prev) l->prev->next = l->next;
    free(l);

    buf->lines--;
    n--;
  }
}

bool buf_undo(buf_buffer *buf) {
  bool anything_to_undo = buf->edits != NULL;
  if (anything_to_undo) {
    buf->old_cursor = buf->edits->cursor;
    free(buf->edits);
    buf->edits = NULL;
  }
  return anything_to_undo;
}

// set height to 0 to stop drawing immediately at end of buffer
void buf_printall(buf_buffer *buf, unsigned int height,
    const char *linenums, const char *eoflines,
    int *cur_row, int *cur_col, buf_highlighter highlighter,
    int highlight_col) {
  buf_line *l = buf->scrolled_l;
  unsigned int lineno = buf->scrolled_lno;
  unsigned int printed_lines = 0;

  char unhighlighted[sizeof(((buf_line *)0)->text)];
  char highlighted[4096];

  if (cur_row) *cur_row = -1;
  if (cur_col) *cur_col = -1;

  while (l && printed_lines < height) {
    char s[LINE_WIDTH];
    if (buf->edits && l == buf->cur_l) {
      buf_stringify_editable(s, sizeof(s), buf->edits);
    } else {
      snprintf(s, sizeof(s), "%s", l->text);
    }

    printf("\033[2K");

    printf(linenums, lineno+1);

    if (l == buf->cur_l) {
      if (cur_row) *cur_row = printed_lines + 1;
      if (cur_col) {
        *cur_col = buf->edits ? buf->edits->cursor : buf->old_cursor;
        if (*cur_col > (int)buf_line_len(buf))
          *cur_col = buf_line_len(buf);
      }
    }

    if (highlight_col < 0) {
      if (highlighter) {
        highlighter(highlighted, sizeof(highlighted), s);
        printf("%s\n", highlighted);
      } else {
        printf("%s\n", s);
      }
    } else {
      if (highlighter) {
        snprintf(unhighlighted, sizeof(unhighlighted), "%-*.*s",
            highlight_col, highlight_col, s);

        highlighter(highlighted, sizeof(highlighted), unhighlighted);
        printf("%s", highlighted);
      } else {
        printf("%-*.*s", highlight_col, highlight_col, s);
      }

      // 1. invert colours
      // 2. print the char directly on the highlight_col (or space)
      // 3. print any chars after the highlight, in grey
      size_t len = strlen(s);
      printf("\033[90m\033[1m\033[7m%c\033[0m\033[90m%s\033[0m\n",
         (int)len > highlight_col    ? s[highlight_col]    : ' ',
         (int)len >  highlight_col+1 ? s + highlight_col+1 : ""
      );
    }

    l = l->next;
    lineno++;
    printed_lines++;
  }

  while (printed_lines < height) {
    printf("\033[2K");
    printf(eoflines, lineno);
    putchar('\n');
    lineno++;
    printed_lines++;
  }

  // this can only happen if they moved the cursor down until it
  // went offscreen. a bit of a hacky solution, but we can fix
  // this from inside the printing function.
  if (*cur_row == -1) {
    unsigned int lines_below = 0;
    buf_line *l = buf->scrolled_l;
    while (l && l != buf->cur_l) {
      l = l->next;
      lines_below++;
    }
    lines_below -= height - 1;
    buf_scroll_d(buf, lines_below, false);
    buf_printall(buf, height, linenums, eoflines, cur_row, cur_col,
        highlighter, highlight_col);
  }
}

void buf_clear(buf_buffer *buf) {
  buf_line *l = buf->first_l;
  while (l) {
    buf_line *next = l->next;
    free(l);
    l = next;
  }

  if (buf->edits)
    free(buf->edits);

  memset(buf, 0x00, sizeof(buf_buffer));
}

const char *buf_line_text(buf_buffer *buf) {
  static char text[LINE_WIDTH];

  if (!buf->edits) return buf->cur_l->text;

  buf_stringify_editable(text, sizeof(text), buf->edits);
  return text;
}

char buf_line_char(buf_buffer *buf) {
  size_t len = buf_line_len(buf);
  unsigned int cur = buf_cursor_x(buf);

  if (cur == len) return '\0';

  if (buf->edits)
    return buf->edits->text[cur];
  else
    return buf_line_text(buf)[cur];
}

void buf_fwrite(buf_buffer *buf, FILE *f) {
  buf_line *l = buf->first_l;

  if (!l) return;

  buf_flush_changes(buf);

  while (l) {
    fwrite(l->text, 1, l->len, f);
    fputc('\n', f);
    l = l->next;
  }
}
