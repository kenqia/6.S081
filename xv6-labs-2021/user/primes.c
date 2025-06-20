#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void sieve(int fd_read){
    int pid, p[2], tmp, buf;
    if(!read(fd_read, &tmp, sizeof(int))){
        exit(0);
    }
    fprintf(1, "prime %d\n", tmp);
    if(pipe(p)){
        fprintf(2, "pipe error");
        exit(1);
    }
    if((pid = fork()) < 0){
        fprintf(2, "fork error");
        exit(2);
    }else if(pid == 0){
        close(p[1]);
        sieve(p[0]);
        close(p[0]);
    }else{
        close(p[0]);
        while(read(fd_read, &buf, sizeof(int))){
            if(buf % tmp != 0){
                write(p[1], &buf, sizeof(int));
            }
        }
        close(p[1]);
        wait(0);
        exit(0);
    }
}

int main(int argc, char *argv[]){
    int data[35] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};
    int p[2], x;
    if(pipe(p)){
        fprintf(2, "pipe error");
        exit(1);
    }
        if((x = fork()) < 0){
            fprintf(2, "fork error");
            exit(2);
        }else if(x == 0){
            close(p[1]);
            sieve(p[0]);
            close(p[0]);
        }else{
            close(p[0]);
            write(p[1], data, 35 * sizeof(int));
            close(p[1]);
            wait(0);
        }

    exit(0);
}