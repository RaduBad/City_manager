#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <dirent.h>

#define SIZE 30

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
    time_t timestmp;
    char description[SIZE];
}REPORT;

typedef struct{
    char user[SIZE];
    char role[SIZE];
    char district_id[SIZE];
    int report_id;
    int value;
}COMMANDS;

void add_files(char* district_id){
    open("reports.dat",O_APPEND | O_CREAT | O_WRONLY ,0664);
    chmod("reports.dat",0664);
    open("district.cfg",O_APPEND | O_CREAT | O_WRONLY ,0640);
    chmod("district.cfg",0640);
    open("logged_district",O_APPEND | O_CREAT | O_WRONLY ,0644);
    chmod("logged_district",0644);
}

void add(char* district_id,char* user){
    char path[256];
    mkdir(district_id,0750);
    chmod(district_id, 0750);
    snprintf(path, sizeof(path), "%s/reports.dat", district_id);
    open(path,O_APPEND | O_CREAT | O_WRONLY ,0664);
    chmod("reports.dat",0664);
    REPORT* r=malloc(sizeof(REPORT));
    strcpy(r->inspector_name,user);
    printf("ID report :");
    scanf("%d",&r->ID);

}

void list(char* district_id){

}

void view(char* district_id, int report_id){

}

void update_threshold(char* district_id, int value){

}
void filter(char* district_id, int value){

}

int main(int argc, char**argv){


    COMMANDS com;
    for (int i = 0 ; i < argc ; i++){
        if(strcmp(argv[i],"--role")==0)
            strcpy(com.role,argv[i+1]);
        if(strcmp(argv[i],"--user")==0)
            strcpy(com.user,argv[i+1]);
        if(strcmp(argv[i],"--add")==0)
            add(argv[++i],com.user);
        //if(strcmp(argv[i],"--list")==0)
           // list();




    }

    return 0;
}
