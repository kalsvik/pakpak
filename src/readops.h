//
// Created by jonas on 8/16/25.
//

#ifndef READOPS_H
#define READOPS_H
#include <stdio.h>

unsigned int getval(const unsigned char *val, int mode);
char *readstr(FILE *file, int newline);
#endif // READOPS_H
