#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXCAPACITY 2048
int main(int argc, char *argv[]){
    char buf[MAXCAPACITY], *Xargv[MAXARG];
    int left = 0 , right, n;

    if(argc < 2){
        fprintf(2, "Usage: xargs <execprogram> <arguments>");
        exit(1);
    }
    // printf("buf : %s\n", buf);
    for(int i = 1 ; i < argc ; i++){
        Xargv[i - 1] = argv[i];
    }
    // for(int i = 0 ; i <= argc ; i++){
    //     printf("Xargv1 : %s\n", Xargv[i]);
    // }
    while((n = read(0, buf, MAXCAPACITY)) > 0){
        left = 0;
        while(left < n){
            int pid;
            right = left;
            for( ;right < n && buf[right] != '\n'; right++);
            buf[right] = '\0';
            Xargv[argc - 1] = buf + left;
            Xargv[argc] = 0;
            if((pid = fork()) < 0){
                fprintf(2, "fork error");
                exit(2);
            }else if(pid == 0){
                // for(int i = 0 ; i <= argc ; i++){
                //     printf("Xargv2 : %s\n", Xargv[i]);
                // }
                exec(argv[1], Xargv);
                exit(0);
            }else{
                wait(0);
            }
            left = right + 1;
        }
    }
    exit(0);
}