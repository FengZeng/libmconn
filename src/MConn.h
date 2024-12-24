#ifndef MULTI_CONNECT
#define MULTI_CONNECT
#ifdef __cplusplus
extern "C" {
#endif

#include "libavformat/avformat.h"

void* mconn_wrap_init(int num);
int mconn_wrap_open(void* mc, const char* url);
int mconn_wrap_read(void* mc, unsigned char* buf, int size);
int mconn_wrap_seek(void* mc, int64_t pos);
int64_t mconn_wrap_size(void* mc);
void mconn_wrap_close(void* mc);

#ifdef __cplusplus
} 
#endif

#ifdef __cplusplus

#include "AsyncReader.h"
#include <string>
#define MAX_THREAD 16

std::string current_time();

class MConn {
public:
    MConn();
    int Init(int num);
    int Open(const char* url);
    int Active();
    int Read(unsigned char* buf, int size);
    int Seek(int64_t pos);
    int64_t Size();
    int Close();

public:
    int m_thdNum;
    int m_bufferCapacity {0};
    int64_t m_newPosition {0};

private:
    std::atomic<bool> m_shutdown {false};
    std::condition_variable m_cvUser;
    bool m_useAsync;
    int m_curIdx;
    int m_readSize;

    int m_activeId;

    AVIOContext* m_avio;
    AsyncReader* m_reader[MAX_THREAD];
};
#endif

#endif