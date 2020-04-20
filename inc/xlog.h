#ifndef __XLOG_API__
#define __XLOG_API__

#include <stdint.h>
#include <stdarg.h>

#include "xdef.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
    E_DBG = 0x01,
    E_INF = 0x02,
    E_WAR = 0x04,
    E_ERR = 0x08,
    E_RAW = 0x10
}XLOG_LEVEL_E;

typedef enum {
    E_STD_OUT    = 0x01,
    E_LOCAL_FILE = 0x02,
    E_NETWORK    = 0x04
}XLOG_OUTPUT_E;

typedef struct {
    XLOG_OUTPUT_E _oput;
    int32_t       _mask;
    int08_p       _path[128];
    int08_p       _name[ 64];
}xlog_attr_t;


int32_t xlog_init(void);
int32_t xlog_term(void);

int32_t xlog_set_attr(xlog_attr_t *attr, int32_t size);
int32_t xlog_get_attr(xlog_attr_t *attr, int32_t size);

int32_t xlog_print(XLOG_LEVEL_E level, const char *format, ...);



#ifdef  __cplusplus
}
#endif

#endif
