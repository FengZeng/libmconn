#include <stdint.h>

void* mconn_init(int num);
int mconn_open(void* mc, const char* url);
int mconn_read(void* mc, unsigned char* buf, int size);
int mconn_seek(void* mc, int64_t offset);
int64_t mconn_size(void* mc);
void mconn_close(void* mc);