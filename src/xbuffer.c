#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "xbuffer.h"

#define XBF_ERR_LOG(fmt, ...) \
    printf("[xbufr] %s():%d " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define XBF_INF_LOG(fmt, ...) \
    printf("[xbufr] %s() " fmt, __FUNCTION__, ##__VA_ARGS__)

#define XBF_ENTER_LOCK(lck) pthread_mutex_lock  (lck)
#define XBF_LEAVE_LOCK(lck) pthread_mutex_unlock(lck)


typedef pthread_mutex_t* xhlock_p;
typedef pthread_cond_t*  xhcond_p;
typedef uint8_t*         uint08_p;
typedef int64_t          xtime_t;

struct xbuffer_ {
    uint32_t _size;
    uint32_t _used;
    uint32_t _dptr;
    uint32_t _rptr;
    uint32_t _wptr;
    uint08_p _data;

    xhlock_p _lock;
    xhcond_p _rcnd;
    xhcond_p _wcnd;
} ;


static int32_t xbufr_sync_init (xbuffer_p hbuffer);
static int32_t xbufr_able_read (xbuffer_p hbuffer);
static int32_t xbufr_able_write(xbuffer_p hbuffer, int32_t size);

static int32_t xbufr_make_spec(struct timespec *pts, uint32_t timeout);
static int64_t xbufr_timems(void);


int32_t xbuffer_create (xbuffer_p *hbuffer, int32_t size)
{
    struct xbuffer_ *handle = NULL;

    handle = (xbuffer_p)malloc(sizeof(struct xbuffer_));
    memset(handle, 0x00, sizeof(struct xbuffer_));

    handle->_size = size * sizeof(uint8_t);
    handle->_data = (uint8_t*)malloc(handle->_size);
    memset(handle->_data, 0x00, handle->_size);

    if (0 != xbufr_sync_init(handle)) {
        xbuffer_destroy(handle);
        *hbuffer = NULL;
        return -1;
    }

    *hbuffer = handle;
    return 0;
}

int32_t xbuffer_destroy(xbuffer_p hbuffer)
{
    if (NULL == hbuffer       ) { return -1;                             }
    if (NULL != hbuffer->_rcnd) { pthread_cond_destroy (hbuffer->_rcnd); }
    if (NULL != hbuffer->_wcnd) { pthread_cond_destroy (hbuffer->_wcnd); }
    if (NULL != hbuffer->_lock) { pthread_mutex_destroy(hbuffer->_lock); }
    if (NULL != hbuffer->_data) { free(hbuffer->_data);                  }
    free(hbuffer);

    return 0;
}

int32_t xbuffer_write(xbuffer_p hbuffer, xbfer_msg_t *xmsg, int32_t timeout)
{
    int32_t ret = 0;

    if (NULL==hbuffer || NULL==xmsg) { return -1; }

    XBF_ENTER_LOCK(hbuffer->_lock);

    while (!xbufr_able_write(hbuffer, xmsg->_size+sizeof(xmsg->_size))) {
        struct timespec ts;
        xtime_t dtime;

        dtime = xbufr_timems();
        xbufr_make_spec(&ts, timeout);
        ret = pthread_cond_timedwait(hbuffer->_wcnd, hbuffer->_lock, &ts);
        if (-1 != timeout) {
            timeout = timeout - (xbufr_timems() - dtime);
            if (timeout <= 0 ) { break; }
        }
    }

    if (0 != ret) {
        XBF_LEAVE_LOCK(hbuffer->_lock);
        return ret;
    }

    if (0 == hbuffer->_used) {
        pthread_cond_signal(hbuffer->_rcnd);
        XBF_INF_LOG("pthread_cond_signal() cond-r. \n");
    }

    memcpy(&hbuffer->_data[hbuffer->_wptr], &xmsg->_size, sizeof(xmsg->_size));
    hbuffer->_wptr += sizeof(xmsg->_size);
    hbuffer->_used += sizeof(xmsg->_size);

    memcpy(&hbuffer->_data[hbuffer->_wptr], xmsg->_data, xmsg->_size);
    hbuffer->_wptr += xmsg->_size;
    hbuffer->_used += xmsg->_size;

    XBF_LEAVE_LOCK(hbuffer->_lock);

    return 0;
}

int32_t xbuffer_read (xbuffer_p hbuffer, xbfer_msg_t *xmsg, int32_t timeout)
{
    int32_t ret = 0;
    uint8_p tmp = NULL;

    if (NULL==hbuffer || NULL==xmsg || NULL==xmsg->_data) { return -1; }

    XBF_ENTER_LOCK(hbuffer->_lock);
    while (!xbufr_able_read(hbuffer)) {
        struct timespec ts;
        xtime_t dtime;

        dtime = xbufr_timems();
        xbufr_make_spec(&ts, timeout);
        ret = pthread_cond_timedwait(hbuffer->_rcnd, hbuffer->_lock, &ts);
        if (-1 != timeout) {
            timeout = timeout - (xbufr_timems() - dtime);
            if (timeout <= 0 ) { break; }
        }
    }

    if (0 != ret) {
        XBF_LEAVE_LOCK(hbuffer->_lock);
        return ret;
    }

    if (hbuffer->_used >= hbuffer->_size) {
        pthread_cond_signal(hbuffer->_wcnd);
        XBF_INF_LOG("pthread_cond_signal() cond-w. \n");
    }

    tmp = &hbuffer->_data[hbuffer->_rptr];
    memcpy(&xmsg->_size, tmp, sizeof(xmsg->_size));
    tmp = tmp + sizeof(xmsg->_size);
    memcpy(xmsg->_data, tmp, xmsg->_size);

    hbuffer->_used -= xmsg->_size + sizeof(xmsg->_size);
    hbuffer->_rptr += xmsg->_size + sizeof(xmsg->_size);
    if (hbuffer->_rptr == hbuffer->_dptr) {
        hbuffer->_rptr = 0;
        hbuffer->_dptr = 0;
    }

    XBF_LEAVE_LOCK(hbuffer->_lock);

    return 0;
}

int32_t xbuffer_take(xbuffer_p hbuffer, xbfer_msg_t *xmsg, int32_t timeout)
{
    int32_t ret = 0;
    uint8_p tmp = NULL;

    if (NULL==hbuffer || NULL==xmsg) { return -1; }

    XBF_ENTER_LOCK(hbuffer->_lock);
    while (!xbufr_able_read(hbuffer)) {
        struct timespec ts;
        xtime_t dtime;

        dtime = xbufr_timems();
        xbufr_make_spec(&ts, timeout);
        ret = pthread_cond_timedwait(hbuffer->_rcnd, hbuffer->_lock, &ts);
        if (-1 != timeout) {
            timeout = timeout - (xbufr_timems() - dtime);
            if (timeout <= 0 ) { break; }
        }
    }

    if (0 != ret) {
        XBF_LEAVE_LOCK(hbuffer->_lock);
        return ret;
    }

    if (hbuffer->_used >= hbuffer->_size) {
        pthread_cond_signal(hbuffer->_wcnd);
        XBF_INF_LOG("pthread_cond_signal() cond-w. \n");
    }

    tmp = &hbuffer->_data[hbuffer->_rptr];
    memcpy(&xmsg->_size, tmp, sizeof(xmsg->_size));
    xmsg->_data = tmp + sizeof(xmsg->_size);

    XBF_LEAVE_LOCK(hbuffer->_lock);

    return 0;
}

int32_t xbuffer_give(xbuffer_p hbuffer)
{
    xbfer_msg_t xmsg[1];

    if (NULL == hbuffer) { return -1; }

    XBF_ENTER_LOCK(hbuffer->_lock);

    memcpy(&xmsg->_size, &hbuffer->_data[hbuffer->_rptr], sizeof(xmsg->_size));

    hbuffer->_used -= xmsg->_size + sizeof(xmsg->_size);
    hbuffer->_rptr += xmsg->_size + sizeof(xmsg->_size);
    if (hbuffer->_rptr == hbuffer->_dptr) {
        hbuffer->_rptr = 0;
        hbuffer->_dptr = 0;
    }

    XBF_LEAVE_LOCK(hbuffer->_lock);

    return 0;
}

int32_t xbuffer_dump(xbuffer_p hbuffer)
{
    if (NULL == hbuffer) { return -1; }

    XBF_ENTER_LOCK(hbuffer->_lock);
    XBF_INF_LOG("size=%d used=%d dptr=%d wptr=%d rptr=%d\n", \
                hbuffer->_size, hbuffer->_used, hbuffer->_dptr, hbuffer->_wptr, hbuffer->_rptr);
    XBF_LEAVE_LOCK(hbuffer->_lock);

    return 0;
}


int32_t xbufr_sync_init(xbuffer_p hbuffer)
{
    int32_t ret = 0;
    pthread_mutexattr_t attr_mutex;
    pthread_condattr_t  attr_cond;

    ret = pthread_mutexattr_init(&attr_mutex);
    if (0 != ret) {
        XBF_ERR_LOG("pthread_mutexattr_init() failed. ret=%d\n", ret);
        return -1;
    }

    ret = pthread_condattr_init(&attr_cond);
    if (0 != ret) {
        XBF_ERR_LOG("pthread_condattr_init() failed. ret=%d\n", ret);
        pthread_mutexattr_destroy(&attr_mutex);
        return -1;
    }

    hbuffer->_lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    ret = pthread_mutex_init(hbuffer->_lock, &attr_mutex);
    if (0 != ret) {
        XBF_ERR_LOG("pthread_mutex_init() failed. ret=%d\n", ret);
        goto xfailed;
    }

    ret = pthread_condattr_setclock(&attr_cond, CLOCK_MONOTONIC);
    if (0 != ret) {
        XBF_ERR_LOG("pthread_condattr_setclock() CLOCK_MONOTONIC failed. ret=%d\n", ret);
        goto xfailed;
    }
    hbuffer->_rcnd = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    ret = pthread_cond_init(hbuffer->_rcnd, &attr_cond);
    if (0 != ret) {
        XBF_ERR_LOG("pthread_cond_init() rcd failed. ret=%d\n", ret);
        goto xfailed;
    }
    hbuffer->_wcnd = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    ret = pthread_cond_init(hbuffer->_wcnd, &attr_cond);
    if (0 != ret) {
        XBF_ERR_LOG("pthread_cond_init() wcd failed. ret=%d\n", ret);
        goto xfailed;
    }

xfailed:
    pthread_mutexattr_destroy(&attr_mutex);
    pthread_condattr_destroy (&attr_cond);
    return ret;
}

int32_t xbufr_able_write(xbuffer_p hbuffer, int32_t size)
{
    if (((hbuffer->_size-hbuffer->_used) >= size) \
         && ((hbuffer->_size-hbuffer->_wptr) >= size)) {
        return 1;
    }
    else {
        hbuffer->_dptr  = hbuffer->_wptr;
        hbuffer->_wptr  = 0;
        hbuffer->_used += hbuffer->_size - hbuffer->_dptr;
        if (hbuffer->_rptr >= size) {
            return 1;
        }
    }

    return 0;
}

int32_t xbufr_able_read(xbuffer_p hbuffer)
{
    if (hbuffer->_used > 0) {
        return 1;
    }
    return 0;
}

int32_t xbufr_make_spec(struct timespec *pts, uint32_t timeout)
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

xtime_t xbufr_timems(void)
{
    int ret = 0;
    struct timespec ts = {0, 0};

    ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (0 != ret) {
        return ret;
    }
    return (xtime_t)(ts.tv_sec * 1000u + ts.tv_nsec/1000000u);
}

void xbuffer_test(void)
{
    return ;
}
