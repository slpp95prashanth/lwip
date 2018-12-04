#include <stdio.h>
#include <stdarg.h>

int diag_printf(char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return (ret);
}

int main(int argc, char **args)
{
    _main();

    return 0;
}

