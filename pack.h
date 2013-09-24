#ifndef PACK_H
#define PACK_H

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

void packi16(unsigned char *buf, unsigned int i);

unsigned int unpacki16(unsigned char *buf);

int8_t pack(unsigned char *buf, const char *format, ...);

void unpack(unsigned char *buf, const char *format, ...);

#endif
