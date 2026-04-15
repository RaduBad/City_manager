#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<sys/stat.h>

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

void add(char* district_id){

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
            add();
        if(strcmp(argv[i],"--list")==0)
            list();
        if(strcmp(argv[i],"--list")==0)


    }

    return 0;
}
