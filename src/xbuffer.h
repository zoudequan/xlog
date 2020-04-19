
#ifndef __XLOG_BUFFER__
#define __XLOG_BUFFER__

#include <stdint.h>

typedef struct xbuffer_* xbuffer_p;
typedef        uint8_t * uint8_p;

typedef struct xbfer_msg_ {
    int16_t _size;
    uint8_p _data;
}xbfer_msg_t;

int32_t xbuffer_create (xbuffer_p *hbuffer, int32_t size);
int32_t xbuffer_destroy(xbuffer_p hbuffer);

int32_t xbuffer_write(xbuffer_p hbuffer, xbfer_msg_t *xmsg, int32_t timeout);
int32_t xbuffer_read (xbuffer_p hbuffer, xbfer_msg_t *xmsg, int32_t timeout);
int32_t xbuffer_take (xbuffer_p hbuffer, xbfer_msg_t *xmsg, int32_t timeout);
int32_t xbuffer_give (xbuffer_p hbuffer);

int32_t xbuffer_dump(xbuffer_p hbuffer);


#endif
