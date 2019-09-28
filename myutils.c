#include "myutils.h"

#define PRINT_BUFF_SIZE  1024

static const char *_name;
static bool _color_enabled = true;

void myutils_set_name(const char *name)
{
    _name = name;
}

const char *myutils_get_name(void)
{
    return _name;
}

void myutils_set_color_enabled(bool enabled)
{
    _color_enabled = enabled;
}

void mymsg(int type, int _errno, int status, const char *format, ...) {
    va_list ap;
    const char *type_str = "";
    color_t color = DEF_COLOR;

    if (_name)
        fprintf(stderr, "%s: ", _name);

    switch (type) {
        case FATAL:
            color = RED;
            type_str = "致命错误：";
            break;
        case ERROR:
            color = RED;
            type_str = "错误：";
            break;
        case WARNING:
            color = PURPLE;
            type_str = "警告：";
            break;
        default:
            color = DEF_COLOR;
            type_str = "";
            break;
    }

    myfcolor(stderr, color);
    fprintf(stderr, "%s", type_str);
    myfcolor(stderr, DEF_COLOR);

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    if (_errno)
        fprintf(stderr, ": %s。\n", strerror(_errno));
    else
        fprintf(stderr, "。\n");

    if (status)
        exit(status);
}


void myfcolor(FILE *fp, color_t color)
{
    if (_color_enabled)
        fprintf(fp, "\033[%dm", color == DEF_COLOR ? 0 : 30 + color);
}

void mybcolor(FILE *fp, color_t color)
{
    if (_color_enabled)
        fprintf(fp, "\033[%dm", color == DEF_COLOR ? 0 : 40 + color);
}
