#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define C_RESET   "\033[0m"
#define C_BLUE    "\033[34m"
#define C_CYAN    "\033[36m"
#define C_YELLOW  "\033[33m"
#define C_GREEN   "\033[32m"
#define C_GREY    "\033[90m"

static void out(char *s, size_t size, size_t *i, const char *t) {
  for (size_t k = 0; t[k]; k++) {
    if (*i + 1 >= size) return;
    s[(*i)++] = t[k];
  }
}

static void outc(char *s, size_t size, size_t *i, char c) {
  if (*i + 1 < size)
    s[(*i)++] = c;
}

/* entry point */
void highlight(char *s, size_t size, const char *l) {
  size_t i = 0;
  size_t j = 0;

  if (!s || !l || size == 0) return;

  /* ---- leading comment or inline comment ---- */
  if (l[0] == '#') {
    out(s, size, &i, C_GREY);
    while (l[j] && i + 1 < size)
      outc(s, size, &i, l[j++]);
    out(s, size, &i, C_RESET);
    s[i] = '\0';
    return;
  }

  /* ---- target rule: "name:" ---- */
  size_t start = 0;
  while (l[start] == ' ' || l[start] == '\t') start++;

  size_t t = start;
  while (l[t] && l[t] != ':' && l[t] != '=' && !isspace((unsigned char)l[t]))
    t++;

  if (l[t] == ':' && t > start) {
    out(s, size, &i, C_BLUE);
    for (size_t k = 0; k < t; k++)
      outc(s, size, &i, l[k]);

    outc(s, size, &i, l[t++]); /* include ':' */
    out(s, size, &i, C_RESET);

    j = t;

    while (l[j] && i + 1 < size)
      outc(s, size, &i, l[j++]);

    s[i] = '\0';
    return;
  }

  /* ---- variable assignment: VAR = ... ---- */
  size_t eq = start;
  while (l[eq] && l[eq] != '=' && l[eq] != ':' ) eq++;

  if (l[eq] == '=') {
    out(s, size, &i, C_CYAN);

    for (size_t k = 0; k < eq; k++)
      outc(s, size, &i, l[k]);

    outc(s, size, &i, l[eq++]); /* '=' */

    out(s, size, &i, C_RESET);

    j = eq;

    while (l[j] && i + 1 < size)
      outc(s, size, &i, l[j++]);

    s[i] = '\0';
    return;
  }

  /* ---- generic scan ---- */
  while (l[j] && i + 1 < size) {

    /* comment mid-line */
    if (l[j] == '#') {
      out(s, size, &i, C_GREY);
      while (l[j] && i + 1 < size)
        outc(s, size, &i, l[j++]);
      break;
    }

    /* $(...) functions */
    if (l[j] == '$' && l[j+1] == '(') {
      out(s, size, &i, C_YELLOW);
      outc(s, size, &i, l[j++]);
      outc(s, size, &i, l[j++]);

      while (l[j] && l[j] != ')' && i + 1 < size)
        outc(s, size, &i, l[j++]);

      if (l[j] == ')')
        outc(s, size, &i, l[j++]);

      out(s, size, &i, C_RESET);
      continue;
    }

    /* strings */
    if (l[j] == '"' || l[j] == '\'') {
      char q = l[j];
      out(s, size, &i, C_GREEN);
      outc(s, size, &i, l[j++]);

      while (l[j] && l[j] != q && i + 1 < size) {
        if (l[j] == '\\' && l[j+1]) {
          outc(s, size, &i, l[j++]);
          outc(s, size, &i, l[j++]);
        } else {
          outc(s, size, &i, l[j++]);
        }
      }

      if (l[j] == q)
        outc(s, size, &i, l[j++]);

      out(s, size, &i, C_RESET);
      continue;
    }

    /* default */
    outc(s, size, &i, l[j++]);
  }

  s[i] = '\0';
}
