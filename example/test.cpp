/* testIO.cpp */
#include <cstdlib>
#include <cstdio>

#include <string.h>// strerror()
#include <unistd.h>// write(), read(), close()
#include <sys/types.h>// open()
#include <sys/stat.h>// open()
#include <fcntl.h>// open()
#include <errno.h>// errno, strerror()

#include <cmath>// rand()
#include <ctime>// time()
#include <sys/time.h>// gettimeofday()
#include <vector>
#include <unordered_map>
using namespace std;

#define MAX_FILES 1000

#define MAX_LEN 4096
char content[MAX_LEN];
char buf[MAX_LEN];

void init_content() {
    printf("size of content: %d bytes\n", sizeof(content));
    for (int i = 0; i < sizeof(content); i++) {
        int j = (i + 33 > 126) ? ((i+33)%126 + 33) : (i + 33);
        content[i] = (char)j; 
    }
}

double wall_time() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return 1.*t.tv_sec + 1.e-6*t.tv_usec;
}

typedef enum {OPEN = -1, READ, WRITE, CLOSE, IO_TYPE_CNT} IO_TYPE;

#define ERROR_OPEN(idx,fd)  {printf("file idx %d of fd %d open failed: %s\n", idx, fd, strerror(errno));  return -1;}
#define ERROR_READ(idx,fd)  {printf("read file ids %d of fd %d failed: %s\n", idx, fd, strerror(errno));  return -1;}
#define ERROR_WRITE(idx,fd) {printf("write file ids %d of fd %d failed: %s\n", idx, fd, strerror(errno)); return -1;}

int main()
{
    srand(time(0));
    int num_files = rand() % MAX_FILES + 1;// 随机决定进程最多拥有多少个文件号
    printf("num files: %d\n", num_files);

    init_content();// 初始化一段字符数组，之后反复用这里面的内容来写

    unordered_map<int, int> idx_to_fd;

    double startT = wall_time(), currT;
    while ((currT = wall_time()) < startT + 2.0) {// 让程序运行够2秒
        int idx = rand() % num_files;// 随机选取一个文件号
        int rv;// return value of system calls
        if (idx_to_fd.find(idx) == idx_to_fd.end()) {// 之前还没创建过这个文件
            char filename[256];
            char randstr[7]={0}; int idx_cpy=idx;
            for(int i=0;i<6;i++) randstr[i]=(char)((idx_cpy*=7)%55+65);
            sprintf(filename, "./tmp/%s-%d.txt", randstr,idx);// 该文件号对应的文件名
            int fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666);
            if (fd < 0) ERROR_OPEN(idx, fd);

            idx_to_fd[idx] = fd;// 该文件对应的描述符
            // 创建这个文件的同时就写入一些东西
            if ((rv = write(fd, content, MAX_LEN)) < 0) ERROR_WRITE(idx, fd);
        } else {
            int fd = idx_to_fd[idx];
            if (fd == -1) continue;// 之前已经关掉过这个文件，就不再用它了

            int type = rand() % IO_TYPE_CNT;// 随机决定读写类型
            if (type == READ) {
                int file_len = lseek(fd, 0, SEEK_END);// 返回值就是文件指针距离文件开头的偏移量，也就是文件的长度
                int offset = rand() % file_len;// 随机决定读的位置
                lseek(fd, offset, SEEK_SET);

                int count = rand() % MAX_LEN;// 随机决定读的量
                if ((rv = read(fd, buf, count)) < 0) ERROR_READ(idx, fd);

                lseek(fd, 0, SEEK_END);// 读完之后重新将文件指针恢复到末尾
            } else if (type == WRITE) {
                int count = rand() % MAX_LEN;
                if ((rv = write(fd, content, count)) < 0) ERROR_WRITE(idx, fd);
            } else {// CLOSE
                close(fd);
                idx_to_fd[idx] = -1;// 标记为已关闭
            }
        }
    }

    // 关掉所有仍活跃的fd
    for (unordered_map<int,int>::iterator it = idx_to_fd.begin(); it != idx_to_fd.end(); it++) {
        if (it->second != -1) {
            close(it->second);
        }
    }

    return 0;
}
