//
// Created by jonas on 8/16/25.
//

#ifndef PAK_H
#define PAK_H

typedef struct {
  char *extension;
  char *directory;
  char *fileName;
  unsigned int size;
  unsigned int alSize;
  unsigned int unSize;
  unsigned int pkSize;
  unsigned int offset;
} pak_entry;

typedef struct {
  char *fileName;
  char *ext;
  unsigned int size;
  unsigned int realSize;
  int compressed;
} pakked_file;

typedef struct {
  char *path;
  int fileCount;
  int noWrite;
  pakked_file *files;
} pakked_folder;

typedef struct {
  pakked_folder *pakFolders;
  unsigned int pakFolderCount;
  unsigned int totalFiles;
  int wii;
} pak_file_structure;

typedef struct {
  char *path;
  int valid;
  unsigned int pakEntryCount;
  pak_entry *pakEntries;
  int pakFolderCount;
  pakked_folder *pakFolders;
  int wii;
} pak_file;

int pakunpak(char *path);
int pakrepak(char *path);
pak_file pakread(char *path);
#endif // PAK_H
