//
// Created by jonas on 8/16/25.
//

#include "writeops.h"
#include "const.h"
#include "zlib/zlib.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int writeval(unsigned char *buffer, unsigned int dec, int mode) {
  if (mode == V_BIGENDIAN) {
    buffer[0] = (dec >> 24) & 0xFF;
    buffer[1] = (dec >> 16) & 0xFF;
    buffer[2] = (dec >> 8) & 0xFF;
    buffer[3] = dec & 0xFF;
  } else {
    buffer[0] = (dec) & 0xFF;
    buffer[1] = (dec >> 8) & 0xFF;
    buffer[2] = (dec >> 16) & 0xFF;
    buffer[3] = (dec >> 24) & 0xFF;
  }
  return 0;
}

// i've stolen this from carl norum on stack overflow like 5 times because its
// just the best solution so shoutout to him
void mkpath(const char *dir) {
  char tmp[512];
  char *p = NULL;
  snprintf(tmp, sizeof(tmp), "%s", dir);
  const size_t len = strlen(tmp);
  if (tmp[len - 1] == '/')
    tmp[len - 1] = 0;
  for (p = tmp + 1; *p; p++)
    if (*p == '/') {
      *p = 0;
#ifdef __WIN32
      mkdir(tmp);
#else
      mkdir(tmp, 0755);
#endif

      *p = '/';
    }
#ifdef __WIN32
  mkdir(tmp);
#else
  mkdir(tmp, 0755);
#endif
}

int reverse(char *src, char *dest) {
  dest[0] = src[3];
  dest[1] = src[2];
  dest[2] = src[1];
  dest[3] = src[0];
  return 0;
}

int zlibdeflate(FILE *source, FILE *destination) {
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char in[CHUNK];
  unsigned char out[CHUNK];

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit(&strm, Z_BEST_COMPRESSION);
  if (ret != Z_OK)
    return ret;

  do {
    strm.avail_in = fread(in, 1, CHUNK, source);
    if (ferror(source)) {
      (void)deflateEnd(&strm);
      return Z_ERRNO;
    }
    if (strm.avail_in == 0)
      break;
    strm.next_in = in;

    do {
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = deflate(&strm, Z_NO_FLUSH);
      assert(ret != Z_STREAM_ERROR);
      have = CHUNK - strm.avail_out;
      if (fwrite(out, 1, have, destination) != have || ferror(destination)) {
        (void)deflateEnd(&strm);
        return Z_ERRNO;
      }
    } while (strm.avail_out == 0);
    assert(strm.avail_in == 0);
  } while (ret != Z_STREAM_END);

  do {
    strm.avail_out = CHUNK;
    strm.next_out = out;
    ret = deflate(&strm, Z_FINISH);
    assert(ret != Z_STREAM_ERROR);
    have = CHUNK - strm.avail_out;
    if (fwrite(out, 1, have, destination) != have || ferror(destination)) {
      (void)deflateEnd(&strm);
      return Z_ERRNO;
    }
  } while (strm.avail_out == 0);

  (void)deflateEnd(&strm);
  return Z_OK;
}

int zlibflate(FILE *source, FILE *destination) {
  z_stream stream;
  unsigned char in[CHUNK];
  unsigned char out[CHUNK];

  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;
  stream.avail_in = 0;
  stream.next_in = Z_NULL;

  int ret = inflateInit(&stream);
  if (ret != Z_OK) {
    return ret;
  }

  do {
    stream.avail_in = fread(in, 1, CHUNK, source);
    if (ferror(source)) {
      (void)inflateEnd(&stream);
      return Z_ERRNO;
    }
    if (stream.avail_in == 0)
      break;

    stream.next_in = in;

    do {
      stream.avail_out = CHUNK;
      stream.next_out = out;

      ret = inflate(&stream, Z_NO_FLUSH);
      switch (ret) {
      case Z_NEED_DICT:
        ret = Z_DATA_ERROR;
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        (void)inflateEnd(&stream);
        return ret;
      default:;
      }

      unsigned have = CHUNK - stream.avail_out;

      if (fwrite(out, 1, have, destination) != have || ferror(destination)) {
        (void)inflateEnd(&stream);
        return Z_ERRNO;
      }

    } while (stream.avail_out == 0);

  } while (ret != Z_STREAM_END);

  (void)inflateEnd(&stream);

  if (ret != Z_STREAM_END) {
    return Z_DATA_ERROR;
  }

  return Z_OK;
}
