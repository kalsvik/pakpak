#include <stdio.h>

#include "pak.h"

#include <string.h>

void help() {
  printf("Commands:\n");
  printf("- unpak <FILE.pak> | unpacks the specified PAK file.\n");
  printf("- pak <FOLDER> | packs the specified UNPAK folder.\n");
  printf("- Drag Support - You can drag a folder or a PAK file onto the "
         "executable to pack or unpack the target.\n");
}

int main(int argc, char **argv) {
  printf("pakpak - Repacker & Unpacker for Epic Mickey PAK files. - Made by "
         "Jonas Kalsvik <jonas@kalsvik.no>\n");

  if (argc <= 1) {
    help();
    printf("Please specify a command.\n");
    return -2;
  }

  if (argc >= 2) {

    if (strcmp(argv[1], "unpak") == 0) {
      printf("unpak chosen\n");

      if (argc < 3) {
        printf("You must specify a path.\n");
        return -3;
      }

      pakunpak(argv[2]);

    } else if (strcmp(argv[1], "pak") == 0) {
      printf("pak chosen\n");
      if (argc < 3) {
        printf("You must specify a path.\n");
        return -3;
      }
      pakrepak(argv[2]);
    }
  }

  return 0;
}
