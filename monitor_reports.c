#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
 
#define PID_FILE ".monitor_pid"
 
static volatile sig_atomic_t got_sigint  = 0;
static volatile sig_atomic_t got_sigusr1 = 0;
 
static void handle_sigint(int sig) {
    (void)sig;
    got_sigint = 1;
}
 
static void handle_sigusr1(int sig) {
    (void)sig;
    got_sigusr1++;
}
 
static int write_pid_file(void) {
    int fd = open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open " PID_FILE);
        return -1;
    }
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%d\n", (int)getpid());
    if (write(fd, buf, len) != len) {
        perror("write " PID_FILE);
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}
 
static void delete_pid_file(void) {
    if (unlink(PID_FILE) != 0 && errno != ENOENT)
        perror("unlink " PID_FILE);
}
 

int main(void) {

    if (write_pid_file() != 0) {
        fprintf(stderr, "monitor_reports: failed to write PID file – aborting.\n");
        return 1;
    }
    printf("monitor_reports started (PID %d). PID written to %s\n",
           (int)getpid(), PID_FILE);
    fflush(stdout);
 
    struct sigaction sa_int, sa_usr1;
    memset(&sa_int,  0, sizeof(sa_int));
    memset(&sa_usr1, 0, sizeof(sa_usr1));
 
    sa_int.sa_handler  = handle_sigint;
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_int.sa_mask);
    sigemptyset(&sa_usr1.sa_mask);
    sa_int.sa_flags  = 0;
    sa_usr1.sa_flags = SA_RESTART;
 
    if (sigaction(SIGINT,  &sa_int,  NULL) != 0) { perror("sigaction SIGINT");  goto cleanup; }
    if (sigaction(SIGUSR1, &sa_usr1, NULL) != 0) { perror("sigaction SIGUSR1"); goto cleanup; }
 
    while (!got_sigint) {
        pause(); 
 
        while (got_sigusr1 > 0) {
            got_sigusr1--;
            printf("[monitor] New report added – received SIGUSR1 notification.\n");
            fflush(stdout);
        }
    }
 
  
    printf("[monitor] SIGINT received – shutting down.\n");
    fflush(stdout);
 
cleanup:
    delete_pid_file();
    return 0;
}