#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>

#define ROUND 10000
#define SIZE 1

int main(){
    int pipe1[2]; // parent -> chird
    int pipe2[2]; // chird -> parent
    if(pipe(pipe1) || pipe(pipe2)){
        printf("pipe error");
        exit(1);
    }
    pid_t pid = fork();
    char buf[SIZE];
    struct timespec start, end;
    if(pid == -1){
        printf("fork error");
        exit(2);
    }else if(pid == 0){
        close(pipe1[1]);
        close(pipe2[0]);
        for(int i = 0 ; i < ROUND ; i++){
            read(pipe1[0], buf, SIZE);
            write(pipe2[1], buf, SIZE);
        }
        close(pipe1[0]);
        close(pipe2[1]);
        exit(0);
    }else{
        close(pipe1[0]);
        close(pipe2[1]);
        buf[0] = 'A';
        clock_gettime(CLOCK_MONOTONIC, &start);
        for(int i = 0 ; i < ROUND ; i++){
            write(pipe1[1], buf, SIZE);
            read(pipe2[0], buf, SIZE);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        close(pipe1[1]);
        close(pipe2[0]);
    }
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Elapsed time: %.9f seconds\n", elapsed);
    printf("run %f times ping-pong every sec\n", ROUND / elapsed);
}