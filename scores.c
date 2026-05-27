#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define SIZE      30
#define PATH_SIZE 512
#define MAX_INSPECTORS 256

typedef struct {
    float latitude;
    float longitude;
} GPS;

typedef struct {
    int  ID;
    char inspector_name[SIZE];
    GPS  coordinates;
    char issue_category[SIZE];
    int  severity_level;
    long timestamp;          
    char description[SIZE * 5];
} REPORT;

typedef struct {
    char name[SIZE];
    int  score;
    int  count;
} InspectorEntry;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: scorer <district_id>\n");
        return 1;
    }

    const char *district = argv[1];

  
    char path[PATH_SIZE];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        
        printf("DISTRICT:%s\n", district);
        printf("ERROR:Cannot open %s\n", path);
        printf("END\n");
        fflush(stdout);
        return 1;
    }

    
    InspectorEntry inspectors[MAX_INSPECTORS];
    int n_inspectors = 0;

    REPORT r;
    while (read(fd, &r, sizeof(REPORT)) == sizeof(REPORT)) {
       
        int found = -1;
        for (int i = 0; i < n_inspectors; i++) {
            if (strncmp(inspectors[i].name, r.inspector_name, SIZE) == 0) {
                found = i;
                break;
            }
        }
        if (found == -1) {
            if (n_inspectors >= MAX_INSPECTORS) continue; 
            strncpy(inspectors[n_inspectors].name, r.inspector_name, SIZE - 1);
            inspectors[n_inspectors].name[SIZE - 1] = '\0';
            inspectors[n_inspectors].score = 0;
            inspectors[n_inspectors].count = 0;
            found = n_inspectors++;
        }
        inspectors[found].score += r.severity_level;
        inspectors[found].count++;
    }
    close(fd);

    

printf("DISTRICT|%s\n", district);
if (n_inspectors == 0) {
    printf("INFO|No reports found.\n");
} else {
    int total_reports = 0;
    for (int i = 0; i < n_inspectors; i++) {
        printf("INSPECTOR|%s|SCORE|%d|x|x|REPORTS|%d\n",
               inspectors[i].name,
               inspectors[i].score,
               inspectors[i].count);
        total_reports += inspectors[i].count;
    }
    printf("SUMMARY|x|x|%d\n", total_reports);
}
    fflush(stdout);
    return 0;
}