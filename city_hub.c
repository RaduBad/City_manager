#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#define PID_FILE   ".monitor_pid"
#define HUB_MON_PID_FILE ".hub_mon_pid"
#define LINE_MAX_HUB 1024
#define MAX_DISTRICTS 64

static ssize_t read_line(int fd, char *buf, size_t maxlen) {
    size_t i = 0;
    char c;
    ssize_t r;
    while (i + 1 < maxlen) {
        r = read(fd, &c, 1);
        if (r < 0) return -1;
        if (r == 0) break;          
        buf[i++] = c;
        if (c == '\n') break;
    }
    buf[i] = '\0';
    return (ssize_t)i;
}

static void run_hub_mon() {

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("hub_mon: pipe");
        exit(EXIT_FAILURE);
    }

    char fd_str[16];
    snprintf(fd_str, sizeof(fd_str), "%d", pipefd[1]);  

    pid_t mon_pid = fork();
    if (mon_pid < 0) {
        perror("hub_mon: fork monitor");
        close(pipefd[0]); close(pipefd[1]);
        exit(EXIT_FAILURE);
    }

    if (mon_pid == 0) {
        close(pipefd[0]);
        execvp("./monitor_reports",
               (char *[]){ "./monitor_reports", "-p", fd_str, NULL }); 
        perror("hub_mon: execvp monitor_reports");
        exit(EXIT_FAILURE);
    }

    close(pipefd[1]);

    printf("[hub_mon] monitor_reports launched (PID %d)\n", (int)mon_pid);
    fflush(stdout);

    char line[LINE_MAX_HUB];
    int monitor_done = 0;

    while (!monitor_done) {
        ssize_t n = read_line(pipefd[0], line, sizeof(line));
        if (n <= 0) {
            printf("[hub_mon] Monitor pipe closed unexpectedly.\n");
            fflush(stdout);
            break;
        }
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        char *sep = strchr(line, '|');
        const char *tag  = line;
        const char *text = "";
        if (sep) {
            *sep = '\0';
            text = sep + 1;
        }

        if (strcmp(tag, "ERR") == 0) {
            printf("[hub_mon] ERROR from monitor: %s\n", text);
            printf("[hub_mon] Monitor has ended (error).\n");
            fflush(stdout);
            monitor_done = 1;
        } else if (strcmp(tag, "END") == 0) {
            printf("[hub_mon] %s\n", text);
            printf("[hub_mon] Monitor has ended (normal shutdown).\n");
            fflush(stdout);
            monitor_done = 1;
        } else {

            printf("[hub_mon] %s\n", text);
            fflush(stdout);
        }
    }

    close(pipefd[0]);

    int status;
    waitpid(mon_pid, &status, 0);
    exit(EXIT_SUCCESS);
}

static void cmd_start_monitor() {
    
    {
        int fd = open(HUB_MON_PID_FILE, O_RDONLY);
        if (fd >= 0) {
            char buf[32] = {0};
            read(fd, buf, sizeof(buf) - 1);
            close(fd);
            pid_t hm_pid = (pid_t)atol(buf);
            if (hm_pid > 0 && kill(hm_pid, 0) == 0) {
                printf("[hub] hub_mon is already running (PID %d).\n",
                       (int)hm_pid);
                return;
            }
        }
    }

    pid_t hub_mon_pid = fork();
    if (hub_mon_pid < 0) {
        perror("hub: fork hub_mon");
        return;
    }

    if (hub_mon_pid == 0) {
        
        setsid();
        run_hub_mon();   
        exit(EXIT_FAILURE);
    }
    int fd = open(HUB_MON_PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        char buf[32];
        int len = snprintf(buf, sizeof(buf), "%d\n", (int)hub_mon_pid);
        write(fd, buf, len);
        close(fd);
    }

    printf("[hub] hub_mon started (PID %d). Monitor output will appear above.\n",
           (int)hub_mon_pid);
}

static void cmd_stop_monitor(void) {
    int fd = open(PID_FILE, O_RDONLY);
    if (fd < 0) {
        printf("[hub] No monitor PID file found – monitor may not be running.\n");
        return;
    }
    char buf[32] = {0};
    read(fd, buf, sizeof(buf) - 1);
    close(fd);
    pid_t mon_pid = (pid_t)atol(buf);
    if (mon_pid <= 0) {
        printf("[hub] Invalid PID in %s.\n", PID_FILE);
        return;
    }
    if (kill(mon_pid, SIGINT) == 0)
        printf("[hub] Sent SIGINT to monitor (PID %d).\n", (int)mon_pid);
    else
        perror("[hub] kill SIGINT");
}


static void cmd_calculate_scores(char **districts, int ndist) {
    if (ndist == 0) {
        printf("[hub] Usage: calculate_scores <district1> [district2 ...]\n");
        return;
    }

    for (int d = 0; d < ndist; d++) {
        const char *district = districts[d];

        int pipefd[2];
        if (pipe(pipefd) < 0) {
            perror("hub: pipe");
            continue;
        }

        pid_t scorer_pid = fork();
        if (scorer_pid < 0) {
            perror("hub: fork scorer");
            close(pipefd[0]); close(pipefd[1]);
            continue;
        }

        if (scorer_pid == 0) {
          
            close(pipefd[0]);
            
            if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
                perror("scorer: dup2");
                exit(EXIT_FAILURE);
            }
            close(pipefd[1]);
            execvp("./scores",
                   (char *[]){ "./scores", (char *)district, NULL });
            perror("hub: execvp scorer");
            exit(EXIT_FAILURE);
        }

        close(pipefd[1]);

        printf("District: %s \n", district);

        char line[LINE_MAX_HUB];
        int  report_count = 0;

        while (1) {
            ssize_t n = read_line(pipefd[0], line, sizeof(line));
            if (n <= 0) break;


            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

            if (strncmp(line, "INSPECTOR|", 10) == 0) {
                
                char name[64] = {0};
                int  score = 0, rcount = 0;
                
                char *tok = strtok(line + 10, "|");
                if (tok) { strncpy(name, tok, 63); tok = strtok(NULL, "|"); }
                if (tok && strcmp(tok, "SCORE") == 0) {
                    tok = strtok(NULL, "|");
                    if (tok) score = atoi(tok);
                }
                
                tok = strtok(NULL, "|"); 
                tok = strtok(NULL, "|"); 
                tok = strtok(NULL, "|"); 
                if (tok && strcmp(tok, "REPORTS") == 0) {
                    tok = strtok(NULL, "|");
                    if (tok) rcount = atoi(tok);
                }
                printf("  Inspector: %-20s  Score: %3d  (Reports: %d)\n",
                       name, score, rcount);
                report_count++;

            } else if (strncmp(line, "SUMMARY|", 8) == 0) {
               
                int total = 0;
                char *tok = strtok(line + 8, "|"); 
                tok = strtok(NULL, "|");            
                tok = strtok(NULL, "|");            
                if (tok) total = atoi(tok);
                printf("  ── Total reports in district: %d\n", total);
            }
        }

        close(pipefd[0]);

        int status;
        waitpid(scorer_pid, &status, 0);

        if (report_count == 0)
            printf("  (no data or district not found)\n");
        printf("\n");
    }

}

int main(void) {

    char line[LINE_MAX_HUB];

    while (1) {
        printf("city_hub> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n[hub] EOF – exiting.\n");
            break;
        }

        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';


        char *tokens[MAX_DISTRICTS + 4];
        int   ntok = 0;
        char *tok = strtok(line, " \t");
        while (tok && ntok < (int)(sizeof(tokens) / sizeof(tokens[0]) - 1)) {
            tokens[ntok++] = tok;
            tok = strtok(NULL, " \t");
        }
        if (ntok == 0) continue;

        const char *cmd = tokens[0];

        if (strcmp(cmd, "start_monitor") == 0) {
            cmd_start_monitor();

        } else if (strcmp(cmd, "stop_monitor") == 0) {
            cmd_stop_monitor();

        } else if (strcmp(cmd, "calculate_scores") == 0) {
            cmd_calculate_scores(&tokens[1], ntok - 1);

        } else if (strcmp(cmd, "quit") == 0 ||
                   strcmp(cmd, "exit") == 0) {
            printf("[hub] Goodbye.\n");
            break;

        } else {
            printf("[hub] Unknown command: '%s'\n", cmd);
            printf("      Available: start_monitor, stop_monitor, "
                   "calculate_scores <districts...>, quit\n");
        }
    }


    unlink(HUB_MON_PID_FILE);
    return 0;
}