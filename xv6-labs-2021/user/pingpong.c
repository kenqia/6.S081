#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    int pipe1[2]; //parent -> chird
    int pipe2[2]; //chird -> parent
    if(pipe(pipe1) || pipe(pipe2)){
        fprintf(2, "pipe error\n");
        exit(1);
    }
    int pid;
    char buf[1];
    if((pid = fork()) < 0){
        fprintf(2, "fork error\n");
        exit(2);
    }else if(pid == 0){
        close(pipe1[1]);
        close(pipe2[0]);
        read(pipe1[0], buf, 1);
        fprintf(1, "%d: received ping\n", getpid());
        write(pipe2[1], buf, 1);
        close(pipe1[0]);
        close(pipe2[1]);
    }else{
        close(pipe1[0]);
        close(pipe2[1]);
        buf[0] = 'A';
        write(pipe1[1], buf, 1);
        read(pipe2[0], buf, 1);
        fprintf(1, "%d: received pong\n", getpid());
        close(pipe1[1]);
        close(pipe2[0]);
    }
    exit(0);
}