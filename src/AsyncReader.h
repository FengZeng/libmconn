#ifndef ASYNC_REAADER
#define ASYNC_REAADER
#include <chrono>
#include <thread>


extern "C" {
#include "libavformat/avformat.h"
}

class MConn;

class AsyncReader {
public:
    AsyncReader(int id, MConn* mc, bool ready, int size);
    ~AsyncReader();
    void ThreadWork();
public:
    std::atomic<bool> m_seek {false};
    std::atomic<bool> m_close {false};
    AVIOContext* m_avio;
    std::atomic<bool> m_eof {false};
    unsigned char* m_buffer;
    int m_bufferSize;
    int m_bufferUsed;
    std::atomic<bool> m_bufferReady {false};
    std::condition_variable m_cvRead;
private:
    MConn* m_mc;
    int m_id;
    std::thread m_thd;
};

#endif