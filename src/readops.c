//
// Created by jonas on 8/16/25.
//

#include "readops.h"
#include "const.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
unsigned int getval(const unsigned char *val, int mode) {
  uint32_t value = 0;
  if (mode == V_BIGENDIAN) {
    value = ((uint32_t)val[0] << 24) | ((uint32_t)val[1] << 16) |
            ((uint32_t)val[2] << 8) | ((uint32_t)val[3]);
  } else {
    value = ((uint32_t)val[0]) | ((uint32_t)val[1] << 8) |
            ((uint32_t)val[2] << 16) | ((uint32_t)val[3] << 24);
  }

  return value;
}

char *readstr(FILE *file, int newline) {
  const unsigned int currentOffset = ftell(file);

  char buf[256] = {0};
  fread(buf, 1, 256, file);

  int sizeOfString = 0;

  while (1) {

    if (buf[sizeOfString] == 0 ||
        (newline == 1 &&
         (buf[sizeOfString] == '\n' || buf[sizeOfString] == 0x0D)) ||
        (newline == 2 && (buf[sizeOfString] == ' '))) {
      break;
    }

    sizeOfString++;

    if (sizeOfString > 255) {
      fseek(file, currentOffset, SEEK_SET);
      printf("read string overflowed the buffer.\n");
      return NULL;
    }
  }

  if (sizeOfString == 0) {
    fseek(file, currentOffset, SEEK_SET);
    return NULL;
  }

  char *finalString = malloc(sizeOfString + 1);
  memset(finalString, 0, sizeOfString + 1);
  strncpy(finalString, buf, sizeOfString);

  fseek(file, currentOffset + sizeOfString, SEEK_SET);

  return finalString;
}
