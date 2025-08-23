//
// Created by jonas on 8/16/25.
//

#include "pak.h"
#include "readops.h"
#include "writeops.h"

#include "zlib/zlib.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define BASEOFF 32

pak_file_structure pakpakreadorder(const char *path) {

  printf("Reading .pakpak-order list...\n");

  pak_file_structure f = {};

  pakked_folder *folder = malloc(512 * sizeof(pakked_folder));
  int currentFolderIndex = -1;
  int currentFileIndex = -1;
  int totalFiles = 0;
  char orderFilePath[256] = {0};
  int fileMode = 1;
  sprintf(orderFilePath, "%s/.pakpak-order", path);
  FILE *orderFile = fopen(orderFilePath, "rb");
  if (orderFile == NULL) {
    printf("could not open .pakpak-order\n");
    return f;
  }

  char *mode = readstr(orderFile, 1);
  printf("%s", mode);

  if (strcmp(mode, "WII") == 0) {
    f.wii = 1;
  } else if (strcmp(mode, "WIN") == 0) {
    f.wii = 0;
  } else {
    printf("invalid mode prefix in .pakpak-order\n");
    return f;
  }

#if __WIN32__
  fseek(orderFile, 2, SEEK_CUR);
#else
  fseek(orderFile, 1, SEEK_CUR);
#endif

  while (!feof(orderFile)) {

    char operator[2] = {0};
    fread(operator, 1, 1, orderFile);

    if (feof(orderFile)) {
      break;
    }

    if (operator[0] == 'D') {

      currentFolderIndex++;

      if (fileMode == 0) {
        printf("pakpak order has an illegal empty directory.\n");
        break;
      }
      fileMode = 0;

      fseek(orderFile, 1, SEEK_CUR);
      char *directory = readstr(orderFile, 1);

#if __WIN32__ // skip two bytes when @ newline on windows because bill gates is
              // evil
      fseek(orderFile, 2, SEEK_CUR);
#else
      fseek(orderFile, 1, SEEK_CUR);
#endif

      folder[currentFolderIndex].path = directory;
      folder[currentFolderIndex].files = malloc(256 * sizeof(pakked_file));
      folder[currentFolderIndex].fileCount = 0;
      folder[currentFolderIndex].noWrite = 0;

      for (int i = 0; i < currentFolderIndex; ++i) {
        if (strcmp(folder[i].path, directory) == 0) {
          folder[currentFolderIndex].noWrite = 1;
          break;
        }
      }

    } else if (operator[0] == 'F') {
      if (fileMode == 0) {
        currentFileIndex = -1;
      }

      fileMode = 1;
      currentFileIndex++;
      fread(operator, 1, 1, orderFile);

      int compressed = 0;
      if (operator[0] == 'C') {
        compressed = 1;
        fseek(orderFile, 1, SEEK_CUR);
      }

      char *file = readstr(orderFile, 2);
      fseek(orderFile, 1, SEEK_CUR);
      char *ext = readstr(orderFile, 1);

      if (ext == NULL) {
        ext = calloc(1, 4);
      }

#if __WIN32__ // see above
      fseek(orderFile, 2, SEEK_CUR);
#else
      fseek(orderFile, 1, SEEK_CUR);
#endif

      char filePath[256] = {0};
      char compressedFilePath[256] = {0};

      sprintf(filePath, "%s/%s/%s", path, folder[currentFolderIndex].path,
              file);
      sprintf(compressedFilePath, "%s-compressed", filePath);

      FILE *pakkedFile = fopen(filePath, "rb");

      if (pakkedFile == NULL) {
        printf("referenced file in pakpak-order does not exist (%s)\n",
               filePath);
        free(ext);
        free(file);
        free(folder);
        return f;
      }

      fseek(pakkedFile, 0, SEEK_END);
      const unsigned int fileSize = ftell(pakkedFile);
      fclose(pakkedFile);

      folder[currentFolderIndex].files[currentFileIndex].fileName = file;
      folder[currentFolderIndex].files[currentFileIndex].ext = ext;
      folder[currentFolderIndex].files[currentFileIndex].size = fileSize;
      folder[currentFolderIndex].files[currentFileIndex].realSize = fileSize;
      folder[currentFolderIndex].files[currentFileIndex].compressed =
          compressed;

      if (compressed == 1) {
        printf("Compressing %s for repacking.\n", file);
        FILE *inputFile = fopen(filePath, "rb");
        FILE *compressedFile = fopen(compressedFilePath, "wb");
        zlibdeflate(inputFile, compressedFile);
        fclose(compressedFile);
        fclose(inputFile);
        inputFile = fopen(compressedFilePath, "rb");
        fseek(inputFile, 0, SEEK_END);
        const unsigned int compressedFileSize = ftell(inputFile);
        fclose(inputFile);
        folder[currentFolderIndex].files[currentFileIndex].size =
            compressedFileSize;
        printf("Compressed %s for repacking. (size=%d)\n", file,
               compressedFileSize);
      }
      folder[currentFolderIndex].fileCount++;
      totalFiles += 1;
    } else if (operator[0] == '\n') {
      fseek(orderFile, 1, SEEK_CUR);
    } else {
      printf("pakpak order file has invalid syntax. (%d)\n",
             (int)ftell(orderFile));

      free(folder);
      return f;
    }
  }

  f.pakFolderCount = currentFolderIndex + 1;
  f.pakFolders = folder;
  f.totalFiles = totalFiles;

  printf("Finished reading pakorder\n");

  return f;
}

pak_file pakread(char *path) {
  printf("Opening PAK file...\n");

  pak_file pakfile = {.path = path, .valid = 0};

  FILE *file = fopen(pakfile.path, "rb");

  if (file == NULL) {
    return pakfile;
  }

  printf("Validating file ID...\n");
  char fileid[5] = {0};
  fread(&fileid, 1, 4, file);

  int wii = 0; // V_LITTLEENDIAN
  if (strcmp(fileid, " KAP") == 0) {
    wii = 1; // = V_BIGENDIAN
  } else if (strcmp(fileid, "PAK ") != 0) {
    printf("this is not a PAK file (got %s)\n", fileid);
    return pakfile;
  }

  printf("File is a valid PAK file. Wii=%d\n", wii);

  unsigned char version[4] = {0};
  unsigned char baseOffset[4] = {0};
  unsigned char dataOffset[4] = {0};
  unsigned char fileAmount[4] = {0};

  fread(&version, 1, 4, file);
  fseek(file, 4, SEEK_CUR);
  fread(&baseOffset, 1, 4, file);
  fread(&dataOffset, 1, 4, file);

  const unsigned int baseOffsetValue = getval(baseOffset, wii);
  const unsigned int dataOffsetValue = getval(dataOffset, wii);

  fseek(file, 32, SEEK_SET);

  fread(&fileAmount, 1, 4, file);
  const unsigned int fileAmountValue = getval(fileAmount, wii);

  printf("Files in PAK: %d\n", fileAmountValue);

  long currentSeekedLocation = 0;
  currentSeekedLocation = ftell(file);

  const unsigned int namesOffset =
      currentSeekedLocation + fileAmountValue * 24; // oh i see what we're doin
  unsigned int offset = baseOffsetValue + dataOffsetValue;

  pak_entry *pak_entries = malloc(fileAmountValue * sizeof(pak_entry));

  pakked_folder *pakked_folders =
      malloc(fileAmountValue * sizeof(pakked_folder));

  char *cachedFolderName = "";
  int currentFolderIndex = -1;
  int currentFolderCount = -1;

  for (unsigned int i = 0; i < fileAmountValue; ++i) {
    unsigned char realSize[4] = {0};
    unsigned char compressedSize[4] = {0};
    unsigned char alignedSize[4] = {0};
    unsigned char fileOffset[4] = {0};
    unsigned char folderOffset[4] = {0};
    char fileType[4] = {0};

    fread(&realSize, 1, 4, file);
    fread(&compressedSize, 1, 4, file);
    fread(&alignedSize, 1, 4, file);
    fread(&folderOffset, 1, 4, file);
    fread(&fileType, 4, 1, file);
    fread(&fileOffset, 1, 4, file);

    char correctedFileType[4] = {0};

    if (pakfile.wii == 0) {
      reverse(fileType, correctedFileType);
    } else {
      memcpy(correctedFileType, fileType, 4);
    }

    currentSeekedLocation += 24;

    unsigned const int alignedSizeValue = getval(alignedSize, wii);
    unsigned const int realSizeValue = getval(realSize, wii);
    unsigned const int compressedSizeValue = getval(compressedSize, wii);

    unsigned const int folderOffsetValue =
        getval(folderOffset, wii) + namesOffset;
    unsigned const int fileNameOffsetValue =
        getval(fileOffset, wii) + namesOffset;
    fseek(file, folderOffsetValue, SEEK_SET);
    char *folder = readstr(file, 0);
    fseek(file, fileNameOffsetValue, SEEK_SET);
    char *fileName = readstr(file, 0);
    fseek(file, currentSeekedLocation, SEEK_SET);

    if (strcmp(cachedFolderName, folder) != 0) { // check immediate cache
      currentFolderCount++;
      currentFolderIndex = currentFolderCount;
      cachedFolderName = folder;

      pakked_file *filesBuffer = malloc(fileAmountValue * sizeof(pakked_file));
      pakked_folders[currentFolderIndex].path = folder;
      pakked_folders[currentFolderIndex].fileCount = 0;
      pakked_folders[currentFolderIndex].files = filesBuffer;
    }

    printf("%s passed as #%d\n", fileName, i);

    assert(currentFolderIndex > -1);

    pakked_file *pakkedFile =
        &pakked_folders[currentFolderIndex]
             .files[pakked_folders[currentFolderIndex].fileCount];

    char *extension = malloc(5);
    memset(extension, 0, 5);
    memcpy(extension, correctedFileType, 4);

    pakkedFile->fileName = fileName;
    pakkedFile->ext = extension;
    pakkedFile->size = compressedSizeValue;
    pakkedFile->realSize = realSizeValue;

    pakked_folders[currentFolderIndex].fileCount++;

    // printf("Extension: %s\n", correctedFileType);
    // printf("Full Path: %s/%s\n", folder, fileName);
    // printf("File size: %d\n", compressedSizeValue);
    // printf("File Offset: %d\n", offset);
    // printf("File Name Offset: %d\n", fileNameOffsetValue);
    // printf("Folder Name Offset: %d\n", folderOffsetValue);
    // printf("Aligned Size %d\n", alignedSizeValue);
    // printf("Real Size %d\n", realSizeValue);

    if (compressedSizeValue != realSizeValue) {
      // printf("Compressed\n");
    }

    pak_entries[i].extension = fileType;
    pak_entries[i].directory = folder;
    pak_entries[i].fileName = fileName;
    pak_entries[i].alSize = alignedSizeValue;
    pak_entries[i].pkSize = compressedSizeValue;
    pak_entries[i].unSize = realSizeValue;
    pak_entries[i].size = compressedSizeValue;
    pak_entries[i].offset = offset;

    offset += alignedSizeValue;
  }

  fclose(file);

  pakfile.pakEntries = pak_entries;
  pakfile.pakEntryCount = fileAmountValue;
  pakfile.valid = 1;
  pakfile.pakFolders = pakked_folders;
  pakfile.pakFolderCount = currentFolderCount;
  pakfile.wii = wii;

  return pakfile;
}

int pakrepak(char *path) {
  pak_file_structure fileStructure = pakpakreadorder(path);
  unsigned char version[4];
  unsigned char baseOffset[4] = {0};
  unsigned char dataOffset[4] = {0};
  unsigned char fileCount[4] = {0};

  unsigned const char wiiVersion[4] = {0x0, 0x0, 0x0, 0x02};
  unsigned const char pcVersion[4] = {0x02, 0x0, 0x0, 0x0};

  if (fileStructure.wii == 1) {
    memcpy(version, wiiVersion, 4);
  } else {

    memcpy(version, pcVersion, 4);
  }

  int wii = fileStructure.wii;

  printf("beginning pakking. wii=%d\n", wii);

  writeval(baseOffset, 32, wii);
  writeval(fileCount, fileStructure.totalFiles, wii);

  writeval(dataOffset, 0, wii);

  FILE *file = fopen("out.pak", "wb");
  fwrite(wii == 1 ? " KAP" : "PAK ", 4, 1, file);
  fwrite(version, 4, 1, file);
  fwrite("\0\0\0\0", 4, 1, file); // whitespace
  fwrite(baseOffset, 4, 1, file);
  fwrite(dataOffset, 4, 1, file);
  fwrite("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 12, 1,
         file); // 12 byte whitespace before base file info cluster
  fwrite(fileCount, 4, 1, file);

  unsigned int folderNameOffsetLocations[512][512] = {0};

  // for loop here hypothetically : add file info
  for (unsigned int i = 0; i < fileStructure.pakFolderCount; ++i) {
    const pakked_folder *folder = &fileStructure.pakFolders[i];

    for (int j = 0; j < folder->fileCount; ++j) {
      const pakked_file insertingFile = folder->files[j];
      unsigned char size[4] = {0};
      writeval(size, insertingFile.realSize, wii);

      unsigned char compressedSize[4] = {0};
      writeval(compressedSize, insertingFile.size, wii);

      unsigned int alignedSizeValue =
          (32 - (insertingFile.size % 32)) % 32 + insertingFile.size;
      unsigned char alignedSize[4] = {0};
      writeval(alignedSize, alignedSizeValue, wii);

      fwrite(size, 4, 1, file); // real size
      fwrite(compressedSize, 4, 1, file);
      fwrite(alignedSize, 4, 1,
             file); // aligned size - if its an actual issue i'll align

      folderNameOffsetLocations[i][j] = ftell(file);
      fwrite("\0\0\0\0", 4, 1, file);

      char paddedExtension[4] = {0};
      strncpy(paddedExtension, insertingFile.ext, strlen(insertingFile.ext));
      char correctedExtension[4] = {0};
      if (wii == 0) {
        reverse(paddedExtension, correctedExtension);
      } else {
        memcpy(paddedExtension, correctedExtension, 4);
      }

      fwrite(correctedExtension, 4, 1, file);
      fwrite("\0\0\0\0", 4, 1, file);
    }
  }

  // for loop hypothetically ends here

  // another another for loop

  const unsigned int nameDataOffset = ftell(file);

  char *writtenFolders[8096] = {0};
  unsigned int writtenFoldersOffset[8096] = {0};
  int currentWrittenFoldersIndex = 0;
  char *writtenFiles[8096] = {0};
  unsigned int writtenFilesOffset[8096] = {0};
  int currentWrittenFilesIndex = 0;

  for (unsigned int i = 0; i < fileStructure.pakFolderCount; ++i) {
    const pakked_folder folder = fileStructure.pakFolders[i];

    unsigned int folderNameOffset = ftell(file);

    if (folder.noWrite == 0) {
      fwrite(folder.path, 1, strlen(folder.path), file);
      fwrite("\0", 1, 1, file);

      writtenFolders[currentWrittenFoldersIndex] = folder.path;
      writtenFoldersOffset[currentWrittenFoldersIndex] = folderNameOffset;
      currentWrittenFoldersIndex++;
    } else {
      for (unsigned int l = 0; l < fileStructure.totalFiles; ++l) {

        if (writtenFolders[l] == NULL) {
          break;
        }

        if (strcmp(folder.path, writtenFolders[l]) == 0) {
          folderNameOffset = writtenFoldersOffset[l];
        }
      }
    }

    for (int j = 0; j < folder.fileCount; ++j) {
      const pakked_file insertingFile = folder.files[j];

      unsigned int noWriteFile = 0;
      for (unsigned int l = 0; l < fileStructure.totalFiles; ++l) {
        if (writtenFiles[l] == NULL) {
          break;
        }

        if (strcmp(insertingFile.fileName, writtenFiles[l]) == 0) {
          noWriteFile = writtenFilesOffset[l];
        }
      }

      unsigned const int currentLocation = ftell(file);
      unsigned const int folderNamePointerOffset =
          folderNameOffsetLocations[i][j];

      unsigned char nameOffset[4] = {0};
      unsigned char folderOffset[4] = {0};

      unsigned int nameOffsetValue = currentLocation;
      unsigned int folderOffsetValue = folderNameOffset;

      if (noWriteFile != 0) {
        nameOffsetValue = noWriteFile;
      }

      writeval(nameOffset, nameOffsetValue - nameDataOffset, wii);
      writeval(folderOffset, folderOffsetValue - nameDataOffset, wii);

      fseek(file, folderNamePointerOffset, SEEK_SET);
      fwrite(folderOffset, 1, 4, file);
      fseek(file, 4, SEEK_CUR);
      fwrite(nameOffset, 1, 4, file);
      fseek(file, currentLocation, SEEK_SET);

      if (noWriteFile == 0) {
        fwrite(insertingFile.fileName, strlen(insertingFile.fileName), 1, file);

        if (folder.fileCount - 1 == j &&
            i == fileStructure.pakFolderCount - 1) {
          continue; // do not add whitespace if the last file in name cluster.
        }

        fwrite("\0", 1, 1, file);

        writtenFiles[i] = insertingFile.fileName;
        writtenFilesOffset[i] = nameOffsetValue;
        currentWrittenFilesIndex++;
      }
    }
  }
  // align header, file info & path info cluster.

  unsigned int headerPaddingSize = (32 - ftell(file) % 32) % 32;
  char *headerPadding = malloc(headerPaddingSize);
  memset(headerPadding, 0, headerPaddingSize);
  fwrite(headerPadding, headerPaddingSize, 1, file);
  free(headerPadding);

  // write data offset to header then return to current point in file

  unsigned const int curLocation = ftell(file);
  writeval(dataOffset, curLocation - 32, wii);
  fseek(file, 16, SEEK_SET);
  fwrite(dataOffset, 4, 1, file);
  fseek(file, curLocation, SEEK_SET);

  // write data cluster to pak

  for (unsigned int i = 0; i < fileStructure.pakFolderCount; ++i) {
    const pakked_folder *folder = &fileStructure.pakFolders[i];
    for (int j = 0; j < folder->fileCount; ++j) {
      pakked_file insertingFile = folder->files[j];
      char inputPath[256] = {0};
      char compressedInputPath[256] = {0};
      unsigned char *fileBuffer = malloc(insertingFile.size);
      sprintf(inputPath, "%s/%s/%s", path, folder->path,
              insertingFile.fileName);

      sprintf(compressedInputPath, "%s-compressed", inputPath);
      FILE *inputFile;
      if (insertingFile.compressed == 0) {
        inputFile = fopen(inputPath, "rb");
      } else {
        inputFile = fopen(compressedInputPath, "rb");
      }
      fread(fileBuffer, insertingFile.size, 1, inputFile);
      fwrite(fileBuffer, insertingFile.size, 1, file);
      fclose(inputFile);

      if (insertingFile.compressed == 1) {
        remove(compressedInputPath);
      }

      free(fileBuffer);
      unsigned int padding = (32 - (insertingFile.size % 32)) % 32;
      char *paddingBytes = malloc(padding);
      memset(paddingBytes, 0, padding);
      fwrite(paddingBytes, padding, 1, file);
      free(paddingBytes);
    }
  }

  printf("Successfully pakked folder! Output saved as: 'out.pak' (size=%d)",
         (int)ftell(file));

  free(fileStructure.pakFolders);
  fclose(file);

  return 0;
}

int pakunpak(char *path) {
  const pak_file pakfile = pakread(path);

  if (pakfile.valid == 0) {
    printf("This pakfile is not valid!\n");
    free(pakfile.pakEntries);
    return -1;
  }

  FILE *file = fopen(pakfile.path, "rb");

  char baseDirName[256] = {0};
  sprintf(baseDirName, "%s-unpak", pakfile.path);

#ifdef __WIN32
  mkdir(baseDirName);
#else
  mkdir(baseDirName, 0755);
#endif

  for (unsigned int i = 0; i < pakfile.pakEntryCount; ++i) {
    char fullDirectory[256] = {0};

    sprintf(fullDirectory, "%s/%s", baseDirName,
            pakfile.pakEntries[i].directory);

    mkpath(fullDirectory);

    unsigned char *fileBuffer = malloc(pakfile.pakEntries[i].pkSize);
    fseek(file, pakfile.pakEntries[i].offset, SEEK_SET);

    printf("Reading %d bytes from offset %d in pakfile\n",
           pakfile.pakEntries[i].pkSize, (int)ftell(file));

    fread(fileBuffer, 1, pakfile.pakEntries[i].pkSize, file);

    char fileName[256] = {0};
    sprintf(fileName, "%s/%s", fullDirectory, pakfile.pakEntries[i].fileName);
    char compressedFileName[256] = {0};
    sprintf(compressedFileName, "%s-compressed", fileName);
    FILE *outputFile;
    if (pakfile.pakEntries[i].pkSize != pakfile.pakEntries[i].unSize) {
      printf("%s is compressed, so we have to deflate it with zlib.\n",
             pakfile.pakEntries[i].fileName);

      outputFile = fopen(compressedFileName, "wb");
    } else {
      outputFile = fopen(fileName, "wb");
    }

    if (outputFile == NULL) {
      printf("failed to create output file! (%s)", fileName);
      free(pakfile.pakEntries);
      free(fileBuffer);
      return -1;
    }

    fwrite(fileBuffer, 1, pakfile.pakEntries[i].pkSize, outputFile);
    fclose(outputFile);
    free(fileBuffer);

    if (pakfile.pakEntries[i].pkSize != pakfile.pakEntries[i].unSize) {
      FILE *toDeflate = fopen(compressedFileName, "rb");
      outputFile = fopen(fileName, "wb");
      int result = zlibflate(toDeflate, outputFile);

      fclose(toDeflate);
      fclose(outputFile);

      remove(compressedFileName);

      if (result == Z_OK) {
        printf("Finished deflating %s\n", compressedFileName);
      } else {

        printf("Error deflating %s. error code: %d\n", compressedFileName,
               result);

        free(pakfile.pakEntries);
        return -1;
      }
    }

    printf("Extracted: %s/%s\n", fullDirectory, pakfile.pakEntries[i].fileName);
  }

  char infoFilePath[256] = {0};
  sprintf(infoFilePath, "%s/.pakpak-order", baseDirName);
  FILE *infoFile = fopen(infoFilePath, "w");
  const int pakFolderCount = pakfile.pakFolderCount + 1;

  int filesWritten = 0;

  if (pakfile.wii == 1) {
    fwrite("WII\n", 1, 4, infoFile);
  } else {
    fwrite("WIN\n", 1, 4, infoFile);
  }

  for (int i = 0; i < pakFolderCount; ++i) {
    char folderPath[256] = {0};
    sprintf(folderPath, "D %s\n", pakfile.pakFolders[i].path);
    fwrite(folderPath, strlen(folderPath), 1, infoFile);
    for (int j = 0; j < pakfile.pakFolders[i].fileCount; ++j) {
      char filePath[256] = {0};
      filesWritten++;

      char *extCut = pakfile.pakFolders[i].files[j].ext;

      if (pakfile.pakFolders[i].files[j].size !=
          pakfile.pakFolders[i].files[j].realSize) {
        sprintf(filePath, "FC %s %s\n", pakfile.pakFolders[i].files[j].fileName,
                extCut);
      } else {
        sprintf(filePath, "F %s %s\n", pakfile.pakFolders[i].files[j].fileName,
                extCut);
      }
      free(extCut);
      fwrite(filePath, strlen(filePath), 1, infoFile);
    }
  }
  printf("wrote %d files into order list!\n", filesWritten);

  fclose(infoFile);
  fclose(file);

  free(pakfile.pakFolders);
  free(pakfile.pakEntries);

  return 0;
}
