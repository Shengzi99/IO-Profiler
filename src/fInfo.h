#include<stdlib.h>
#include<stdint.h>
#include<string>
#include "defs.h"

struct fInfo{
    std::string fname;
    
    size_t rd_cnt;
    size_t wt_cnt;

    size_t rd_size;
    size_t wt_size;

    double rd_time;
    double wt_time;
};