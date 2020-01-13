#ifndef BOBHASH32_H
#define BOBHASH32_H

#include <string.h>

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short ushort;
//typedef unsigned long long int ulong;

#define MAX(a, b) (a > b? a:b)
#define MIN(a, b) (a < b? a:b)

#define MAX_PRIME 1229
//#define INTERVAL 0.1

#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}\

uint BOBHash32(const uchar* str, uint len, uint num);

#endif // BOBHASH32_H

