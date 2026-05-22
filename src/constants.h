#ifndef CONSTANTS_H
#define CONSTANTS_H

#define VERSION "v0.0.0"
#define WELCOME "Welcome to MADLAD " VERSION "!"

#define HIGHLIGHT_COL 80

#define BLUE 34
#define YELLOW 33
#define RED 31
#define GREY 90
#define DEFAULT 39

#define STR(x) #x
#define XSTR(x) STR(x)
#define ANSI(col) "\033[" XSTR(col) "m"

#define SHIFT_(key) ((key) - 32)
#define CTRL_(key) (SHIFT_(key) & 0x1f)
#define ESC '\033'

#define word_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"
#define WORD_DELIM " \t\n"

#define RESET ANSI(0)

#endif // CONSTANTS_H
