#ifndef COMMON_H
#define COMMON_H

typedef struct 
{
    char id[5];
    char name[50];
    char contact[13];
    char status[11];
}Student;

typedef struct
{
    char classID[5];
    char instructorID[5];
    char martialArt[50];
    char dateTime[17];
    unsigned int capacity;
    unsigned int bookedCount;
    char status[10];
}Class;

#endif