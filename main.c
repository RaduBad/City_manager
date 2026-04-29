#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>


#define SIZE 30
#define PATH_SIZE 512
typedef struct{
    float latitude;
    float longitude;
}GPS;

typedef struct{
    int ID;
    char inspector_name[SIZE];
    GPS coordinates;
    char issue_category[SIZE];
    int severity_level;
    time_t timestamp;
    char description[SIZE*5];
}REPORT;

typedef struct{
    char user[SIZE];
    char role[SIZE];

}USER;

void create_symlink(const char *district_id) {
    char link_name[PATH_SIZE];
    char target[PATH_SIZE];
    snprintf(link_name, sizeof(link_name), "active_reports-%s", district_id);
    snprintf(target,    sizeof(target),    "%s/reports.dat",     district_id);
 
    struct stat lst;
    if (lstat(link_name, &lst) == 0) 
        unlink(link_name);
 
    if (symlink(target, link_name) != 0)
        perror("symlink");
}
 
void check_symlinks() {
    DIR *d = opendir(".");
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, "active_reports-", 15) != 0) continue;
        struct stat lst, st;
        lstat(ent->d_name, &lst);
        if (S_ISLNK(lst.st_mode)) {
            if (stat(ent->d_name, &st) != 0)
                fprintf(stderr, "WARNING: dangling symlink '%s'\n", ent->d_name);
        }
    }
    closedir(d);
}


void log_action(const char *district_id, const char *role, char *user, char *action) {
    char path[PATH_SIZE];
    snprintf(path, sizeof(path), "%s/logged_district", district_id);
 
    if (strcmp(role, "inspector") == 0) {
        fprintf(stderr, "WARNING: inspector '%s' cannot write to log – skipping log entry.\n", user);
        return;
    }
 
    int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) { 
        perror("log open"); 
        return; 
    }
 
    char buf[PATH_SIZE];
    time_t now = time(NULL);
    char *ts = ctime(&now);
    ts[strlen(ts)-1] = '\0'; 
    int len = snprintf(buf, sizeof(buf), "[%s] role=%s user=%s action=%s\n", ts, role, user, action);
    write(fd, buf, len);
    close(fd);
}

int check_access(const char *path, char *role, int need_read, int need_write) {
    struct stat st;
    if (stat(path, &st) != 0) {
        perror(path);
        return 0;
    }
    mode_t mode = st.st_mode;
    int ok = 1;
    if (strcmp(role, "manager") == 0) {
        if (need_read  && !(mode & S_IRUSR)) ok = 0;
        if (need_write && !(mode & S_IWUSR)) ok = 0;
    } else { 
        if (need_read  && !(mode & S_IRGRP)) ok = 0;
        if (need_write && !(mode & S_IWGRP)) ok = 0;
    }
    if (!ok) {

        fprintf(stderr, "ERROR: role '%s' does not have required access to '%s' \n",role, path);
    }
    return ok;
}

void init_district(const char *district_id) {
    char path[PATH_SIZE];
 
    mkdir(district_id, 0750);
    chmod(district_id, 0750);

    snprintf(path, sizeof(path), "%s/reports.dat", district_id);
    int fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0664);
    if (fd >= 0) close(fd);
    chmod(path, 0664);

    snprintf(path, sizeof(path), "%s/district.cfg", district_id);
    if (access(path, F_OK) != 0) {
        fd = open(path, O_CREAT | O_WRONLY, 0640);
        if (fd >= 0) {
            const char *def = "threshold=1\n";
            write(fd, def, strlen(def));
            close(fd);
        }
    }
    chmod(path, 0640);
 
    snprintf(path, sizeof(path), "%s/logged_district", district_id);
    fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd >= 0) 
    close(fd);
    chmod(path, 0644);
 
    create_symlink(district_id);
}

void add(char* district_id , USER* u){
init_district(district_id);
 
    char path[PATH_SIZE];
    snprintf(path, sizeof(path), "%s/reports.dat", district_id);
 
   if (!check_access(path, u->role, 0, 1)) {
        return;
   }
 
    REPORT r;
    
    strncpy(r.inspector_name, u->user, SIZE - 1);
 
    printf("Report ID: ");
    if (scanf("%d", &r.ID) != 1) { 
        fprintf(stderr, "Invalid ID\n"); 
        return; 
    }
 
    printf("Latitude: ");
    if (scanf("%f", &r.coordinates.latitude) != 1) { 
        fprintf(stderr, "Invalid latitude\n"); 
        return; 
    }
 
    printf("Longitude: ");
    if (scanf("%f", &r.coordinates.longitude) != 1) { 
        fprintf(stderr, "Invalid longitude\n"); 
        return; 
    }
 
    printf("Issue category (road/lighting/flooding/...): ");
    if (scanf("%63s", r.issue_category) != 1) { 
        fprintf(stderr, "Invalid category\n"); 
        return; 
    }
 
    printf("Severity level (1=minor 2=moderate 3=critical): ");
    if (scanf("%d", &r.severity_level) != 1 ||
        r.severity_level < 1 || r.severity_level > 3) {
        fprintf(stderr, "Severity must be 1-3\n"); 
        return;
    }
 
    getchar(); 
    printf("Description: ");
    if (fgets(r.description, SIZE*5, stdin) == NULL) { 
        fprintf(stderr, "Invalid description\n"); 
        return; 
    }
 
    r.timestamp = time(NULL);
 
    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0) { 
        perror("reports.dat open"); 
        return; 
    }
    write(fd, &r, sizeof(REPORT));
    close(fd);
 
    chmod(path, 0664);
    printf("Report %d added to district '%s'.\n", r.ID, district_id);
 
    char action[PATH_SIZE];
    snprintf(action, sizeof(action), "add report_id=%d", r.ID);
    log_action(district_id, u->role, u->user, action);



}
 
void list(const char *district_id, USER *u) {
    char path[PATH_SIZE];
    snprintf(path, sizeof(path), "%s/reports.dat", district_id);
 
    if (!check_access(path, u->role, 1, 0)){ 
        return;
    }   
    struct stat st;
    if (stat(path, &st) != 0) { 
        perror(path); 
        return; }
 
    char *mtime = ctime(&st.st_mtime);
    mtime[strlen(mtime)-1] = '\0';
    printf("File: %s | Size: %lld bytes | Modified: %s\n\n",path , (long long)st.st_size, mtime);
 
    int fd = open(path, O_RDONLY);
    if (fd < 0) { 
        perror("reports.dat open"); 
        return; 
    }
 
    REPORT r;
    int count = 0;
    while (read(fd, &r, sizeof(REPORT)) == sizeof(REPORT)) {
        char *ts = ctime(&r.timestamp);
        ts[strlen(ts)-1] = '\0';
        printf("ID:%-4d | Inspector:%-20s | Category:%-12s | Severity:%d | %s\n",r.ID, r.inspector_name, r.issue_category, r.severity_level, ts);
        count++;
    }
    close(fd);
 
    if (count == 0) 
        printf("No reports found.\n");
    else 
        printf("\n%d report(s) total.\n", count);
 
    log_action(district_id, u->role, u->user, "list");
}

void view(const char *district_id, int report_id, USER *u) {
char path[PATH_SIZE];
snprintf(path, sizeof(path), "%s/reports.dat", district_id);

    if (!check_access(path, u->role, 1, 0)) 
        return;

int fd = open(path, O_RDONLY);
if (fd < 0) { 
    perror("reports.dat open"); 
    return; 
}

REPORT r;
int found = 0;
while (read(fd, &r, sizeof(REPORT)) == sizeof(REPORT)) {
if (r.ID == report_id) {
    found = 1;
    printf("─────────────────────────────────────\n");
    printf("Report ID : %d\n", r.ID);
    printf("Inspector : %s\n", r.inspector_name);
    printf("GPS : %.6f, %.6f\n", r.coordinates.latitude, r.coordinates.longitude);
    printf("Category : %s\n", r.issue_category);
    printf("Severity : %d\n", r.severity_level);
    printf("Timestamp : %s", ctime(&r.timestamp));
    printf("Description : %s\n", r.description);
    printf("─────────────────────────────────────\n");
    break;
}
}
close(fd);

if (!found) 
    fprintf(stderr, "Report %d not found in district '%s'.\n", report_id, district_id);

char action[64];
snprintf(action, sizeof(action), "view report_id=%d", report_id);
log_action(district_id, u->role, u->user, action);
}

void remove_report(const char *district_id, int report_id, USER *u) {
    if (strcmp(u->role, "manager") != 0) {
        fprintf(stderr, "ERROR: only managers can remove reports.\n");
        return;
    }
 
    char path[PATH_SIZE];
    snprintf(path, sizeof(path), "%s/reports.dat", district_id);
 
   if (!check_access(path, u->role, 1, 1)){ 
    return;
   }
 
    int fd = open(path, O_RDWR);
    if (fd < 0) { 
        perror("reports.dat open"); 
        return; 
    }
 
    struct stat st;
    fstat(fd, &st);
    int total = st.st_size / sizeof(REPORT); //nr de rapoarte
 
    REPORT *buf = malloc(total * sizeof(REPORT));
    if (!buf) { 
        fprintf(stderr, "malloc failed\n"); 
        close(fd); 
        return; 
    }
 
    int count = 0, found = 0;
    REPORT r;
    while (read(fd, &r, sizeof(REPORT)) == sizeof(REPORT)) {
        if (r.ID != report_id) 
            buf[count++] = r;
        else
            found = 1; 
    }
 
    if (!found) {
        fprintf(stderr, "Report %d not found.\n", report_id);
        free(buf); close(fd); return;
    }
 

    lseek(fd, 0, SEEK_SET);
    for (int i = 0; i < count; i++)
        write(fd, &buf[i], sizeof(REPORT));
    ftruncate(fd, count * sizeof(REPORT));
 
    free(buf);
    close(fd);
 
    printf("Report %d removed from district '%s'.\n", report_id, district_id);
 
    char action[64];
    snprintf(action, sizeof(action), "remove_report report_id=%d", report_id);
    log_action(district_id, u->role, u->user, action);
}

void update_threshold(const char *district_id, int value, USER *u) {
    if (strcmp(u->role, "manager") != 0) {
        fprintf(stderr, "ERROR: only managers can update the threshold.\n");
        return;
    }
 
    char path[PATH_SIZE];
    snprintf(path, sizeof(path), "%s/district.cfg", district_id);
 
   if (!check_access(path, u->role, 0, 1)){ 
    return;
   }
 
    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd < 0) { 
        perror("district.cfg open"); 
        return; 
    }
 
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "threshold=%d\n", value);
    write(fd, buf, len);
    close(fd);
 
    printf("Threshold for district '%s' set to %d.\n", district_id, value);
 
    char action[64];
    snprintf(action, sizeof(action), "update_threshold value=%d", value);
    log_action(district_id, u->role, u->user, action);
}

 
/*
 * parse_condition: splits "field:op:value" into three parts.
 * Returns 1 on success, 0 on malformed input.
 * (AI-generated function, reviewed and adapted — see ai_usage.md)
 */
int parse_condition(const char *input, char *field, char *op, char *value) {
    const char *p1 = strchr(input, ':');
    if (!p1) return 0;
 
    size_t flen = p1 - input;
    if (flen == 0 || flen >= SIZE) return 0;
    strncpy(field, input, flen);
    field[flen] = '\0';
 
    const char *p2 = strchr(p1 + 1, ':');
    if (!p2) return 0;
 
    size_t olen = p2 - (p1 + 1);
    if (olen == 0 || olen >= 4) return 0;
    strncpy(op, p1 + 1, olen);
    op[olen] = '\0';
 
    strncpy(value, p2 + 1, SIZE - 1);
    value[SIZE - 1] = '\0';
    if (strlen(value) == 0) return 0;
 
    return 1;
}
 
/*
 * match_condition: tests whether report r satisfies field op value.
 * Returns 1 if satisfied, 0 otherwise.
 * (AI-generated function, reviewed and adapted — see ai_usage.md)
 */
int match_condition(REPORT *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int v = atoi(value);
        int s = r->severity_level;
        if (strcmp(op,"==")== 0) return s == v;
        if (strcmp(op,"!=")== 0) return s != v;
        if (strcmp(op,"<") == 0) return s <  v;
        if (strcmp(op,"<=")== 0) return s <= v;
        if (strcmp(op,">") == 0) return s >  v;
        if (strcmp(op,">=")== 0) return s >= v;
    } else if (strcmp(field, "category") == 0) {
        int cmp = strcmp(r->issue_category, value);
        if (strcmp(op,"==")== 0) return cmp == 0;
        if (strcmp(op,"!=")== 0) return cmp != 0;
        if (strcmp(op,"<") == 0) return cmp <  0;
        if (strcmp(op,"<=")== 0) return cmp <= 0;
        if (strcmp(op,">") == 0) return cmp >  0;
        if (strcmp(op,">=")== 0) return cmp >= 0;
    } else if (strcmp(field, "inspector") == 0) {
        int cmp = strcmp(r->inspector_name, value);
        if (strcmp(op,"==")== 0) return cmp == 0;
        if (strcmp(op,"!=")== 0) return cmp != 0;
        if (strcmp(op,"<") == 0) return cmp <  0;
        if (strcmp(op,"<=")== 0) return cmp <= 0;
        if (strcmp(op,">") == 0) return cmp >  0;
        if (strcmp(op,">=")== 0) return cmp >= 0;
    } else if (strcmp(field, "timestamp") == 0) {
        time_t v = (time_t)atol(value);
        time_t t = r->timestamp;
        if (strcmp(op,"==")== 0) return t == v;
        if (strcmp(op,"!=")== 0) return t != v;
        if (strcmp(op,"<") == 0) return t <  v;
        if (strcmp(op,"<=")== 0) return t <= v;
        if (strcmp(op,">") == 0) return t >  v;
        if (strcmp(op,">=")== 0) return t >= v;
    }
    fprintf(stderr, "WARNING: unknown field '%s' or operator '%s'\n", field, op);
    return 0;
}
 
 
void filter(const char *district_id, USER *u, char **conditions, int ncond) {
    char path[PATH_SIZE];
    snprintf(path, sizeof(path), "%s/reports.dat", district_id);
 
    if (!check_access(path, u->role, 1, 0)) return;
 
    /* Parse all conditions upfront */
    char fields[16][SIZE], ops[16][4], values[16][SIZE];
    for (int i = 0; i < ncond; i++) {
        if (!parse_condition(conditions[i], fields[i], ops[i], values[i])) {
            fprintf(stderr, "ERROR: malformed condition '%s'\n", conditions[i]);
            return;
        }
    }
 
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("reports.dat open"); return; }
 
    REPORT r;
    int matched = 0;
    while (read(fd, &r, sizeof(REPORT)) == sizeof(REPORT)) {
        int ok = 1;
        for (int i = 0; i < ncond; i++) {
            if (!match_condition(&r, fields[i], ops[i], values[i])) { ok = 0; break; }
        }
        if (ok) {
            char *ts = ctime(&r.timestamp);
            ts[strlen(ts)-1] = '\0';
            printf("ID:%-4d | Inspector:%-20s | Category:%-12s | Severity:%d | %s | %s\n",
                   r.ID, r.inspector_name, r.issue_category, r.severity_level, ts, r.description);
            matched++;
        }
    }
    close(fd);
 
    printf("\n%d report(s) matched.\n", matched);
    log_action(district_id, u->role, u->user, "filter");
}

void remove_district(const char* district_id, USER* u){
    if (strcmp(u->role, "manager") != 0) {
        fprintf(stderr, "ERROR: only managers can remove reports.\n");
        return;
    }

    char* symlink="active_reports-";
    strcat(symlink,district_id);
    symlink[strlen(symlink)-1]='\0';
    pid_t p = fork();
    if(p == 0){

        execlp("rm", "rm", "-rf", district_id ,NULL);
        execlp("rm", "rm", "-rf", symlink ,NULL);
        exit(0);
    }


}

int main(int argc, char**argv){


    USER u;
    check_symlinks();
 
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc)
            strncpy(u.role, argv[++i], SIZE - 1);
        else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc)
            strncpy(u.user, argv[++i], SIZE - 1);
    }
 
    if (strlen(u.user) == 0) {
        fprintf(stderr, "ERROR: --user is required.\n");
        return 1;
    }
    if (strcmp(u.role, "manager") != 0 && strcmp(u.role, "inspector") != 0) {
        fprintf(stderr, "ERROR: --role must be 'manager' or 'inspector'.\n");
        return 1;
    }
 
    for (int i = 1; i < argc; i++) { 
        if (strcmp(argv[i], "--add") == 0 && i + 1 < argc) {
            add(argv[++i], &u);
 
        } else if (strcmp(argv[i], "--list") == 0 && i + 1 < argc) {
            list(argv[++i], &u);
 
        } else if (strcmp(argv[i], "--view") == 0 && i + 2 < argc) {
            const char *dist = argv[++i];
            int report_id = atoi(argv[++i]);
            view(dist, report_id, &u);
 
        } else if (strcmp(argv[i], "--remove_report") == 0 && i + 2 < argc) {
            const char *dist = argv[++i];
            int rid = atoi(argv[++i]);
            remove_report(dist, rid,&u);
 
        } else if (strcmp(argv[i], "--update_threshold") == 0 && i + 2 < argc) {
            const char *dist = argv[++i];
            int val = atoi(argv[++i]);
            update_threshold(dist, val, &u);
 
        } else if (strcmp(argv[i], "--filter") == 0 && i + 1 < argc) {
            const char *dist = argv[++i];
            
            int ncond = 0;
            char **conds = &argv[i + 1];
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                ncond++;
                i++;
            }
            filter(dist, &u, conds, ncond);
        } else if (strcmp(argv[i], "--remove_district")==0 && i + 1 < argc){
            remove_district(argv[++i],&u);
        }
    }
    

    return 0;
}
