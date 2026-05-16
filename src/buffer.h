#ifndef BUFFER_H
#define BUFFER_H

// ERRORS:
// for now, many errors are caught by asserts with comments above
// them explaining their meaning. this will be changed in future,
// likely to some kind of errno system.

typedef struct buf_line buf_line;
struct buf_line {
  buf_line *prev;
  char text[LINE_WIDTH]; // text should be null-terminated but still
                         // has length for convenience
  size_t len;
  buf_line *next;
};

typedef struct {
  buf_line *original; // original line before edits were made
  size_t cursor;      // index of the start of the gap
  size_t aftergap;    // index of the first char after the gap
  char text[LINE_WIDTH-1];
} buf_editing;

typedef struct {
  buf_line *first_l;
  buf_line *last_l;
  buf_line *scrolled_l; // line at the top of the screen
  buf_line *cur_l;
  buf_editing *edits;   // the edits of cur_l; NULL if none)
  size_t lines;         // total number of lines in buffer
  size_t old_cursor;    // cursor position, from when there was an editable
} buf_buffer;

void buf_flush_changes(buf_buffer *buf);

void buf_cursor_u(buf_buffer *buf, unsigned int n);
void buf_cursor_d(buf_buffer *buf, unsigned int n);
void buf_cursor_l(buf_buffer *buf, unsigned int n);
void buf_cursor_r(buf_buffer *buf, unsigned int n);

void buf_scroll_u(buf_buffer *buf, unsigned int n);
void buf_scroll_d(buf_buffer *buf, unsigned int n);

void buf_insert_l(buf_buffer *buf);
void buf_insert_c(buf_buffer *buf, char c);
void buf_insert_s(buf_buffer *buf, const char *s);

void buf_backspace(buf_buffer *buf);

// undo doesn't undo everything, and there is no way to redo, so it is
// probably best to implement undo history separately in your editor.
void buf_undo(buf_buffer *buf);

// print at most height lines of buf, starting at the scrolled-to line.
// - linenums: a format string that takes a single %u; drawn at the
//             start of every line in the buffer
// - eoflines: a format string that takes a single %u; drawn at the
//             start of every line after the end of the buffer, to
//             fill to height lines
void buf_printall(buf_buffer *buf, unsigned int height,
    const char *linenums, const char *eoflines);

#endif // BUFFER_H
