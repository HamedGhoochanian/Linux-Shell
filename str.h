//
// Created by hamed on 12/14/20.
//
#include <string.h>


char *replaceStrAtBeg(char *str, char *needle, char *replace)
{

    char *str_res = strstr(str, needle);

    if (str_res == NULL)
    { // not found
        return str;
    }
    else if ((str_res - str) != 0)
    { // not at the beggining
        return str;
    }

    char *res = malloc(strlen(str) + strlen(replace) - strlen(needle) + 1);

    strcpy(res, replace);

    strcpy(res + strlen(replace), str + strlen(needle));

    return res;
}

#ifndef SHELL_STR_H
#define SHELL_STR_H

#endif //SHELL_STR_H
