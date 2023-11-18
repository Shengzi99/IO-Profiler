#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include "defs.h"
#include "fInfo.h"
using namespace std;

uint64_t trace_id = 0;
FILE *trace_fp = NULL;
map<string, fInfo> all_fInfo;
fInfo *fd2fI[1024] = {NULL};

int open(const char *pathname, int flags, ...){
    va_list valist;
    va_start(valist, flags);

    ssize_t (*real_open)(const char *, int, mode_t);
    real_open = (ssize_t(*)(const char *, int, mode_t))dlsym(RTLD_NEXT, "open");
    int __fd = real_open(pathname, flags, va_arg(valist, int));

    if (all_fInfo.count(pathname) == 0) 
        all_fInfo[pathname] = fInfo{pathname, 0, 0, 0, 0, .0, .0};

    fd2fI[__fd] = &(all_fInfo[pathname]);
    // printf("open %d:%p\n", __fd, fd2fI[__fd]);
    return __fd;
}

int close(int __fd){
    int (*real_close)(int);

    real_close = (int (*)(int))dlsym(RTLD_NEXT, "close");

    int ret = real_close(__fd);

    fd2fI[__fd]=NULL;
    return ret;
}

ssize_t read(int __fd, void *__buf, size_t __nbytes){
    ssize_t (*real_read)(int, void *, size_t);
    real_read = (ssize_t(*)(int, void *, size_t))(ssize_t)dlsym(RTLD_NEXT, "read");

    timeval start, end;
    gettimeofday(&start, NULL);
    int rv = real_read(__fd, __buf, __nbytes);
    gettimeofday(&end, NULL);

    fd2fI[__fd]->rd_cnt++;
    fd2fI[__fd]->rd_size+=__nbytes;
    fd2fI[__fd]->rd_time+=TV2INTERVAL(start, end);

    fprintf(trace_fp, "%10lld | %6d | %20s | %8s | %10d \n", trace_id++, __fd, fd2fI[__fd]->fname.c_str(), "read", __nbytes);

    return rv;
}

ssize_t write(int __fd, const void *buf, size_t count){
    ssize_t (*real_write)(int, const void *, size_t);
    real_write = (ssize_t(*)(int, const void *, size_t))dlsym(RTLD_NEXT, "write");

    timeval start, end;
    gettimeofday(&start, NULL);
    int rv = real_write(__fd, buf, count);
    gettimeofday(&end, NULL);
    // printf("write %d:%p\n", __fd, fd2fI[__fd]);
    fd2fI[__fd]->wt_cnt++;
    fd2fI[__fd]->wt_size+=count;
    fd2fI[__fd]->wt_time+=TV2INTERVAL(start, end);
    fprintf(trace_fp, "%10lld | %6d | %20s | %8s | %10d \n", trace_id++, __fd, fd2fI[__fd]->fname.c_str(), "write", count);

    return rv;
}

__attribute__((constructor)) void before_main(void){
    all_fInfo.clear();
    trace_fp = fopen("ioProf_trace.out", "w");

    fprintf(trace_fp, "%10s | %-6s | %-20s | %-8s | %-12s \n", "Trace ID", "fd", "File Name", "Type", "Size(Bytes)");
    fprintf(trace_fp, "%10s-|-%-6s-|-%-20s-|-%-8s-|-%-12s \n", "----------", "------", "--------------------", "--------", "------------");

}

bool cmp(const fInfo &a, const fInfo &b){
    return (a.rd_size+a.wt_size)>(b.rd_size+b.wt_size);
}
__attribute__((destructor)) void after_main(void){
    fflush(trace_fp);
    fclose(trace_fp);
    printf("IOProf trace saved!\n");

    vector<fInfo> ordered_fInfo;
    for(auto it:all_fInfo) ordered_fInfo.push_back(it.second);

    sort(ordered_fInfo.begin(), ordered_fInfo.end(), cmp);

    FILE *profile_fp;
    profile_fp = fopen("ioProf_profile.out", "w");
    fprintf(profile_fp, "%8s | %-20s | %-10s | %-10s | %-10s | %-10s | %-10s | %-10s \n", "Rank", "File Name", "Rd Count", "Rd Size(B)", "Rd Time(s)", "Wt Count", "Wt Size(B)", "Wt Time(s)");
    fprintf(profile_fp, "%8s | %-20s | %-10s | %-10s | %-10s | %-10s | %-10s | %-10s \n", "--------", "--------------------", "--------", "--------", "----------", "--------", "--------", "----------");
    for(int i=0;i<ordered_fInfo.size();i++)
        fprintf(profile_fp, "%8d | %-20s | %-10d | %-10d | %-10lf | %-10d | %-10d | %-10lf \n", i, ordered_fInfo[i].fname.c_str(), 
                                                                                            ordered_fInfo[i].rd_cnt, ordered_fInfo[i].rd_size, ordered_fInfo[i].rd_time,
                                                                                            ordered_fInfo[i].wt_cnt, ordered_fInfo[i].wt_size, ordered_fInfo[i].wt_time);
    fclose(profile_fp);
    printf("IOProf profile saved!\n");
}