#ifndef CORE_H
#define CORE_H

typedef struct {
    char id[37];
    char email[255];
    char first_name[255];
    char last_name[255];
    char country[80];       /* app.country -> nicename */
} User;

int core_view_home(User **user, int *numRows);

#endif