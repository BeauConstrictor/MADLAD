#ifndef ED_H
#define ED_H

#include "buffer.h"

typedef enum {
  NORMAL,
  INSERT,
  COMMAND,
  REPLACE,
} ed_mode;

typedef struct {
  void *lib;
  buf_highlighter fn;
  char name[32];
} ed_highlighter;

typedef struct {
    ed_highlighter highlighter;
    int highlight_col;
} ed_settings;

typedef struct {
  buf_buffer buf;
  ed_mode mode;
  char status[2048];
  unsigned int stat_col;
  int exit;
  ed_settings settings;
} ed_editor;

bool ed_use_highlighter(ed_editor *ed, const char *filetype);

void ed_draw(ed_editor *ed);
void ed_default_settings(ed_editor *ed);
void ed_handle_key(ed_editor *ed, char c);
void ed_chmode(ed_editor *ed, ed_mode mode);

#endif // ED_H
