#ifndef BUFFER_H
#define BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define LINE_WIDTH 128

// ERRORS:
// for now, many errors are caught by asserts with comments above
// them explaining their meaning. this will be changed in future,
// likely to some kind of errno system.

typedef void (*buf_highlighter)(char *s, size_t size, const char *l);

typedef struct buf_line buf_line;
struct buf_line {
  buf_line *prev;
  char text[LINE_WIDTH]; // text should be null-terminated but still
  size_t len;            // has len for convenience
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
  size_t old_cursor;    // cursor position, from when there was an editabl
  size_t scrolled_lno;  // lineno of scrolled_l
  char path[4096];
} buf_buffer;

// CREATING A NEW BUFFER:
// on the stack:
//   buf_buffer buf = {0};
// on the heap:
//   buf_buffer *buf = malloc(sizeof(buf_buffer));
//   memset(buf, 0, sizeof(buf_buffer));

// move cursor up n lines
void buf_cursor_u(buf_buffer *buf, unsigned int n);
// move cursor down n lines
void buf_cursor_d(buf_buffer *buf, unsigned int n);
// move cursor left n characters
void buf_cursor_l(buf_buffer *buf, unsigned int n);
// move cursor right n characters
void buf_cursor_r(buf_buffer *buf, unsigned int n);
// move cursor to start of line
void buf_cursor_s(buf_buffer *buf);
// move cursor to end of line
void buf_cursor_e(buf_buffer *buf);

// NOTE:
// when moving the cursor up, a scroll is also automatically done
// when the cursor is at the top line of the screen. this library
// has no notion of a bottom screen line, as it does not track screen
// height. however, buf_printall does take a screen height, and will
// automatically scroll down until the cursor is visible again.
// basically, if you don't use the built-in buf_printall function to
// draw the buffer, you have to make sure to manually scroll down to
// keep the cursor in view.

// scroll up n lines
void buf_scroll_u(buf_buffer *buf, unsigned int n, bool move_cur);
// scroll down n lines
void buf_scroll_d(buf_buffer *buf, unsigned int n, bool move_cur);

// create a new line below the cursor, and move cursor to it
void buf_insert_l(buf_buffer *buf);
// insert a single character before the cursor
void buf_insert_c(buf_buffer *buf, char c);
// insert a string before the cursor
void buf_insert_s(buf_buffer *buf, const char *s);
// insert the contents of a stream before the cursor
void buf_insert_f(buf_buffer *buf, FILE *f);

// delete n characters from before the cursor
void buf_delete_c(buf_buffer *buf, unsigned int n);
// delete n line from before the cursor
void buf_delete_l(buf_buffer *buf, unsigned int n);

// replace the contents of the current line with s; cannot undo
void buf_replace_l(buf_buffer *buf, const char *s);

// write the contents of the buffer to f
void buf_fwrite(buf_buffer *buf, FILE *f);

// this undo function is *incredibly* limited. it remove all previous
// insertions made after the most receent line change or call to
// buf_flush_changes(buf) - there is no undo history.
// returns true is there was anything to undo, or false if not - 
// this value is equal to (buf->edits != NULL).
bool buf_undo(buf_buffer *buf);
void buf_flush_changes(buf_buffer *buf);

// print at most height lines of buf, starting at the scrolled-to line.
// - linenums: a format string that takes a single %u; drawn at the
//             start of every line in the buffer
// - eoflines: a format string that takes a single %u; drawn at the
//             start of every line after the end of the buffer, to
//             fill to height lines
// - highlight_col: a specific column to highlight, such as 80
//                  (-1 for none)
void buf_printall(buf_buffer *buf, unsigned int height,
    const char *linenums, const char *eoflines,
    int *cur_row, int *cur_col, buf_highlighter highlighter,
    int highlight_col);

// restore a buffer to being completely empty
void buf_clear(buf_buffer *buf);

// get the column that the cursor is in
size_t buf_cursor_x(buf_buffer *buf);
// get the number of characters in the current line
size_t buf_line_len(buf_buffer *buf);
// returns an array containing the text of the current line
// you don't need to worry about freeing this array
const char *buf_line_text(buf_buffer *buf);
// returns the char under the cursor, or \0 if there isn't one
char buf_line_char(buf_buffer *buf);

// returns whether or not the cursor is on the first line
bool buf_at_sof(buf_buffer *buf);
// returns whether or not the cursor is at the start of the line
bool buf_at_sol(buf_buffer *buf);
// returns whether or not the cursor is on the last line
bool buf_at_eof(buf_buffer *buf);
// returns whether or not the cursor is after the last char in the line
bool buf_at_eol(buf_buffer *buf);
// returns whether or not the cursor is on the last char in the line
bool buf_at_lastc(buf_buffer *buf);
// returns whether or not the line length is 0, but slightly faster
// than doing that directly
bool buf_line_empty(buf_buffer *buf);

#endif // BUFFER_H
