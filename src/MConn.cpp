#include <chrono>
#include <cstdint>

#include "MConn.h"
#include "libavformat/avio.h"

static int interrupt_cb(void *opaque)
{
    AsyncReader *reader = (AsyncReader*)opaque;
    return reader->m_close.load();
}

std::string current_time() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_time_t);

    char buffer[16]; // Buffer size enough for "HH:MM:SS.mmm"
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);  // Only time (without date)
    snprintf(buffer + 8, sizeof(buffer) - 8, ".%03d", static_cast<int>(ms.count()));

    return std::string(buffer);
}

MConn::MConn() {

}

int MConn::Init(int num) {
    m_thdNum = num;
    m_shutdown = false;
    m_useAsync = false;
    m_readSize = 0;
    m_activeId = 0;

    for (int i=0; i<m_thdNum; i++) {
        m_reader[i] = new AsyncReader(i, this, true, 0);
    }

    return 0;
}

int MConn::Open(const char* url) {
    m_curIdx = 0;
    m_BufferCapacity = 0;
    m_notifySeek = false;

    for (int i=0; i<m_thdNum; i++) {
        AVIOInterruptCB cb = {
            .callback = interrupt_cb,
            .opaque = m_reader[i],
        };
        avio_open2(&m_reader[i]->m_avio, url, AVIO_FLAG_READ, &cb, NULL);
        if (i == 0) {
            int64_t videoSize = Size();
            if (videoSize < 2147483648)
                m_BufferCapacity = 6 * 1024 * 1024;
            else if (videoSize < 4294967296)
                m_BufferCapacity = 9 * 1024 * 1024;
            else if (videoSize < 6442450944)
                m_BufferCapacity = 13 * 1024 * 1024;
            else if (videoSize < 9663676416)
                m_BufferCapacity = 18 * 1024 * 1024;
            else
                m_BufferCapacity = 20 * 1024 * 1024;
        }
        m_reader[i]->m_buffer = (unsigned char*)malloc(m_BufferCapacity * sizeof(unsigned char));
    }
    m_avio = m_reader[m_thdNum - 1]->m_avio;
    return 0;
}

int MConn::Notify() {
    m_cvUser.notify_one();
    return 0;
}

int MConn::Read(unsigned char* buf, int size) {
    int ret = 0;
    std::mutex mtx;
    std::unique_lock<std::mutex> lk(mtx);
    int lsize = size;
    int offset = 0;
    const int splitSize = 3 * m_BufferCapacity / m_thdNum;

    if (!m_useAsync) {
        ret = avio_read_partial(m_avio, buf, size);
        if (ret > 3 * m_BufferCapacity - m_readSize) {
            m_curIdx = 0;
            m_reader[m_curIdx]->m_bufferUsed = ret - (3 * m_BufferCapacity - m_readSize);
            m_useAsync = true;
            av_log(m_avio, AV_LOG_WARNING, "---zzz---last read size: %d, async reader will be used, m_BufferCapacity: %d, m_readSize: %d\n", size, m_BufferCapacity, m_readSize);
            m_reader[m_thdNum - 1]->m_bufferReady.store(false);
            m_reader[m_thdNum - 1]->m_cvRead.notify_one();
            av_log(m_avio, AV_LOG_WARNING, "\n---zzz---start Async Reader0[%d]\n", m_thdNum - 1);
            av_log(m_avio, AV_LOG_WARNING, "---zzz---ret: %d m_BufferCapacity: %d, m_readSize: %d\n", ret, m_BufferCapacity, m_readSize);
            return ret;
        }
        //av_log(m_avio, AV_LOG_WARNING, "---zzz---last read ret: %d\n", ret);
        if (ret > 0)
            m_readSize += ret;

        if (m_readSize > ((m_activeId + 1) * splitSize)) {
            m_reader[m_activeId]->m_bufferReady.store(false);
            m_reader[m_activeId]->m_cvRead.notify_one();
            av_log(m_avio, AV_LOG_WARNING, "\n---zzz---start Async Reader0[%d]\n", m_activeId);
            av_log(m_avio, AV_LOG_WARNING, "---zzz---ret: %d m_BufferCapacity: %d, m_readSize: %d\n", ret, m_BufferCapacity, m_readSize);
            m_activeId++;
        }

        return (ret <= 0) ? -1 : ret;
    } else {
        //av_log(m_avio, AV_LOG_WARNING, "---zzz--- reading from async buffer[%d]\n", m_curIdx);
        while (!m_shutdown.load()) {
            if (m_reader[m_curIdx]->m_bufferReady.load()) {
                if (!(lsize > m_reader[m_curIdx]->m_bufferSize - m_reader[m_curIdx]->m_bufferUsed)) {
                    memcpy(buf + offset, m_reader[m_curIdx]->m_buffer + m_reader[m_curIdx]->m_bufferUsed, lsize);
                    m_reader[m_curIdx]->m_bufferUsed += lsize;
                    ret += lsize;
                    if (m_reader[m_curIdx]->m_bufferUsed == m_reader[m_curIdx]->m_bufferSize) {
                        av_log(m_reader[m_curIdx]->m_avio, AV_LOG_WARNING,"---zzz---[%s] Read buffer[%d] exhausted_1----\n", current_time().c_str(), m_curIdx);
                        m_reader[m_curIdx]->m_bufferReady.store(false);
                        m_reader[m_curIdx]->m_cvRead.notify_one();
                        m_curIdx = (m_curIdx + 1) % m_thdNum;
                    }
                    break;
                } else {
                    int rsize = m_reader[m_curIdx]->m_bufferSize - m_reader[m_curIdx]->m_bufferUsed;
                    memcpy(buf + offset, m_reader[m_curIdx]->m_buffer + m_reader[m_curIdx]->m_bufferUsed, rsize);
                    offset += rsize;
                    ret += rsize;
                    lsize -= rsize;
                    if (m_reader[m_curIdx]->m_eof.load()) {
                        break;
                    } else {
                        av_log(m_reader[m_curIdx]->m_avio, AV_LOG_WARNING,"---zzz---[%s] Read buffer[%d] exhausted_0----\n", current_time().c_str(), m_curIdx);
                        m_reader[m_curIdx]->m_bufferReady.store(false);
                        m_reader[m_curIdx]->m_cvRead.notify_one();
                        m_curIdx = (m_curIdx + 1) % m_thdNum;
                    }
                }
            } else {
                if (m_reader[m_curIdx]->m_eof.load()) {
                    break;
                }
                m_cvUser.wait(lk);
            }
        }
    }

    return ret;
}

int MConn::Seek(int64_t pos) {
    int ret = 0;
    m_newPosition = pos + 3 * m_BufferCapacity;
    for (int i=0; i<m_thdNum; i++) {
        m_reader[i]->m_seek.store(true);
        m_reader[i]->m_bufferReady.store(true);
    }
    m_useAsync = false;
    m_notifySeek = true;
    m_readSize = 0;
    m_activeId = 0;

    if (avio_seek(m_avio, pos, SEEK_SET) < 0)
        ret = 0;
    else
        ret = 1;
    av_log(m_avio, AV_LOG_WARNING, "---zzz---[%s] seek to %lld ret: %d, m_newPosition: %lld\n", current_time().c_str(), pos, ret, m_newPosition);
    return ret;
}

int64_t MConn::Size() {
    return avio_size(m_avio);
}

int MConn::Close() {
    m_shutdown = true;
    for (int i=0; i<m_thdNum; i++) {
        av_log(m_reader[i]->m_avio, AV_LOG_WARNING, "---zzz---Close reader[%d] \n", i);
        delete m_reader[i];
    }
    return 0;
}

void* mconn_wrap_init(int num) {
    MConn* pMC = new MConn();
    pMC->Init(num);
    return (void*)pMC;
}

int mconn_wrap_open(void* mc, const char* url) {
    MConn* pMC = (MConn*)mc;
    return pMC->Open(url);
}

int mconn_wrap_read(void* mc, unsigned char* buf, int size) {
    MConn* pMC = (MConn*)mc;
    return pMC->Read(buf, size);
}

int mconn_wrap_seek(void* mc, int64_t pos) {
    MConn* pMC = (MConn*)mc;
    return pMC->Seek(pos);
}

int64_t mconn_wrap_size(void* mc) {
    MConn* pMC = (MConn*)mc;
    return pMC->Size();
}

void mconn_wrap_close(void* mc) {
    MConn* pMC = (MConn*)mc;
    pMC->Close();
    delete (MConn*)pMC;
}