#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

#define C_RESET    "\033[0m"
#define C_KEY      "\033[38;5;208m"
#define C_BUILTIN  "\033[38;5;11m"
#define C_STR      "\033[32m"
#define C_COMMENT  "\033[90m"
#define C_NUM      "\033[35m"
#define C_VAR      "\033[36m"

static int is_kw(const char *w, size_t n) {
    static const char *kw[] = {
        "if","then","else","elif","fi","for","while","do","done",
        "case","esac","function","select","until","in","break","continue","return"
    };
    for (size_t i = 0; i < sizeof(kw)/sizeof(kw[0]); i++)
        if (strlen(kw[i]) == n && strncmp(kw[i], w, n) == 0)
            return 1;
    return 0;
}

static int is_builtin(const char *w, size_t n) {
    static const char *builtins[] = {
        "echo","printf","read","exit","export","unset","cd","pwd","shift","test","let"
    };
    for (size_t i = 0; i < sizeof(builtins)/sizeof(builtins[0]); i++)
        if (strlen(builtins[i]) == n && strncmp(builtins[i], w, n) == 0)
            return 1;
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
    size_t i = 0, j = 0;
    if (!s || !l || size == 0) return;

    while (l[j] && i + 1 < size) {
        // Comments
        if (l[j] == '#') {
            out(s, size, &i, C_COMMENT);
            while (l[j] && i + 1 < size) outc(s, size, &i, l[j++]);
            out(s, size, &i, C_RESET);
            break;
        }

        // Strings
        if (l[j] == '"' || l[j] == '\'') {
            char quote = l[j];
            out(s, size, &i, C_STR);
            outc(s, size, &i, l[j++]);
            while (l[j] && l[j] != quote && i + 1 < size) {
                if (l[j] == '\\' && l[j+1]) {
                    outc(s, size, &i, l[j++]);
                    outc(s, size, &i, l[j++]);
                } else {
                    outc(s, size, &i, l[j++]);
                }
            }
            if (l[j] == quote) outc(s, size, &i, l[j++]);
            out(s, size, &i, C_RESET);
            continue;
        }

        // Variables like $VAR or ${VAR}
        if (l[j] == '$') {
            out(s, size, &i, C_VAR);
            outc(s, size, &i, l[j++]);
            if (l[j] == '{') outc(s, size, &i, l[j++]);
            while (isalnum((unsigned char)l[j]) || l[j]=='_' || l[j]=='}')
                outc(s, size, &i, l[j++]);
            out(s, size, &i, C_RESET);
            continue;
        }

        // Numbers
        if (isdigit((unsigned char)l[j])) {
            out(s, size, &i, C_NUM);
            while (isdigit((unsigned char)l[j]) || l[j] == '.' || l[j]=='e')
                outc(s, size, &i, l[j++]);
            out(s, size, &i, C_RESET);
            continue;
        }

        // Keywords or builtins
        if (isalpha((unsigned char)l[j]) || l[j] == '_') {
            char buf[128];
            size_t k = 0;
            while ((isalnum((unsigned char)l[j]) || l[j]=='_') && k + 1 < sizeof(buf)) {
                buf[k++] = l[j++];
            }
            buf[k] = '\0';
            int kw = is_kw(buf, k);
            int bi = is_builtin(buf, k);
            if (kw) out(s, size, &i, C_KEY);
            else if (bi) out(s, size, &i, C_BUILTIN);
            for (size_t m = 0; m < k; m++) outc(s, size, &i, buf[m]);
            if (kw || bi) out(s, size, &i, C_RESET);
            continue;
        }

        outc(s, size, &i, l[j++]);
    }

    out(s, size, &i, C_RESET);
    s[i] = '\0';
}
