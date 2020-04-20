#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#include <sys/time.h>
#include <pthread.h>

#include "xbuffer.h"

static int32_t xprint(const char* fmt, ... )
{
    int  ret = 0;
    char buffer[1024];
    va_list list;

    struct timeval tv;
    struct tm*     tm;

    memset(buffer, 0x00, sizeof(buffer));
    gettimeofday(&tv, NULL);
    tm = localtime(&(tv.tv_sec));

    snprintf(buffer, 64, "%04d/%02d/%02d-%02d:%02d:%02d.%03ld ", \
             tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,        \
             tm->tm_hour,      tm->tm_min,   tm->tm_sec,         \
             tv.tv_usec/1000);

    va_start(list, fmt);
    ret = vsnprintf(buffer+24, 1000, fmt, list);
    va_end(list);

    printf("%s", buffer);
    fflush(stdout);
    return ret;
}


int main(int argc, char *args[])
{
    xbuffer_p hbuffer = NULL;
    int32_t ret = 0;
    int32_t inx = 0;
    int8_t  log[128];
    xbfer_msg_t xmsg;

    xprint("hello xlog.\n");
    ret = xbuffer_create (&hbuffer, 55);
    xprint("xbuffer_create() ret=%d.\n", ret);

    xmsg._size = 0;
    xmsg._data = (uint8_p)log;
    ret = xbuffer_read(hbuffer, &xmsg, 1000);
    if (ret != 0) {
            xprint("xbuffer_read() ret=%d\n", ret);
    }

//    getchar();
    for (inx=0; inx<6; inx++) {
        xmsg._size = 8;
        xmsg._data = (uint8_p)"1234567";
        ret = xbuffer_write(hbuffer, &xmsg, 100);
        if (ret != 0) {
            xprint("xbuffer_write() ret=%d\n", ret);
        }
        xbuffer_dump(hbuffer);
    }

//    getchar();
    for (inx=0; inx<6; inx++) {
        xmsg._size = 0;
        xmsg._data = (uint8_p)log;
        ret = xbuffer_read(hbuffer, &xmsg, 100);
        if (ret != 0) {
            xprint("xbuffer_read() ret=%d\n", ret);
        }
        else {
            xprint("xbuffer_read() size=%d data=%s\n", xmsg._size, xmsg._data);
        }
        //xbuffer_dump(hbuffer);
    }

    ret = xbuffer_destroy(hbuffer);
    xprint("gobye xlog.\n");
    return 0;
}
