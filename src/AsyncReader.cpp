#include "AsyncReader.h"
#include "MConn.h"

AsyncReader::AsyncReader(int id, MConn *mc, bool ready, int size) {
    m_id = id;
    m_mc = mc;
    m_bufferReady.store(true);
    m_bufferSize = 0;
    m_close.store(false);
    m_thd = std::thread([this](){ ThreadWork(); });
}

AsyncReader::~AsyncReader() {
    m_close.store(true);
    m_cvRead.notify_one();
    if (m_thd.joinable())
        m_thd.join();
    free(m_buffer);
}

void AsyncReader::ThreadWork() {
    std::mutex mtx;
    std::unique_lock<std::mutex> lk(mtx);
    bool skip = false;
    int mtime = 500;
    while(!m_close.load()) {
        //av_log(NULL, AV_LOG_ERROR, "---zzz---[%s] AsyncReader[%d] bufferReady %d\n", current_time().c_str(), m_id, m_reader[m_id]->m_bufferReady.load());
        if (!m_bufferReady.load()) {
            if (m_seek.load()) {
                int64_t seekPos = m_mc->m_newPosition + (m_id == 0 ? m_mc->m_thdNum : m_id) * m_mc->m_BufferCapacity;
                av_log(m_avio, AV_LOG_ERROR, "---zzz---[%s] AsyncReader[%d] execute seek to %lld\n", current_time().c_str(), m_id, seekPos);
                avio_seek(m_avio, seekPos, SEEK_SET);
                skip = false;
                m_seek.store(false);
                continue;
            }

            if (skip) {
                if (m_close.load())
                    break;
                if (avio_skip(m_avio, (m_mc->m_thdNum - 1) * m_mc->m_BufferCapacity) < 0) {
                    av_log(m_avio, AV_LOG_ERROR, "---zzz---[%s] AsyncReader[%d] seek failed, sleep_for: %d\n", current_time().c_str(), m_id, mtime);
                    std::this_thread::sleep_for(std::chrono::milliseconds(mtime));
                    if (mtime < 3000)
                        mtime += 500;
                    continue;
                }
                else {
                    av_log(m_avio, AV_LOG_ERROR, "---zzz---[%s] AsyncReader[%d] seek success\n", current_time().c_str(), m_id);
                }
            }
            mtime = 500;
            //const int DataSize = !skip && !m_id ? 1024 * 1024 * 3 : m_BufferCapacity;
            const int DataSize = m_mc->m_BufferCapacity;
            int size = avio_read(m_avio, m_buffer, DataSize);
            skip = true;

            if (m_close.load())
                break;

            if (size > 0) {
                av_log(m_avio, AV_LOG_WARNING, "---zzz---[%s] AsyncReader[%d] buffer full ret: %d----\n", current_time().c_str(), m_id, size);
                m_bufferUsed = 0;
                m_bufferSize = size;
                m_bufferReady.store(true);
                m_mc->Notify();
                if (size < DataSize) {
                    av_log(m_avio, AV_LOG_WARNING, "---zzz---[%s] AsyncReader[%d] reach EOF_0----\n", current_time().c_str(), m_id);
                    break;
                }
            } else {
                av_log(m_avio, AV_LOG_WARNING,"---zzz---[%s] AsyncReader[%d] reach EOF_1----\n", current_time().c_str(), m_id);
                break;
            }
        } else {
            m_cvRead.wait(lk);
        }
    }
    m_eof.store(true);
}

// bool Reader::isEOF() {
//     return m_eof.load();
// }