#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include "xlog.h"

typedef struct xlog_ {

}xlog_t;

int32_t xlog_init(void)
{
    return 0;
}

int32_t xlog_term(void)
{
    return 0;
}

int32_t xlog_set_attr(xlog_attr_t *attr, int32_t size)
{
    return 0;
}

int32_t xlog_get_attr(xlog_attr_t *attr, int32_t size)
{
    return 0;
}

int32_t xlog_print(XLOG_LEVEL_E level, const char *format, ...)
{
    return 0;
}

