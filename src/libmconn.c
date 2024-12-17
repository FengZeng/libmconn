#include "MConn.h"

void* mconn_init(int num) {
    return mconn_wrap_init(num);
}

int mconn_open(void* mc, const char* url) {
    return mconn_wrap_open(mc, url);
}

int mconn_read(void* mc, unsigned char* buf, int size) {
    return mconn_wrap_read(mc, buf, size);
}

int mconn_seek(void* mc, int64_t pos) {
    return mconn_wrap_seek(mc, pos);
}

int64_t mconn_size(void* mc) {
    return mconn_wrap_size(mc);
}

void mconn_close(void* mc) {
    mconn_wrap_close(mc);
}