#ifndef CORE_H
#define CORE_H

typedef struct {
    char id[37];
    char email[255];
    char country[80];       /* app.country -> nicename */
    char full_name[255];
} User;

int core_view_home(User **user, int *num_rows, int *num_columns);

#endif