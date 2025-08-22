//
// Created by jonas on 8/16/25.
//

#ifndef WRITEOPS_H
#define WRITEOPS_H
#include <stdio.h>
#define CHUNK 16348
int writeval(unsigned char *buffer, unsigned int dec, int mode);
int reverse(char *src, char *dest);
void mkpath(const char *dir);
int zlibdeflate(FILE *source, FILE *destination);
int zlibflate(FILE *source, FILE *destination);
#endif // WRITEOPS_H
