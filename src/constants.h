#ifndef CONSTANTS_H
#define CONSTANTS_H

#define VERSION "v0.2.1"
#define WELCOME "Welcome to MADLAD " VERSION "!"

#define HELP_MSG \
    "Usage: madlad [FILE]\n" \
    "\n" \
    "Options:\n" \
    "  -h, --help     Show this help message\n" \
    "  -v, --version  Print version number\n" \
    " --headless      Enter a command REPL\n" \
    "\n" \
    "If FILE is not found, it will be created on save"

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

#define BUF_STATUS_CMD "status"

#define RESET ANSI(0)

#endif // CONSTANTS_H
