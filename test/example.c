#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mach/mach_time.h>
#include "libmconn.h"

int main(void) {
    const char* url =
        //"http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerEscapes.mp4"
        //"http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4"
        //"http://192.168.123.11:5244/d/aliyun/Movie/BigBuckBunny.mp4?sign=ae6gCGl0aiRetMNC3B7gJOCbFdYqIuZjZfAhhj9p1Kg=:0"
        //"https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/WeAreGoingOnBullrun.mp4"
        //"https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerFun.mp4"
        "http://192.168.123.11:5244/d/aliyun/Movie/ForBiggerFun.mp4?sign=16JZXNlkOtX7jcdYRDCPqY-mEBVSTxVklEaxi1EVbK0=:0"
        //"http://192.168.123.11:5244/d/aliyun/Movie/Friends%EF%BC%88%E5%85%A8%EF%BC%89/S01/S01E01.mp4?sign=u2FbTpIMlTIusgi5NEFHql9hIYlvziZIyUmxagYBOxg=:0"
        ;
    int ret = 0;
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);

    void* mc = mconn_init(5);
    mconn_open(mc, url);
    FILE* fp = fopen("output.mp4", "wb");
    unsigned int buf_size = 1024 * 16;
    int total = 0;
    unsigned char* buf = (unsigned char*)malloc(buf_size);
    uint64_t start = mach_absolute_time();
    while(1) {
        ret = mconn_read(mc, buf, buf_size);
        if (ret > 0) {
            total += ret;
            printf("mconn_read ret: %d, total: %.4fMB\n", ret, (double)total / 1048576);
            fwrite(buf, buf_size, sizeof(unsigned char), fp);
            struct timespec req = {0};
            req.tv_sec = 0;          // Seconds
            req.tv_nsec = 20000000; // Nanoseconds (0.1 seconds = 100,000,000 ns)

            if (nanosleep(&req, NULL) < 0) {
                perror("nanosleep failed");
                return 1;
            }
        }
        if (ret != buf_size)
            break;
    }
    uint64_t end = mach_absolute_time();
    double duration = (double)(end - start) * timebase.numer / timebase.denom / 1e9;
    double fileSize = (double)total/1024;
    double speed = fileSize / duration;
    printf("Time used: %.2fs, File size: %.2fMB, speed: %.2fKB/s\n", duration, fileSize/1024, speed);
    free(buf);
    fclose(fp);
    mconn_close(mc);
}