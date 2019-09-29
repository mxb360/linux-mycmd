#include <unistd.h>
#include <getopt.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define NO_TYPE     0
#define ERROR       1
#define FATAL       2
#define WARNING     3

#define BLACK       0
#define RED         1
#define GREEN       2
#define YELLOW      3
#define BLUE        4
#define PURPLE      5
#define CYGN        6
#define WHITLE      7
#define DEF_COLOR   8

#define FSTR_BLACK       "\033[30m"
#define FSTR_RED         "\033[31m"
#define FSTR_GREEN       "\033[32m"
#define FSTR_YELLOW      "\033[33m"
#define FSTR_BLUE        "\033[34m"
#define FSTR_PURPLE      "\033[35m"
#define FSTR_CYGN        "\033[36m"
#define FSTR_WHITLE      "\033[37m"
#define FSTR_DEF_COLOR   "\033[0m"

#define BSTR_BLACK       "\033[40m"
#define BSTR_RED         "\033[41m"
#define BSTR_GREEN       "\033[42m"
#define BSTR_YELLOW      "\033[43m"
#define BSTR_BLUE        "\033[44m"
#define BSTR_PURPLE      "\033[45m"
#define BSTR_CYGN        "\033[46m"
#define BSTR_WHITLE      "\033[47m"
#define BSTR_DEF_COLOR   "\033[0m"

#define HIGHLIGHT        "\033[1m"

/* myutils config */
void myutils_set_name(const char *name);
const char *myutils_get_name(void);
void myutils_set_color_enabled(bool enabled);

/* myutils output functions */
void mymsg(int type, int _errno, int status, const char *errformat, ...);
#define myfatal(_errno, status, format...)      mymsg(FATAL, _errno, status, ##format)
#define myerror(_errno, status, format...)      mymsg(ERROR, _errno, status, ##format)

/* myutils color */
typedef int color_t;

void myfcolor(FILE *fp, color_t color);
void mybcolor(FILE *fp, color_t color);
