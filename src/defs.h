#include<limits.h>
#define TV2INTERVAL(ST, ED) ((double)((ED).tv_sec-(ED).tv_sec)+((ED).tv_usec-(ST).tv_usec)/1000000.0)

// #define MAX_FD_NUM 1024
// #define MAX_FNAME_LEN 