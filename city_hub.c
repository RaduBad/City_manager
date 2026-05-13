#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>


int main(int argc , char **argv){

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    return 0;
}