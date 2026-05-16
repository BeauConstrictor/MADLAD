#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "constants.h"
#include "buffer.h"

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
  char *bef_cur    = e->text;
  size_t bef_cur_l = e->cursor;
  char *aft_cur    = e->text + e->aftergap;
  size_t aft_cur_l = sizeof(e->text) - e->aftergap;
  size_t total_len = bef_cur_l + aft_cur_l;

  // size should be big enough to hold the string
  assert(size >= total_len + 1);

  memcpy(s, bef_cur, bef_cur_l);
  memcpy(s+bef_cur_l, aft_cur, aft_cur_l);
  s[total_len] = '\0';

  return total_len;
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
  if (e->cursor <= 0) return;
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

  buf->cur_l = new_line;
  buf->lines++;
}

void buf_cursor_u(buf_buffer *buf, unsigned int n) {
  buf_flush_changes(buf);
  while (n) {
    if (buf->cur_l->prev)
      buf->cur_l = buf->cur_l->prev;
    else
      break;
    n--;
  }
}

void buf_cursor_d(buf_buffer *buf, unsigned int n) {
  buf_flush_changes(buf);
  while (n) {
    if (buf->cur_l->next)
      buf->cur_l = buf->cur_l->next;
    else
      break;
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
    else if (buf->old_cursor <= buf->cur_l->len)
      buf->old_cursor++;
    n--;
  }
}

void buf_scroll_u(buf_buffer *buf, unsigned int n) {
  while (n && buf->scrolled_l->prev) {
    buf->scrolled_l = buf->scrolled_l->prev;
    n--;
  }
}

void buf_scroll_d(buf_buffer *buf, unsigned int n) {
  while (n && buf->scrolled_l->next) {
    buf->scrolled_l = buf->scrolled_l->next;
    n--;
  }
}

static void buf_begin_editing(buf_buffer *buf) {
  if (buf->edits) return;
  buf->edits = buf_start_editing(buf->cur_l, buf->old_cursor);
  buf->old_cursor = 0;
}

void buf_insert_c(buf_buffer *buf, char c) {
  if (!buf->first_l || c == '\n')
    buf_insert_l(buf);
  if (c == '\n')
    return;

  buf_begin_editing(buf);

  // the editable must have some space
  assert(buf->edits->cursor != buf->edits->aftergap);

  buf->edits->text[buf->edits->cursor] = c;
  buf->edits->cursor++;
}

void buf_insert_s(buf_buffer *buf, const char *s) {
  while (*s) {
    buf_insert_c(buf, *s);
    s++;
  }
}

void buf_backspace(buf_buffer *buf) {
  buf_begin_editing(buf);

  if (buf->edits->cursor > 0)
    buf->edits->cursor--;
}

// undo doesn't undo everything, and there is no way to redo, so it is
// probably best to implement undo history separately in your editor.
void buf_undo(buf_buffer *buf) {
  buf->old_cursor = buf->edits->cursor;
  free(buf->edits);
  buf->edits = NULL;
}

// set height to 0 to stop drawing immediately at end of buffer
void buf_printall(buf_buffer *buf, unsigned int height,
    const char *linenums, const char *eoflines) {
  buf_line *l = buf->first_l;
  unsigned int lineno = 0;
  unsigned int printed_lines = 0;
  bool above_scroll = true;

  while (l) {
    if (above_scroll) {
      if (l == buf->scrolled_l) {
        above_scroll = false;
      } else {
        lineno++;
        l = l->next;
      }
      continue;
    }

    char s[LINE_WIDTH]; size_t len;
    if (buf->edits && l == buf->cur_l) {
      len = buf_stringify_editable(s, sizeof(s), buf->edits);
    } else {
      snprintf(s, sizeof(s), "%s", l->text);
      len = l->len < sizeof(s) - 1 ? l->len : sizeof(s) - 1;
    }

    printf(linenums, lineno+1);

    if (l == buf->cur_l) {
      int cur = buf->edits ? buf->edits->cursor : buf->old_cursor;
      if (cur >= (int)len)
        printf("%s\033[7m \033[0m\n", s);
      else
        printf("%.*s\033[7m%c\033[0m%s\n", cur, s, s[cur], s+cur+1);
    } else {
      printf("%s\n", s);
    }

    l = l->next;
    lineno++;
    printed_lines++;
  }

  while (printed_lines < height) {
    printf(eoflines, lineno);
    putchar('\n');
    lineno++;
    printed_lines++;
  }
}

void free_buf(buf_buffer *buf, bool free_buf_itself) {
  buf_line *l = buf->first_l;
  while (l) {
    buf_line *next = l->next;
    free(l);
    l = next;
  }

  if (buf->edits)
    free(buf->edits);

  if (free_buf_itself)
    free(buf);
}
