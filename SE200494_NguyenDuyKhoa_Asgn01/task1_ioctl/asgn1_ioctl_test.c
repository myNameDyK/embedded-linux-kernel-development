#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "asgn1_ioctl.h"



#define CMD_RESET   0
#define CMD_STATS   1
#define CMD_MODE    2
#define CMD_VERSION 3



int main(int argc, char *argv[]) {        // sh truyen xuong nhung arg theo 1 vector argv, can argc de dem so luong arg

    int fd;

    if(argc < 2) {
        printf( " need at least 1 arg more after: %s + reset|stats|mode|version \n", argv[0] ); // argv[0] = ./asgn1_ioctl_test, yeu cau thuc thi file nay                                                                                     
        return 1;                                                                               // can them it nhat 1 arg chon cac lenh trong 4 lenh kia
    }

    fd = open("/dev/asgn1", O_RDWR);
    if (fd<0) {
        perror("OPEN");
        return 1;
    }




    int user_cmd = -1;

    if (strcmp(argv[1], "reset") == 0)          user_cmd = CMD_RESET;
    else if (strcmp(argv[1], "stats") == 0)     user_cmd = CMD_STATS;
    else if (strcmp(argv[1], "mode") == 0)      user_cmd = CMD_MODE;
    else if (strcmp(argv[1], "version") == 0)   user_cmd = CMD_VERSION;


    switch(user_cmd) {
        case CMD_RESET: {
            if( ioctl(fd, ASGN1_RESET_BUFFER) < 0 ){      
                perror("ioctl RESET BUFFER: ");
                close(fd);
                return 1;
            }
            else printf(" RESET: done\n");
            break;
        }   
        case CMD_STATS: {
            struct asgn1_stats stats;

            if( ioctl(fd, ASGN1_GET_STATS, &stats ) < 0){       
                perror("ioctl GET STATS: ");
                close(fd);
                return 1;
            }
            else {
                printf(" OPEN COUNT:        %d\n", stats.open_count);
                printf(" WRITE COUNT:       %d\n", stats.write_count);
                printf(" READ COUNT:        %d\n", stats.read_count );
                printf(" BUFFER LEN:        %d\n", stats.buffer_len);
                printf(" LAST WRITE SIZE:   %d\n", stats.last_write_size);
                printf(" GET_STATS: done\n");
            }
            break;           
        }
        case CMD_MODE: {
            int setmode = 0;
            if( argc ==3 && (strcmp(argv[2], "0") == 0 || strcmp(argv[2], "1") == 0)) {
                setmode = atoi(argv[2]);
                if( ioctl(fd, ASGN1_SET_MODE, &setmode) < 0 ){        // <0 = loi
                    perror("ioctl SET MODE: ");
                    close(fd);
                    return 1;
                }
                else printf(" SET MODE: done\n");
            }
            else {
                printf("need a correct mode 0|1\n");
                close(fd);
                return 1;
            }
            break;
        }   
        case CMD_VERSION: {
            char version[ASGN1_VERSION_LEN];
            memset(version, 0, sizeof(version));

            if( ioctl(fd, ASGN1_GET_VERSION, version) < 0){
                perror("ioctl GET VERSION: ");
                close(fd);
                return 1;
            }
            printf("VERSION: %s\n", version);
            break;
        }

        default:
            printf("Unknown command: %s\n", argv[1]);
            close(fd);
            return 1;
    }

    close(fd);
    return 0;

}