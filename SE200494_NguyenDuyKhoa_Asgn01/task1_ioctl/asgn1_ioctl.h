#ifndef ASGN1_IOCTL_H
#define ASGN1_IOCTL_H



#include <linux/ioctl.h>


//==================== khai bao struct tra ve thong tin driver khi goi ASGN1_GET_STATS ==========================
struct asgn1_stats {
    int open_count;
    int write_count;
    int read_count;
    int buffer_len;
    int last_write_size;
};
//====================================================================================


//==================== khai bao cac lenh ioctl cho driver ==========================
#define ASGN1_MAGIC 'K'             // 1 magic la 1 nhom, ta gom nhieu ioctl command vao 1 magic,
                                    // de de quan ly chu ko co gi dac biet, de biet cac commnad nay cung 1 nhom                               
#define ASGN1_VERSION_LEN 64
#define ASGN1_RESET_BUFFER _IO(ASGN1_MAGIC, 0)      // IO la lenh ko co tham so, command 0, magic la SE200494                   
#define ASGN1_GET_STATS _IOR(ASGN1_MAGIC, 1, struct asgn1_stats)   // IO read, command 1, magic la SE200494, tra ve struct asgn1_stats
#define ASGN1_SET_MODE _IOW(ASGN1_MAGIC, 2, int)  // IO write, command 2, magic la SE200494, dung de set mode cho driver, 0: kernel - 1 echo
#define ASGN1_GET_VERSION _IOR(ASGN1_MAGIC, 3, char[ASGN1_VERSION_LEN]) // IO read, command 3, magic la SE200494, tra ve version cua driver
//====================================================================================



#endif 
