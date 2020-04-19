#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#include <sys/time.h>
#include <pthread.h>


static pthread_t       s_thread;
static pthread_mutex_t s_mutex;
static pthread_cond_t  s_cond;

static int32_t xprint(const char* fmt, ... )
{
    int  ret = 0;
    char buffer[1024];
    char times[64];
    va_list list;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *tm = localtime(&(tv.tv_sec));
    suseconds_t tu = tv.tv_usec;
    sprintf(times, "%04d/%02d/%02d-%02d:%02d:%02d.%03ld", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, tu/1000);

    va_start(list, fmt);
    ret = vsnprintf(buffer, 1024, fmt, list);
    va_end(list);

    printf("%s %s", times, buffer);
    fflush(stdout);
    return ret;
}

static int32_t make_spec(struct timespec *pts, uint32_t timeout)
{
    int ret = 0;

    clock_gettime(CLOCK_MONOTONIC, pts);
    pts->tv_sec  +=  timeout / 1000;
    pts->tv_nsec += (timeout % 1000) * 1000000;
    if (pts->tv_nsec >= 1000000000) {
        pts->tv_sec  += 1;
        pts->tv_nsec -= 1000000000;
    }
    return ret;
}

static int32_t cond_send(void)
{
    pthread_mutex_lock(&s_mutex);

    pthread_cond_signal(&s_cond);

    pthread_mutex_unlock(&s_mutex);

    printf("press any key. \n");
    fflush(stdout);
    getchar();
    return 0;
}

static int32_t cond_wait(void)
{
    struct timespec ts;
    int32_t ret = 0;

    pthread_mutex_lock(&s_mutex);
    xprint("111\n");
re:
    make_spec(&ts, 1000);
    ret = pthread_cond_timedwait(&s_cond, &s_mutex, &ts);
    if (110 == ret) {
        xprint("222\n");
        goto re;
    }

    pthread_mutex_unlock(&s_mutex);

    xprint("333\n");
    return 0;
}

static void *thread_fuc(void *param)
{
    while (1) {
        cond_wait();
    }

    return NULL;
}

int main(int argc, char *args[])
{
    pthread_condattr_t  attr_cond;

    printf("hello xlog.\n");

    pthread_condattr_init(&attr_cond);
    pthread_condattr_setclock(&attr_cond, CLOCK_MONOTONIC);

    pthread_cond_init(&s_cond, &attr_cond);
    pthread_mutex_init(&s_mutex, NULL);
    pthread_create(&s_thread, NULL, thread_fuc, NULL);

    sleep(2);

    while (1) {
        cond_send();
    }
    return 0;
}
