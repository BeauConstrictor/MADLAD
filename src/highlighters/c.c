#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

#define C_RESET   "\033[0m"
#define C_KEY     "\033[38;5;208m"
#define C_TYPE    "\033[38;5;11m"
#define C_STR     "\033[32m"
#define C_COMMENT "\033[90m"
#define C_NUM     "\033[35m"
#define C_PREPROC "\033[34m"

static int is_kw(const char *w, size_t n) {
  static const char *kw[] = {
    "if","else","while","for","return","break","continue","switch","case",
    "sizeof","typedef","struct","enum","union","static","extern","const"
  };

  for (size_t i = 0; i < sizeof(kw)/sizeof(kw[0]); i++) {
    if (strlen(kw[i]) == n && strncmp(kw[i], w, n) == 0)
      return 1;
  }
  return 0;
}

static int is_type(const char *w, size_t n) {
  static const char *types[] = {
    "int","char","void","float","double","long","short",
    "size_t","unsigned","signed","bool"
  };

  for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
    if (strlen(types[i]) == n && strncmp(types[i], w, n) == 0)
      return 1;
  }
  return 0;
}

static void out(char *s, size_t size, size_t *i, const char *t) {
  if (!t) return;

  for (size_t k = 0; t[k] != '\0'; k++) {
    if (*i + 1 >= size) return;
    s[(*i)++] = t[k];
  }
}

static void outc(char *s, size_t size, size_t *i, char c) {
  if (*i + 1 < size)
    s[(*i)++] = c;
}

void highlight(char *s, size_t size, const char *l) {
  size_t i = 0;
  size_t j = 0;

  if (size == 0 || !s || !l) return;

  while (l[j] && i + 1 < size) {
      if (j == 0 && l[j] == '#') {
        out(s, size, &i, C_PREPROC);

        while (l[j] && i + 1 < size) {
          outc(s, size, &i, l[j++]);
        }

        out(s, size, &i, C_RESET);
        break;
      }

      if (l[j] == '/' && l[j+1] == '/') {
        out(s, size, &i, C_COMMENT);

      while (l[j] && i + 1 < size) {
        outc(s, size, &i, l[j++]);
      }

      out(s, size, &i, C_RESET);
      break;
    }

    if (l[j] == '"') {
      out(s, size, &i, C_STR);
      outc(s, size, &i, l[j++]);

      while (l[j] && i + 1 < size) {
        if (l[j] == '\\' && l[j+1]) {
          outc(s, size, &i, l[j++]);
          outc(s, size, &i, l[j++]);
        } else if (l[j] == '"') {
          break;
        } else {
          outc(s, size, &i, l[j++]);
        }
      }

      if (l[j] == '"')
        outc(s, size, &i, l[j++]);

      out(s, size, &i, C_RESET);
      continue;
    }

    if (l[j] == '\'') {
      out(s, size, &i, C_STR);
      outc(s, size, &i, l[j++]);

      if (l[j] == '\\' && l[j+1]) {
        outc(s, size, &i, l[j++]);
        outc(s, size, &i, l[j++]);
      } else if (l[j]) {
        outc(s, size, &i, l[j++]);
      }

      if (l[j] == '\'')
        outc(s, size, &i, l[j++]);

      out(s, size, &i, C_RESET);
      continue;
    }

    if (isdigit((unsigned char)l[j])) {
      out(s, size, &i, C_NUM);

      while (l[j] && (isdigit((unsigned char)l[j]) || l[j] == '.') && i + 1 < size)
        outc(s, size, &i, l[j++]);

      out(s, size, &i, C_RESET);
      continue;
    }

    if (isalpha((unsigned char)l[j]) || l[j] == '_') {
      char buf[128];
      size_t k = 0;

      while ((isalnum((unsigned char)l[j]) || l[j] == '_') && k + 1 < sizeof(buf)) {
        buf[k++] = l[j++];
      }
      buf[k] = '\0';

      int kw = is_kw(buf, k);
      int ty = is_type(buf, k);

      if (kw) out(s, size, &i, C_KEY);
      else if (ty) out(s, size, &i, C_TYPE);

      for (size_t m = 0; m < k; m++)
        outc(s, size, &i, buf[m]);

      if (kw || ty)
        out(s, size, &i, C_RESET);

      continue;
    }

    outc(s, size, &i, l[j++]);
  }

  out(s, size, &i, C_RESET);
  s[i] = '\0';
}
