#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

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

int main(int argc, char**argv){


    return 0;
}
